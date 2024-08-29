/********************************************************
//PID controller that reads inputs from multiple RTD sensors (takes the average) and outputs heat accordingly. 
// Controlls 3 PID loops independently. 
 ********************************************************/

#include <PID_v1.h>
#include <Motoron.h>
#include <Wire.h>
#include <Adafruit_MAX31865.h>
// #include <Ethernet.h>
// #include <SPI.h>

// Use software SPI: CS, DI, DO, CLK
Adafruit_MAX31865 max_1 = Adafruit_MAX31865(10, 11, 12, 13);
Adafruit_MAX31865 max_2 = Adafruit_MAX31865(9, 11, 12, 13);
// Adafruit_MAX31865 max_3 = Adafruit_MAX31865(3, 11, 12, 13);
// Adafruit_MAX31865 max_4 = Adafruit_MAX31865(4, 11, 12, 13);
// Adafruit_MAX31865 max_5 = Adafruit_MAX31865(5, 11, 12, 13);
// Adafruit_MAX31865 max_6 = Adafruit_MAX31865(6, 11, 12, 13);
// Adafruit_MAX31865 max_7 = Adafruit_MAX31865(7, 11, 12, 13);
// Adafruit_MAX31865 max_8 = Adafruit_MAX31865(8, 11, 12, 13);
// Adafruit_MAX31865 max_9 = Adafruit_MAX31865(9, 11, 12, 13);
// Adafruit_MAX31865 max_10 = Adafruit_MAX31865(10, 11, 12, 13);

// The value of the Rref resistor. Use 430.0 for PT100 and 4300.0 for PT1000
#define RREF      430.0
// The 'nominal' 0-degrees-C resistance of the sensor
// 100.0 for PT100, 1000.0 for PT1000
#define RNOMINAL  100.0

MotoronI2C mc;

//Define Variables we'll be connecting to
double Setpoint;
double Input1, Input2, Input3;
double Output1, Output2, Output3;

//Specify the links and initial tuning parameters
double Kp=80, Ki=2.8, Kd=0.02;

PID myPID1(&Input1, &Output1, &Setpoint, Kp, Ki, Kd, DIRECT);
PID myPID2(&Input2, &Output2, &Setpoint, Kp, Ki, Kd, DIRECT);
PID myPID3(&Input3, &Output3, &Setpoint, Kp, Ki, Kd, DIRECT);

void setup()
{
  Serial.begin(9600);
  Wire.begin();

  // reset the motor controller
  mc.reinitialize();    // Bytes: 0x96 0x74
  mc.disableCrc();      // Bytes: 0x8B 0x04 0x7B 0x43
  mc.clearResetFlag();  // Bytes: 0xA9 0x00 0x04

  max_1.begin(MAX31865_2WIRE);  // set to 2WIRE or 4WIRE as necessary
  max_2.begin(MAX31865_2WIRE);  
  max_3.begin(MAX31865_2WIRE); 
  max_4.begin(MAX31865_2WIRE); 
  max_5.begin(MAX31865_2WIRE); 
  max_6.begin(MAX31865_2WIRE); 
  max_7.begin(MAX31865_2WIRE); 
  max_8.begin(MAX31865_2WIRE); 
  max_9.begin(MAX31865_2WIRE); 
  max_10.begin(MAX31865_2WIRE); 

  // Input the average of two sensors on each filter
  Input1 = (max_1.temperature(RNOMINAL, RREF) + max_2.temperature(RNOMINAL, RREF))/2;
  Input2 = (max_3.temperature(RNOMINAL, RREF) + max_4.temperature(RNOMINAL, RREF))/2;
  Input3 = (max_5.temperature(RNOMINAL, RREF) + max_6.temperature(RNOMINAL, RREF))/2;

  Setpoint = 31; // higher because measuring temperature on the rim

  //turn the PID on
  myPID1.SetMode(AUTOMATIC);
  myPID2.SetMode(AUTOMATIC);
  myPID3.SetMode(AUTOMATIC);  

  // Setting the limits of the output motor
  myPID1.SetOutputLimits(0, 800);
  myPID2.SetOutputLimits(0, 800);
  myPID3.SetOutputLimits(0, 800);
}

