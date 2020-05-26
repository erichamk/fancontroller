int sensorPin = 13;//ambient, thermistor 11
int sensorPinTop = 15; //casetop
int sensorPinGpu = 14; //casegpu

int pinPWMfront = 11;
int pinPWMgpu = 6;
int pinPWMback = 5;
int pinPWMtop = 9;

double sensorValAmbient; double tempAmbient;
double sensorValCaseTop; double tempCaseTop;
double sensorValCaseGpu; double tempCaseGpu;
int pcOnce = 0;
int pc = 0;
int tempCpu = 99; int tempGpu = 99; int useGpu = 0; int clockGpu = 0;
const int avgCount = 100;

int topAdc1 = 710; int topAdc2 = 628; int topC1 = 24; int topC2 = 33;
double delta; double deltaC; double deltaCPU; double deltaGPU;
int minDelta = 4;
int maxDelta = 7;
int minPwmFront = 130;
int maxPwmFront = 180;
int minPwmTop = 145;
int maxPwmTop = 200;
int minPwmBack = 100;
int maxPwmBack = 200;

int PWMValFront = minPwmFront;
int PWMValFront2 = minPwmFront; //para gradual
int PWMValTop = minPwmTop;
int PWMValTop2 = minPwmTop;
int PWMValBack = minPwmBack;
int PWMValBack2 = minPwmBack;
int PWMValGpu = 0;
int PWMuser = 0;
int deltauser = 0;
int counter = 0;

int monitoring = 0;
int idle = 0;
char *strings[10];
char *ptr = NULL;

//EEPROM
#include <EEPROM.h>
int address = 0;
byte value;
void(* resetFunc) (void) = 0;

void setup() {
  // put your setup code here, to run once:
  int myEraser = 7;             // this is 111 in binary and is used as an eraser
  TCCR1B &= ~myEraser;   // this operation (AND plus NOT),  set the three bits in TCCR2B to 0 (~myEraser==1111000, solo toca los ultimos 3 bits y pone 0)
  TCCR3B &= ~myEraser;
  TCCR4B &= ~myEraser;
  int myPrescaler = 1;         // this could be a number in [1 , 6]. In this case, 3 corresponds in binary to 011.
  TCCR1B |= myPrescaler;  //this operation (OR), replaces the last three bits in TCCR2B with our new value 011
  TCCR3B |= myPrescaler;
  TCCR4B |= myPrescaler;
  pinMode(sensorPin, INPUT);
  pinMode(sensorPinTop, INPUT);
  pinMode(sensorPinGpu, INPUT);
  pinMode(pinPWMfront, OUTPUT);
  pinMode(pinPWMtop, OUTPUT);
  pinMode(pinPWMback, OUTPUT);
  //pinMode(pinPWMgpu, OUTPUT);
  //analogWrite(pinPWMgpu, 150);
  analogWrite(pinPWMfront, 150);
  analogWrite(pinPWMtop, 150);
  analogWrite(pinPWMback, 150);
  //digitalWrite(8,HIGH);
  Serial.begin(9600);
  /*while (address < EEPROM.length()){
    value = EEPROM.read(address);
    Serial.print(address);
    Serial.print("\t");
    Serial.print(value, DEC);
    Serial.println();
    address = address + 1;
    }*/
}

