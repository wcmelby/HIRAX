#include <PID_v1.h>

/********************************************************
//PID controller that reads inputs from multiple RTD sensors (takes the average) and outputs heat accordingly
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
// Adafruit_MAX31865 max_3 = Adafruit_MAX31865(8, 11, 12, 13);
#define IR1_ADDRESS 0x5A
#define IR2_ADDRESS 0x5B

// The value of the Rref resistor. Use 430.0 for PT100 and 4300.0 for PT1000
#define RREF      430.0
// The 'nominal' 0-degrees-C resistance of the sensor
// 100.0 for PT100, 1000.0 for PT1000
#define RNOMINAL  100.0

MotoronI2C mc;

// #define PIN_INPUT 0
// #define PIN_OUTPUT 3

//Define Variables we'll be connecting to
double Setpoint, Input, Output;

//Specify the links and initial tuning parameters
double Kp=80, Ki=2.8, Kd=0.02;
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

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
  // max_3.begin(MAX31865_2WIRE); 

  Input = max_1.temperature(RNOMINAL, RREF);
  // Input = (max_1.temperature(RNOMINAL, RREF) + max_2.temperature(RNOMINAL, RREF))/2;
  Setpoint = 31; // higher because measuring temperature on the rim

  //turn the PID on
  myPID.SetMode(AUTOMATIC);

  myPID.SetOutputLimits(0, 800); // Setting the limits of the output motor
}

void loop()
{
  static unsigned long startTime = millis();
  unsigned long currentTime = millis();
  float elapsedTime = (currentTime - startTime) / 1000.0;

  Input = max_1.temperature(RNOMINAL, RREF);
  // Input = (max_1.temperature(RNOMINAL, RREF) + max_2.temperature(RNOMINAL, RREF))/2;
  myPID.Compute();

  // Serial.print("RTD Temperature 1 = "); Serial.println(max_1.temperature(RNOMINAL, RREF)); // temp readout in Celsius
  // Serial.print("RTD Temperature 2 = "); Serial.println(max_2.temperature(RNOMINAL, RREF)); // temp readout in Celsius
  float RTD_temp1 = max_1.temperature(RNOMINAL, RREF);
  float RTD_temp2 = max_2.temperature(RNOMINAL, RREF);
  // float RTD_temp3 = max_3.temperature(RNOMINAL, RREF);
  float avg_temp = (RTD_temp1 + RTD_temp2)/2;

  float targetspeed = Output;
  float percent_power = (targetspeed/800)*100;

  
  // analogWrite(PIN_OUTPUT, Output);

  // Control motor speed based on PID output
  mc.setSpeedNow(1, Output);
  // mc.setSpeedNow(2, Output);
  // mc.setSpeedNow(3, Output);

  //   // Check and print any faults for sensor 1
  // uint8_t fault1 = max_1.readFault();
  // if (fault1) {
  //   Serial.print("Fault 1: 0x"); Serial.println(fault1, HEX);
  //   if (fault1 & MAX31865_FAULT_HIGHTHRESH) Serial.println("RTD 1 High Threshold");
  //   if (fault1 & MAX31865_FAULT_LOWTHRESH) Serial.println("RTD 1 Low Threshold");
  //   if (fault1 & MAX31865_FAULT_REFINLOW) Serial.println("RTD 1: REFIN- > 0.85 x Bias");
  //   if (fault1 & MAX31865_FAULT_REFINHIGH) Serial.println("RTD 1: REFIN- < 0.85 x Bias - FORCE- open");
  //   if (fault1 & MAX31865_FAULT_RTDINLOW) Serial.println("RTD 1: RTDIN- < 0.85 x Bias - FORCE- open");
  //   if (fault1 & MAX31865_FAULT_OVUV) Serial.println("RTD 1: Under/Over voltage");
  //   max_1.clearFault();
  // }

  // // Check and print any faults for sensor 2
  // uint8_t fault2 = max_2.readFault();
  // if (fault2) {
  //   Serial.print("Fault 2: 0x"); Serial.println(fault2, HEX);
  //   if (fault2 & MAX31865_FAULT_HIGHTHRESH) Serial.println("RTD 2 High Threshold");
  //   if (fault2 & MAX31865_FAULT_LOWTHRESH) Serial.println("RTD 2 Low Threshold");
  //   if (fault2 & MAX31865_FAULT_REFINLOW) Serial.println("RTD 2: REFIN- > 0.85 x Bias");
  //   if (fault2 & MAX31865_FAULT_REFINHIGH) Serial.println("RTD 2: REFIN- < 0.85 x Bias - FORCE- open");
  //   if (fault2 & MAX31865_FAULT_RTDINLOW) Serial.println("RTD 2: RTDIN- < 0.85 x Bias - FORCE- open");
  //   if (fault2 & MAX31865_FAULT_OVUV) Serial.println("RTD 2: Under/Over voltage");
  //   max_2.clearFault();
  // }

  // Serial.println("--------------------------");

  // Just print the organized outputs I want as Time, Power Output, Temp1, Temp2, avgTemp
  Serial.print(elapsedTime);
  Serial.print(",");
  Serial.print(percent_power);
  Serial.print(",");
  Serial.print(RTD_temp1);
  Serial.print(",");
  Serial.print(RTD_temp2);
  Serial.print(",");
  Serial.println(avg_temp);

  delay(250);
}