void loop()
{
  static unsigned long startTime = millis();
  unsigned long currentTime = millis();
  float elapsedTime = (currentTime - startTime) / 1000.0;

  Input1 = (max_1.temperature(RNOMINAL, RREF) + max_2.temperature(RNOMINAL, RREF))/2;
  Input2 = (max_3.temperature(RNOMINAL, RREF) + max_4.temperature(RNOMINAL, RREF))/2;
  Input3 = (max_5.temperature(RNOMINAL, RREF) + max_6.temperature(RNOMINAL, RREF))/2;

  myPID1.Compute();
  myPID2.Compute();
  myPID3.Compute();

  // Serial.print("RTD Temperature 1 = "); Serial.println(max_1.temperature(RNOMINAL, RREF)); // temp readout in Celsius
  // Serial.print("RTD Temperature 2 = "); Serial.println(max_2.temperature(RNOMINAL, RREF)); // temp readout in Celsius
  float RTD_temp1 = max_1.temperature(RNOMINAL, RREF);
  float RTD_temp2 = max_2.temperature(RNOMINAL, RREF);
  float RTD_temp3 = max_3.temperature(RNOMINAL, RREF);
  float RTD_temp4 = max_4.temperature(RNOMINAL, RREF);
  float RTD_temp5 = max_5.temperature(RNOMINAL, RREF);
  float RTD_temp6 = max_6.temperature(RNOMINAL, RREF);
  float RTD_temp7 = max_7.temperature(RNOMINAL, RREF);
  float RTD_temp8 = max_8.temperature(RNOMINAL, RREF);
  float RTD_temp9 = max_9.temperature(RNOMINAL, RREF);
  float RTD_temp10 = max_10.temperature(RNOMINAL, RREF);
  float avg_temp1 = (RTD_temp1 + RTD_temp2)/2;
  float avg_temp2 = (RTD_temp3 + RTD_temp4)/2;
  float avg_temp3 = (RTD_temp5 + RTD_temp6)/2;

  float targetspeed1 = Output1;
  float percent_power1 = (targetspeed1/800)*100;

  float targetspeed2 = Output2;
  float percent_power2 = (targetspeed2/800)*100;

  float targetspeed3 = Output1;
  float percent_power3 = (targetspeed3/800)*100;
  
  // Control motor speed based on PID output
  mc.setSpeedNow(1, Output1);
  mc.setSpeedNow(2, Output2);
  mc.setSpeedNow(3, Output3);

  // Just print the organized outputs I want as Time, Power Output, Temp1, Temp2, avgTemp
  Serial.print(elapsedTime);
  Serial.print(",");
  Serial.print(percent_power1);
  Serial.print(",");
  Serial.print(percent_power2);
  Serial.print(",");
  Serial.print(percent_power3);
  Serial.print(",");
  Serial.print(avg_temp1);
  Serial.print(",");
  Serial.print(avg_temp2);
  Serial.print(",");
  Serial.print(avg_temp3);
  Serial.print(",");
  Serial.print(RTD_temp1);
  Serial.print(",");
  Serial.print(RTD_temp2);
  Serial.print(",");
  Serial.print(RTD_temp3);
  Serial.print(",");
  Serial.print(RTD_temp4);
  Serial.print(",");
  Serial.print(RTD_temp5);
  Serial.print(",");
  Serial.print(RTD_temp6);
  Serial.print(",");
  Serial.print(RTD_temp7);
  Serial.print(",");
  Serial.print(RTD_temp8);
  Serial.print(",");
  Serial.print(RTD_temp9);
  Serial.print(",");
  Serial.print(RTD_temp10);
  Serial.print(",");

  delay(150);
}
