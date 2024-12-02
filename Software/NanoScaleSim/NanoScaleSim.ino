// CAN Send Example
//
//#define DISPLAY_ATTACHED     //Comment out if not using display
#ifdef DISPLAY_ATTACHED
  #include <Wire.h>
  #include <Adafruit_GFX.h>
  #include <Adafruit_SSD1306.h>
  #define WIRE Wire
#endif
#include "mcp_can.h"
#include <SPI.h>
#include <EEPROM.h>


#ifdef DISPLAY_ATTACHED
uint8_t DisplayChanged=1;
long DisplayTrigger=0;
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 64, &WIRE);
#endif

typedef struct EE_S{
  uint8_t  Start;
  uint32_t Scale0;
  uint32_t Scale1;
  uint32_t Scale2;
  uint32_t Scale3;
  uint16_t Speed;
}EE_T;
EE_T NVMem;
#define EEBYTES 19
long storeNVMemTrigger = 0;

MCP_CAN CAN0(10);     // Set CS to pin 10
long scaleXmitLockout=0;
#define SCALE_XMIT_LOCK 50	//one transmission a second for 4 scales
typedef struct Scale_S{
  uint32_t value;     //in grams
  uint8_t stability;
}Scale_T;
uint8_t adjustMode=4;   //0-3 scale, 4 = speed
Scale_T scale[4];
uint16_t speed=0;       //in mm/s
byte data[8] = {0x00, 0x00, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x00};
byte speedData[8] = {0x00, 0x00, 0xEF, 0xFB, 0x04, 0x00, 0xFF, 0x00};

//********Rotary variables ***********
// These constants won't change. They're used to give names to the pins used:
const int pulsePin = 3;       //Rotary Pulse input
const int directionPin = 4;  //Direction input
const int buttonPin = 5;      //PB Input
const int led1=A0, led2=A1, led3=A2, led4=A3;
int oldValue = 1;             //What was the last scan of pulsepin showing
int oldButtonValue=0;         //What was the last scan on the PB showing
int clickCount=0;             //CC rotation count
long printTarget=0;           //Print delay
uint8_t btnPressCycles=0;     //How long is button pressed

int pulseValue = 0;    // value read from the pot
int directionValue = 0;  // value output to the PWM (analog out)
int buttonValue = 1;

long buttonLockout=0;
#define BUTTON_LOCK   250
long lockoutPulse=0;         //Pulse lockout
#define DEBOUNCE_LOCK 20     // 20 ms
/**************************************/

void trigger(){
  long now=millis();
  if(now<lockoutPulse){
    return;
  }
  directionValue = !digitalRead(directionPin);
  if(directionValue){
	  if(clickCount>0){
		  clickCount--;
	  }
  }else{
    clickCount++;
  }
  //debounce
  lockoutPulse=now+DEBOUNCE_LOCK;
}

void GetEEPROM(){
  int ii=0;
  uint8_t *ptr = &NVMem.Start;
  for(ii=0;ii<EEBYTES;ii++){
    *ptr = EEPROM.read(ii);
    ptr++;
  }
  scale[0].value=NVMem.Scale0;
  scale[1].value=NVMem.Scale1;
  scale[2].value=NVMem.Scale2;
  scale[3].value=NVMem.Scale3;
  speed=NVMem.Speed;
}
void SetEEPROM(){
  int ii;
  uint8_t *ptr = &NVMem.Start;
  NVMem.Scale0=scale[0].value;
  NVMem.Scale1=scale[1].value;
  NVMem.Scale2=scale[2].value;
  NVMem.Scale3=scale[3].value;
  NVMem.Speed=speed;
  for (ii = 0 ; ii < EEBYTES ; ii++) {
    EEPROM.write(ii, *ptr);
    ptr++;
  }
  Serial.println("NV Mem Saved");
}

void nextAdjMode(void){
    adjustMode++;
    if(adjustMode>4){
      adjustMode=0;
    }
    Serial.print("Adjust Mode: ");
    //Set Click Count
    if(adjustMode>3){
      clickCount=(uint16_t)(((float)speed+0.5)/44.7); //add .5 to round
      Serial.println(speed);
    }else{
      clickCount=(uint32_t)(((float)scale[adjustMode].value+.5)/45359.2); //add .5 to round
      Serial.println(scale[adjustMode].value);
    }
#ifdef DISPLAY_ATTACHED
    DisplayChanged=1;
#endif
}