void loop() {
  pc = 0;
  parseSerial();
  if (pcOnce && !pc){
     analogWrite(pinPWMfront, 150);
     analogWrite(pinPWMtop, 150);
     analogWrite(pinPWMback, 150);
  }
  /*if (PWMuser > 0 || deltauser > 0)
    counter++;
  if (counter > 60) {
    PWMuser = 0;
    deltauser = 0;
    counter = 0;
  }*/

  sensorValAmbient = sensorValCaseTop = sensorValCaseGpu = 0;

  for (int i = 0; i < avgCount; i++)
  {
    sensorValAmbient += analogRead(sensorPin); //975 25c,
    sensorValCaseTop += analogRead(sensorPinTop); //677:25c, 639:31c
    sensorValCaseGpu += analogRead(sensorPinGpu); //677:25c, 639:31c
    delay(10);
  }
  sensorValAmbient = sensorValAmbient / avgCount; 
  sensorValCaseTop = sensorValCaseTop / avgCount;
  sensorValCaseGpu = sensorValCaseGpu / avgCount;
  
  tempAmbient = temptable_11(sensorValAmbient);
  tempCaseTop = modifiedMap(sensorValCaseTop, topAdc1, topAdc2, topC1, topC2); //temptable_4(sensorVal2);
  tempCaseGpu = modifiedMap(sensorValCaseGpu, topAdc1, topAdc2, topC1, topC2); //temptable_4(sensorVal2);

  deltaCPU = tempCaseTop - tempAmbient;
  deltaGPU = tempCaseGpu - tempAmbient;  
  //if (clockGpu > 1500 && useGpu > 10)
  if (useGpu > 10)
    deltaC = deltaGPU + 2;
  else
    deltaC = max(deltaCPU, deltaGPU);
    
  if (deltauser > 0) deltaC = deltauser;
  
  //map and assign pwm values to the fan output 0 to 255 corresponds to 0 to 100%
  //PWMVal = map(delta, 0, 50, 130, 150);
  PWMValFront = modifiedMap(deltaC, minDelta, maxDelta, minPwmFront, maxPwmFront);
  PWMValTop = modifiedMap(deltaC, minDelta, maxDelta, minPwmTop, maxPwmTop);
  PWMValGpu = modifiedMap(tempGpu, 60, 80, 120, 255);
  PWMValBack = modifiedMap(deltaC, minDelta, maxDelta, minPwmBack, maxPwmBack);
  
  if (PWMValFront < minPwmFront) PWMValFront = minPwmFront;
  if (PWMValFront > maxPwmFront) PWMValFront = maxPwmFront;
  
  if (PWMValTop < minPwmTop) PWMValTop = minPwmTop;
  if (PWMValTop > maxPwmTop) PWMValTop = maxPwmTop;
  
  if (PWMValBack < minPwmBack) PWMValBack = minPwmBack;
  if (PWMValBack > maxPwmBack) PWMValBack = maxPwmBack;
  
  if (PWMValGpu > 255) PWMValGpu = 255;
  
  if (tempGpu < 60){
    PWMValGpu = 0;
  }
  if (deltauser == 0)
  if (tempCpu < 65 && tempGpu < 60){
    PWMValFront = minPwmFront;
    PWMValTop = minPwmTop;
    PWMValBack = minPwmBack;
  }
  if (PWMuser > 0){
    PWMValFront = PWMuser;
    PWMValTop = PWMuser;
    PWMValBack = PWMuser;
  }
  if (idle == 1){
    PWMValFront = 1;
    PWMValTop = 1;
    PWMValBack = 1;
  }
  //write the PWM value to the pwm output pin
  analogWrite(pinPWMfront, PWMValFront);
  analogWrite(pinPWMtop, PWMValTop);
  analogWrite(pinPWMback, PWMValBack);

  if (monitoring) {
    printStatus();
  }
}
void printStatus() {
  Serial.print("Ambient: "); Serial.print(tempAmbient); if (tempAmbient < 30) Serial.println("c"); else Serial.println("c");
  //Serial.println(sensorVal);
  Serial.print("Case:  "); Serial.print("CPU ");  Serial.print(tempCaseTop); Serial.print("c "); Serial.print("GPU ");  Serial.print(tempCaseGpu); Serial.println("c");
  Serial.print("Delta: "); Serial.print("CPU ");  Serial.print(deltaCPU); Serial.print("c   "); Serial.print("GPU ");  Serial.print(deltaGPU); Serial.print("c "); if (useGpu > 10) Serial.println("*"); else Serial.println();
  //Serial.println(delta);
  Serial.print("PWM front top back: "); Serial.print(PWMValFront); Serial.print(" "); Serial.print(PWMValTop); Serial.print(" "); Serial.print(PWMValBack); if (tempCpu < 60 && tempGpu < 60) Serial.println(" (min)"); else Serial.println(" ");
  Serial.print("CPU GPU: "); Serial.print(tempCpu); Serial.print(" "); Serial.print(tempGpu); Serial.print(" "); Serial.println(useGpu); //Serial.print(" "); Serial.println(PWMValGpu);
}

