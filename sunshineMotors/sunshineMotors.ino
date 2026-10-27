int eastPin = A0;  //red
int southPin = A1; //yellow
int northPin = A2; //white or grey
int westPin = A3;  // green

int eastValue = 0;
int southValue = 0;
int northValue = 0;
int westValue = 0;

int eW_error = 0;
int nS_error = 0;

int eW_deadband = 35;
int nS_deadband = 35;

float eastVolt = 0;
float southVolt = 0;
float northVolt = 0;
float westVolt = 0;

float voltageSource = 5;

float totalLight = 0;
float avgLight = 0;

 //adjustables
const int PPR[2] = {400, 400}; // pulses per revolution set on first lower then upper motor driver (ignore gearbox)
const int pinTable[2][3] = { //pulse, dir, enable
  {3, 4, 6},
  {7, 8, 10}
};
const unsigned long interval = 5*1000;    //change first number to change how many seconds
  //

int btmsign; // bottom sign to determine direction
int topsign; // top sign to determine direction
int btmrev;
int toprev;
bool emergencystop = false;
bool sensorcondition = true;
bool othersensorcondition = false;
bool resetbutton = false;



unsigned long pmils = 0;
unsigned long pmils2 = 0;
int secs = 0;

float minLight = 1000; // minimum average light value that should be read by the sensor


int StepMotor(int motorNum, float revs, int dir, float speed) { 
  // dir: -1 = ccw, 1 = cw | revs = number of revolutions | speed = max rpm | motorNum: 0 = lower 1 = upper

  float steps = PPR[motorNum]*revs;
  int pulpin = pinTable[motorNum][0];
  int dirpin = pinTable[motorNum][1];
  float mindel = (1/(speed*PPR[motorNum]/60))*1000000;
  float delaytime;
  float num;
  String print = "Motor ";
  print += motorNum;
  Serial.println(print);
  Serial.println(steps);
  Serial.println(revs);
  Serial.println(pinTable[motorNum][0]);
  Serial.println(pinTable[motorNum][1]);
  for (int i = 1; i <= steps; i++) {
    if (emergencystop) {digitalWrite(pinTable[1][2], HIGH); digitalWrite(pinTable[0][2], HIGH); return; } else {digitalWrite(pinTable[1][2], LOW); digitalWrite(pinTable[0][2], LOW);} 
    //emergency stop condition, move/copy if needed
    digitalWrite(pulpin, HIGH); //send high pulse
    if (dir == 1) {
      digitalWrite(dirpin, HIGH);
    }
    else if (dir == -1) {
      digitalWrite(dirpin, LOW);
    }
    if (i < steps/2) {
      num = i;
    }
    else if (i >= steps/2) {
      num = -(i - steps);
    }
    delaytime = constrain((2000 - num), mindel, 2000);
    delayMicroseconds(delaytime);
    digitalWrite(pulpin, LOW); //send low pulse
  }  
  digitalWrite(pinTable[motorNum][2], HIGH);
  Serial.println("done!");
  return steps, dir;
}

void setup() {
  //lower/sshaft
  pinMode(3,OUTPUT); //pulse
  pinMode(4,OUTPUT); //direction
  pinMode(6,OUTPUT); //enable

  //upper/dshaft
  pinMode(7,OUTPUT);  //pulse
  pinMode(8,OUTPUT);  //direction
  pinMode(10,OUTPUT); //enable
  digitalWrite(pinTable[1][2], HIGH);
  digitalWrite(pinTable[0][2], HIGH);
  Serial.begin(9600);
  Serial.println("initialized!");
}

void loop() {
  eastValue = analogRead(eastPin);
  southValue = analogRead(southPin);
  northValue = analogRead(northPin);
  westValue = analogRead(westPin);

  


  totalLight = eastValue + southValue + northValue + westValue;
  avgLight = totalLight / 4;

  if (avgLight < minLight){
    StepMotor(0, 0, 0, 0);
    StepMotor(1, 0, 0, 0);

  } 


  // if all sensors read 0, something is wrong and the system should stop
  if (totalLight == 0) {
    sensorcondition = false;
    othersensorcondition = true;
  } else {
    sensorcondition = true;
    othersensorcondition = false;
  }

  eW_error = eastValue - westValue;
  nS_error = northValue - southValue;
  btmsign = eW_error/abs(eW_error);
  topsign = nS_error/abs(nS_error);



  unsigned long cmils = millis();
  if (cmils - pmils2 >= 1000) {
    pmils2 = cmils;
    secs++;
    Serial.println(secs);
  }
  if (cmils - pmils >= interval) {
    Serial.println("interval has passed! reading sun sensor now:");
    if (sensorcondition) {
      /*btmrev = 30;    //place these in if/then statements when sensor reading is normalized
      toprev = 30;
      StepMotor(0, btmrev, 1, 300); //motor, revolutions, direction, max speed
      StepMotor(1, toprev, 1, 300);
      */
      if(abs(eW_error) > eW_deadband) {
        btmrev = 3;
        StepMotor(0, btmrev, btmsign, 300);
      }
      if(abs(nS_error) > nS_deadband) {
        toprev = 3;
        StepMotor(1, toprev, topsign, 300);
      }
    }
    secs = 0;
    cmils = millis();
    pmils = cmils;
  }
  if (othersensorcondition) {
    emergencystop = true;
  }
  if (resetbutton) {
    emergencystop = false;
    sensorcondition = true;
  }

  //outputSensorValue();

  //delay(1000);

}





void outputSensorValue() {
  Serial.print("East: ");
  Serial.println(eastValue);

  Serial.print("South: ");
  Serial.println(southValue);

  Serial.print("North: ");
  Serial.println(northValue);

  Serial.print("West: ");
  Serial.println(westValue);
}
