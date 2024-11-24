
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

void trigger(){
  long now=millis();
  if(now<lockoutPulse){
    return;
  }
  directionValue = digitalRead(directionPin);
  if(directionValue){
    clickCount--;
  }else{
    clickCount++;
  }
  //debounce
  lockoutPulse=now+DEBOUNCE_LOCK;
}

void setup() {
  // initialize serial communications at 9600 bps:
  Serial.begin(115200);
  pinMode(pulsePin,INPUT);
  pinMode(directionPin,INPUT);
  pinMode(buttonPin,INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pulsePin),trigger,FALLING);
}

void loop() {
  long now=millis();
  // Read in the values
  //pulseValue = digitalRead(pulsePin);
  //directionValue = digitalRead(directionPin);
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