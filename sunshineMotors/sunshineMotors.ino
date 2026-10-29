 //adjustables---------------------------------------------
const int PPR[2] = {400, 800}; // pulses per revolution set on first lower then upper motor driver (ignore gearbox)
const int pinTable[2][3] = { //pulse, dir, enable
  {3, 4, 6},
  {7, 8, 10}
};

float Position[2] = {0, 0};   // position of motors in pulses, first = lower, second = upper
const int MaxRevs[2] = {62.5, 10};  // gear ratios divided by two to set max +/- revolutions

float minLight = 0; // minimum average light value that should be read by the sensor

const unsigned long interval = 5*1000;    //change first number to change how many seconds
//-----------------------------------

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

int btmsign; // bottom sign to determine direction
int topsign; // top sign to determine direction
int btmrev;
int toprev;
bool emergencystop = false;
bool sensorcondition = false;
bool othersensorcondition = false;
bool resetbutton = false;

unsigned long pmils = 0;
unsigned long pmils2 = 0;
int secs = 0;

void StepMotor(int motorNum, float revs, int dir, float speed) { 
  // dir: -1 = ccw, 1 = cw | revs = number of revolutions (make sure PPR * revs is a whole number) | speed = max rpm | motorNum: 0 = lower 1 = upper

  float steps = PPR[motorNum]*revs;
  float pulcheck = Position[motorNum] + steps*dir;
  float MaxPul = MaxRevs[motorNum]*PPR[motorNum];
  
  //over rotation check, will reverse direction and adjust steps accordingly
  if(abs(pulcheck) > MaxPul) {   
    Serial.println("too much rotation in this direction! reversing...");
    int pulsigncheck = -pulcheck/abs(pulcheck);
    float goal = (MaxPul - (abs(pulcheck) - (MaxPul)))*pulsigncheck;
    steps = abs(goal - Position[motorNum]);
    revs = steps/PPR[motorNum];
    dir *= -1;
  }

  int pulpin = pinTable[motorNum][0];
  int dirpin = pinTable[motorNum][1];
  float mindel = (1/(speed*PPR[motorNum]/60))*1000000;
  float delaytime;
  float num;
  String print = "Motor ";
  print += motorNum;
  Serial.println(print);
  Serial.println(steps*dir);
  Serial.println(revs*dir);
  for (float i = 1; i <= steps; i++) {
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
  digitalWrite(dirpin, LOW);
  digitalWrite(pulpin, LOW);
  digitalWrite(pinTable[motorNum][2], HIGH);
  Position[motorNum] += steps*dir;
  Serial.println("done! now at position:");
  Serial.println(Position[motorNum]);
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
    autoHome();             // if the light detected is below the minimum threshold, move both motors to their home positions
  } 


  // if all sensors read 0, something is wrong and the system should stop. The analog pins will only read 0 if there is no connection at all.
  // if they all read 1024, the max of the ADC, then there is continuity, meaning the sensors should be properly connected and functioning
  if (totalLight == 0) {
    othersensorcondition = true;
  } else {
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
  if (cmils - pmils >= interval || sensorcondition) {
    Serial.println("interval has passed or sensor button pressed! reading sun sensor now:");
    //if(abs(eW_error) > eW_deadband) {
      btmrev = 5;
      StepMotor(0, btmrev, btmsign, 300); //motor, revolutions, direction, max speed
    //}
    if(abs(nS_error) > nS_deadband) {
      toprev = 5;
      StepMotor(1, toprev, topsign, 300);
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
    delay(3000);
  }

  //outputSensorValue();

  //delay(1000);

}


void autoHome(){
  // Move both motors to their home positions
  btmsign = Position[0]/abs(Position[0]);
  topsign = Position[1]/abs(Position[1]);

  StepMotor(0, Position[0], -btmsign, 30);         /*-BtmSign means the movement will be in the opposite direction of where it has already travelled */
  StepMotor(1, Position[1], -topsign, 30);         // same behavior, but for the top motor
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
