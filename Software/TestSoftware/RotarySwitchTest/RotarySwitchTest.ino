
// These constants won't change. They're used to give names to the pins used:
const int pulsePin = 4;       //Rotary Pulse input
const int directionPin = A0;  //Direction input
const int buttonPin = 3;      //PB Input
int oldValue = 1;             //What was the last scan of pulsepin showing
int oldButtonValue=0;         //What was the last scan on the PB showing
int clickCount=0;             //CC rotation count
long printTarget=0;           //Print delay

int pulseValue = 0;    // value read from the pot
int directionValue = 0;  // value output to the PWM (analog out)
int buttonValue = 1;

void setup() {
  // initialize serial communications at 9600 bps:
  Serial.begin(115200);
  pinMode(pulsePin,INPUT);
  pinMode(directionPin,INPUT);
  pinMode(buttonPin,INPUT_PULLUP);
}

void loop() {
  long now=millis();
  // Read in the values
  pulseValue = digitalRead(pulsePin);
  directionValue = digitalRead(directionPin);
  buttonValue= digitalRead(buttonPin);

  //Zero the count if button pressed
  if(buttonValue==0){
    clickCount=0;
  }

  //Between detents, the signal goes low on the signal
  if(oldValue!=pulseValue && pulseValue==0){
    //If the direction value is low at time of pulse, increase
    if(!directionValue){
      clickCount++;
    }else{
      if(clickCount>0){
        clickCount--;     //decrease if direction value high and clickCount isn't zero
      }
    }
  }
  oldValue=pulseValue;
  
  //****Print Routine****
  if(now>printTarget){
    // print the results to the Serial Monitor every .5 sec:
    Serial.print("output = ");
    Serial.println(clickCount);
    printTarget=now+500;
  }
}


