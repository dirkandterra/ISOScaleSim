// CAN Send Example
//

#include "mcp_can.h"
#include <SPI.h>

MCP_CAN CAN0(10);     // Set CS to pin 10
long scaleXmitLockout=0;
#define SCALE_XMIT_LOCK 250	//one transmission a second for 4 scales

//********Rotary variables ***********
// These constants won't change. They're used to give names to the pins used:
const int pulsePin = 3;       //Rotary Pulse input
const int directionPin = A0;  //Direction input
const int buttonPin = 4;      //PB Input
int oldValue = 1;             //What was the last scan of pulsepin showing
int oldButtonValue=0;         //What was the last scan on the PB showing
int clickCount=0;             //CC rotation count
long printTarget=0;           //Print delay

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
  directionValue = digitalRead(directionPin);
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

void setup()
{
  uint8_t error;
  Serial.begin(115200);
  error=CAN0.begin(MCP_ANY, CAN_250KBPS, MCP_8MHZ);
  // Initialize MCP2515 running at 16MHz with a baudrate of 500kb/s and the masks and filters disabled.
  if(error == CAN_OK) Serial.println("MCP2515 Initialized Successfully!");
  else{
    Serial.print("Error Initializing MCP2515...");
    Serial.println(error);
  } 

  CAN0.setMode(MCP_NORMAL);   // Change to normal mode to allow messages to be transmitted
  
  //********* Rotary Setup ****************
  pinMode(pulsePin,INPUT);
  pinMode(directionPin,INPUT);
  pinMode(buttonPin,INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pulsePin),trigger,FALLING);
  //***************************************
}

byte data[8] = {0x20, 0x00, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x00};

void loop()
{
	long now = millis();
	long scaleValue=0;
  
  // send data:  ID = 0x100, Standard CAN Frame, Data length = 8 bytes, 'data' = array of data bytes to send
  if(scaleXmitLockout<now){
	  scaleValue=(long)((float)clickCount*4535.92);
	  data[4]=(byte)(scaleValue & 0x000000FF);
	  data[5]=(byte)((scaleValue >>8) & 0x000000FF);
	  data[6]=(byte)((scaleValue >>16) & 0x000000FF);
	  data[7]=(byte)((scaleValue >>24) & 0x000000FF);
	  byte sndStat = CAN0.sendMsgBuf(0x0CCBFF82, 1, 8, data);
	  if(sndStat != CAN_OK){
		Serial.println("Error Sending Message...");
	  }
	  scaleXmitLockout = now + SCALE_XMIT_LOCK;
  }
  
  //*************Rotary Functions****************
   buttonValue= digitalRead(buttonPin);

  //Zero the count if button pressed
  if(buttonValue==0 && now>buttonLockout){
    clickCount=0;
    buttonLockout=now+BUTTON_LOCK;
    Serial.println("Zeroing....");
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