void transmitCycle(uint8_t cycle){
  byte sndStat=0;
  long xmitValue=0;
  switch(cycle){
    case 0:
    case 1:
    case 2:
    case 3:
      xmitValue=scale[cycle].value;
      data[0]=(byte)((cycle+1)<<4) & 0xF0;
      data[4]=(byte)(xmitValue & 0x000000FF);
      data[5]=(byte)((xmitValue >>8) & 0x000000FF);
      data[6]=(byte)((xmitValue >>16) & 0x000000FF);
      data[7]=(byte)((xmitValue >>24) & 0x000000FF);
      sndStat = CAN0.sendMsgBuf(0x0CCBFF82, 1, 8, data);
      break;
    case 4:
    default:
      speedData[0]=(byte)(speed & 0x00FF);
      speedData[1]=(byte)((speed >> 8) & 0x00FF);
      sndStat = CAN0.sendMsgBuf(0x0CFE4926, 1, 8, speedData);
      break;
  }
  if(sndStat != CAN_OK){
    Serial.print("Error Sending Message...");
    Serial.println(cycle);
  }
}
#ifdef DISPLAY_ATTACHED
void updateDisplay(){
  if(DisplayChanged){
    display.clearDisplay();
    // text display tests
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    switch(adjustMode){
      case 0:
      case 1:
      case 2:
      case 3:
        display.print("Scale ");
        display.print(adjustMode+1);
        display.println(" Lb");
        display.println((uint16_t)((float)scale[adjustMode].value/453.592+0.5));
        break;
      default:
      case 4:
        display.println("Speed MPH:");
        display.println((float)speed/447, 1); //display tenths of mph
        break;
    }

    display.setCursor(0,0);
    DisplayChanged=0;
  }
}
#endif

void setup()
{
  uint8_t error;
  Serial.begin(115200);
  error=CAN0.begin(MCP_ANY, CAN_250KBPS, MCP_8MHZ);
  // Initialize MCP2515 running at 16MHz with a baudrate of 500kb/s and the masks and filters disabled.
  if(error == CAN_OK){
    Serial.println("MCP2515 Initialized Successfully!");
  } 
  else{
    Serial.print("Error Initializing MCP2515...");
    Serial.println(error);
  } 

  CAN0.setMode(MCP_NORMAL);   // Change to normal mode to allow messages to be transmitted
  EEPROM.begin();
  GetEEPROM();
  nextAdjMode();  //start at zero and update clickCount
  pinMode(led1,OUTPUT);
  pinMode(led2,OUTPUT);
  pinMode(led3,OUTPUT);
  pinMode(led4,OUTPUT);
  //********* Rotary Setup ****************
  pinMode(pulsePin,INPUT);
  pinMode(directionPin,INPUT);
  pinMode(buttonPin,INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pulsePin),trigger,FALLING);
  //***************************************
#ifdef DISPLAY_ATTACHED
  //********* Display Setup ***************
    // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Address 0x3C for 128x32
  display.display();
  DisplayTrigger=1220;  // let the splash screen linger
#endif
}

void loop()
{
	long now = millis();
  int ii=0;
  static int xmitCycle=0;
  static uint16_t oldClickCount=0;

  if(storeNVMemTrigger && (storeNVMemTrigger<now)){
    SetEEPROM();
    storeNVMemTrigger=0;
  }

  if(oldClickCount!=clickCount){
    oldClickCount=clickCount;
    storeNVMemTrigger=now+5000; //store change in 5 sec
#ifdef DISPLAY_ATTACHED
    DisplayChanged=1;
#endif
  }
  switch(adjustMode){
    case 4:
    default:
      digitalWrite(led4,HIGH);
      digitalWrite(led3,HIGH);
      digitalWrite(led2,HIGH);
      digitalWrite(led1,HIGH);
      break;
    case 3:
      digitalWrite(led4,HIGH);
      digitalWrite(led3,LOW);
      digitalWrite(led2,LOW);
      digitalWrite(led1,LOW);
      break;
    case 2:
      digitalWrite(led4,LOW);
      digitalWrite(led3,HIGH);
      digitalWrite(led2,LOW);
      digitalWrite(led1,LOW);
      break;
    case 1:
      digitalWrite(led4,LOW);
      digitalWrite(led3,LOW);
      digitalWrite(led2,HIGH);
      digitalWrite(led1,LOW);
      break;
    case 0:
      digitalWrite(led4,LOW);
      digitalWrite(led3,LOW);
      digitalWrite(led2,LOW);
      digitalWrite(led1,HIGH);
      break;
  }
  if(adjustMode>3){
    speed=(uint16_t)((float)clickCount*44.7+.5);
    digitalWrite(led4,HIGH);
    digitalWrite(led3,LOW);
    digitalWrite(led2,LOW);
    digitalWrite(led1,LOW);
  }else{
    scale[adjustMode].value=(long)((float)clickCount*45359.2+.5);
  }

  if(scaleXmitLockout<now){
    transmitCycle(xmitCycle++);
    if(xmitCycle>4){
      xmitCycle=0;
    }
	  scaleXmitLockout = now + SCALE_XMIT_LOCK;
  }
#ifdef DISPLAY_ATTACHED
  if(now>DisplayTrigger){
    updateDisplay();
    display.display();
    DisplayTrigger=now+250;
  }
#endif 
  //*************Rotary Functions****************
   buttonValue= digitalRead(buttonPin);

  //Zero the count if button pressed
  if(buttonValue==0 && now>buttonLockout){
    btnPressCycles++;
    buttonLockout=now+BUTTON_LOCK;
    if(btnPressCycles==3){
      clickCount=0;
      Serial.println("Zeroing....");
    }
  }
  if((buttonValue==1) && (btnPressCycles>0)){
    //Short press
    if(btnPressCycles<3){
      nextAdjMode();
    }
    btnPressCycles=0;
  }


  //****Print Routine****
  if(now>printTarget){
    // print the results to the Serial Monitor every .5 sec:
    Serial.print("output = ");
    Serial.println(clickCount);
    printTarget=now+500;
  }
}

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/