void parseSerial() {
  while (Serial.available() > 0) {
    //delay(2);
    //Serial.println();
    //char string[32];
    char* string;
    String s = Serial.readStringUntil('\n');
    string = s.c_str();
    //Serial.println(string);
    /*int availableBytes = Serial.available();
      for (int i = 0; i < availableBytes; i++)
      {
      string[i] = Serial.read();
      }*/
    int index = 0;
    ptr = strtok(string, " \n\t");  // takes a list of delimiters
    while (ptr != NULL)
    {
      strings[index] = ptr;
      //Serial.println(strings[index]);
      index++;
      ptr = strtok(NULL, " \n\t");  // takes a list of delimiters
    }

    if (!strcmp(strings[0], "monitoring"))
    {
      if (monitoring) {monitoring = 0;Serial.println("Monitoring Off");}
      else  {monitoring = 1;Serial.println("Monitoring On");}
    }
    if (!strcmp(strings[0], "idle"))
    {
      if (idle) {idle = 0;Serial.println("Idle Off");}
      else {idle = 1;Serial.println("Idle On");}
    }
    if (!strcmp(strings[0], "pc"))
    {
      if (index > 4) {
        pcOnce = 1;
        tempCpu = atoi(strings[1]); tempGpu = atoi(strings[2]); useGpu = atoi(strings[3]); clockGpu = atoi(strings[4]);
      }
      printStatus();
      //Serial.println(atoi(strings[1]));Serial.println(atoi(strings[2]));Serial.println(PWMtemp);
    }
    if (!strcmp(strings[0], "status"))
    {
      printStatus();
    }
    if (!strcmp(strings[0], "pwm"))
    {
      Serial.print("PWM:"); Serial.println(PWMValFront);
      if (index > 1){
        PWMuser = atoi(strings[1]);Serial.print("PWMUser set to ");Serial.println(strings[1]);}
    }
    if (!strcmp(strings[0], "delta"))
    {
      if (index > 1){
        deltauser = atoi(strings[1]);Serial.print("Deltauser set to ");Serial.println(strings[1]);}
    }
    if (!strcmp(strings[0], "curve"))
    {
      Serial.print("Actual curve "); Serial.print(minDelta); Serial.print(" "); Serial.print(maxDelta); Serial.print(" "); Serial.print(minPwmFront); Serial.print(" "); Serial.print(maxPwmFront); 
      Serial.print(" "); Serial.print(minPwmTop); Serial.print(" "); Serial.print(maxPwmTop); Serial.print(" "); Serial.print(minPwmBack); Serial.print(" "); Serial.println(maxPwmBack);
      if (index > 6) {
        minDelta = atoi(strings[1]);
        maxDelta = atoi(strings[2]);
        minPwmFront = atoi(strings[3]);
        maxPwmFront = atoi(strings[4]);
        minPwmTop = atoi(strings[5]);
        maxPwmTop = atoi(strings[6]);
        minPwmBack = atoi(strings[7]);
        maxPwmBack = atoi(strings[8]);
      }
      Serial.print("New curve "); Serial.print(minDelta); Serial.print(" "); Serial.print(maxDelta); Serial.print(" "); Serial.print(minPwmFront);  Serial.print(" "); Serial.print(maxPwmFront); 
      Serial.print(" "); Serial.print(minPwmTop); Serial.print(" "); Serial.print(maxPwmTop); Serial.print(" "); Serial.print(minPwmBack); Serial.print(" "); Serial.println(maxPwmBack);
    }
    if (!strcmp(strings[0], "map"))
    {
      Serial.print("Actual Top map "); Serial.print(topAdc1); Serial.print(" "); Serial.print(topC1); Serial.print(" "); Serial.print(topAdc2); Serial.print(" "); Serial.println(topC2);
      if (index > 4) {
        topAdc1 = atoi(strings[1]); topC1 = atoi(strings[2]); topAdc2 = atoi(strings[3]); topC2 = atoi(strings[4]);
        Serial.print("New top map "); Serial.print(topAdc1); Serial.print(" "); Serial.print(topC1); Serial.print(" "); Serial.print(topAdc2); Serial.print(" "); Serial.println(topC2);
      }
    }
    if (!strcmp(strings[0], "reset"))
    {
      resetFunc();
    }

    while (Serial.read() >= 0) Serial.print("flushing...");
    //Serial.println("ok");
  }
}

double modifiedMap(double x, double in_min, double in_max, double out_min, double out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

double temptable_custom(double r)
{
  if (r >= 531 && r < 584)
    return modifiedMap(r, 531, 584, 41, 35);
  if (r >= 584 && r < 637)
    return modifiedMap(r, 584, 637, 35, 30);
  if (r >= 637 && r < 690)
    return modifiedMap(r, 637, 690, 30, 25);
  if (r >= 690 && r < 743)
    return modifiedMap(r, 690, 743, 25, 20);
  if (r >= 743 && r < 796)
    return modifiedMap(r, 743, 796, 20, 14);
}

double temptable_4(double r)
{
  if (r >= 531 && r < 584)
    return modifiedMap(r, 531, 584, 41, 35);
  if (r >= 584 && r < 637)
    return modifiedMap(r, 584, 637, 35, 30);
  if (r >= 637 && r < 690)
    return modifiedMap(r, 637, 690, 30, 25);
  if (r >= 690 && r < 743)
    return modifiedMap(r, 690, 743, 25, 20);
  if (r >= 743 && r < 796)
    return modifiedMap(r, 743, 796, 20, 14);
}

double temptable_11(double r)
{
  if (r >= 921 && r < 941)
    return modifiedMap(r, 921, 941, 45, 39);
  if (r >= 941 && r < 971)
    return modifiedMap(r, 941, 971, 39, 28);
  if (r >= 971 && r < 981)
    return modifiedMap(r, 971, 981, 28, 23);
  if (r >= 981 && r < 991)
    return modifiedMap(r, 981, 991, 23, 17);
  if (r >= 991 && r < 1001)
    return modifiedMap(r, 991, 1001, 17, 9);
}
/*
  double beta(int adcAverage){
  double BALANCE_RESISTOR   = 4700.0;
  double MAX_ADC = 1023.0;
  double BETA = 3950.0;
  double ROOM_TEMP          = 298.15;   // room temperature in Kelvin
  double RESISTOR_ROOM_TEMP = 10000.0;
  double rThermistor = 0;            // Holds thermistor resistance value
  double tKelvin     = 0;            // Holds calculated temperature

  rThermistor = BALANCE_RESISTOR * ( (MAX_ADC / sensorVal2) - 1);
  tKelvin = (BETA * ROOM_TEMP) /
           (BETA + (ROOM_TEMP * log(rThermistor / RESISTOR_ROOM_TEMP)));
  return  tKelvin - 273.15;

  }*/
