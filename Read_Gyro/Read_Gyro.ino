/*
 *  Name: Read Gyro
 *  Description:
 *    The purpose of this code is to test the single
 *    functionality of collecting a gyro snapshot
 *    The code will collect the data and then print in 
 *    on the serial monitor
 *  Author: Liam Ballesty
 *  Date: 8/2/2017
 */

#include "SLOOD.h"

#include <stdio.h>

#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"

#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
#include <Wire.h>
#endif

#include "SDFileManager.h"

#define DEBUG_ON 1

// FUTURE: MOVE THESE DEFINES TO SLOOD.H
#define TCAADDR 0x70
#define INTERRUPT_PIN 3

// FUTURE: MOVE THESE DEFINES TO SLOOD.H
#define MPU_PORT_NUMBER 7

// MOVE THESE INTO THE MPU_6050 Library/.begin Method
bool dmpReady = false;  // set true if DMP init was successful
uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
uint16_t fifoCount;     // count of all bytes currently in FIFO
uint8_t fifoBuffer[64]; // FIFO storage buffer

Quaternion q;           // [w, x, y, z]         quaternion container
VectorInt16 aa;         // [x, y, z]            accel sensor measurements
VectorInt16 aaReal;     // [x, y, z]            gravity-free accel sensor measurements
VectorInt16 aaWorld;    // [x, y, z]            world-frame accel sensor measurements
VectorFloat gravity;    // [x, y, z]            gravity vector
float euler[3];         // [psi, theta, phi]    Euler angle container
float ypr[3];           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector

// Declare MPU6050 Object for obtaining Gyro, Accel, Temp data
MPU6050 mpu(0x68);


// The intermessage delay between DUE and MEGA platforms
#define INTERMESSAGE_DELAY_MSEC 100

volatile bool mpuInterrupt = false;     // indicates whether MPU interrupt pin has gone high
void dmpDataReady() {
  mpuInterrupt = true;
}


void setup() {
  CheckForI2CMultiplexer(7);

  // join I2C bus (I2Cdev library doesn't do this automatically)
  Wire.begin();

  Serial1.begin(BOARD_TO_BOARD_SERIAL);  //Communicate through Rx3 and Tx3 on the arduinos
  Serial.begin(BOARD_TO_PC_SERIAL);     // set up Serial library at 9600 bps

  Serial.println(F("Initializing I2C devices..."));
  mpu.initialize();
  pinMode(INTERRUPT_PIN, INPUT);

  Serial.println(F("Testing device connections..."));
  Serial.println(mpu.testConnection() ? F("MPU6050 connection successful") : F("MPU6050 connection failed"));

  Serial.println(F("Initializing DMP..."));
  devStatus = mpu.dmpInitialize();

  // make sure it worked (returns 0 if so)
  if (devStatus == 0)
  {
    // turn on the DMP, now that it's ready
    Serial.println(F("Enabling DMP..."));
    mpu.setDMPEnabled(true);

    // enable Arduino interrupt detection
    Serial.println(F("Enabling interrupt detection (Arduino external interrupt 0)..."));
    attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), dmpDataReady, RISING);
    mpuIntStatus = mpu.getIntStatus();

    // set our DMP Ready flag so the main loop() function knows it's okay to use it
    Serial.println(F("DMP ready! Waiting for first interrupt..."));
    dmpReady = true;

    // get expected DMP packet size for later comparison
    packetSize = mpu.dmpGetFIFOPacketSize();
  }
  else
  {
    // ERROR!
    // 1 = initial memory load failed
    // 2 = DMP configuration updates failed
    // (if it's going to break, usually the code will be 1)
    Serial.print(F("DMP Initialization failed (code "));
    Serial.print(devStatus);
    Serial.println(F(")"));
  }


#if (0)
  // FUTURE: Move this get rate experiment into a single, separate Test Example.
  Serial.print("MPU6050 Output Rate: ");
  Serial.println(mpu.getRate());
  //mpu.setRate(8);
  //Serial.print("New rate: ");
  //Serial.println(mpu.getRate());
#endif

  // FUTURE: Move Wait For Stable Gyro into MPU6050 Library.
  Serial.println("Waiting for gyro to stabilize...");
  if (waitForStableGyro(.01, 20, 30))
    Serial.println("Gyro ready.");
  else
    Serial.println("Gyro is not accurate.");
}


void loop() {
    readMPU6050Data();
}




int GetMuxPortNum(unsigned short int *MuxPortPtr)
{
  int ReturnVal = false;

  *MuxPortPtr = MPU_PORT_NUMBER;
  ReturnVal = true;

  return (ReturnVal);
}

int CheckForI2CMultiplexer(unsigned short int MuxPortNum)
{
  int ReturnValue = false;
  unsigned short int PortPosition = 1;

  // Set the binary Port Position bit for enabling a Port (example: 0b00000100 or 0b00100000)
  PortPosition = PortPosition << MuxPortNum; // HARD SET to 128 (binary 1000 0000)

  // Enable I2C Port on DUE with Wire call
  Wire.begin();

  // Trasmit to I2C Address assigned to Mux
  Wire.beginTransmission(TCAADDR);

  Wire.write(PortPosition);

  ReturnValue = Wire.endTransmission(true);

  // Close I2C bus
  Wire.end();

#if (DEBUG_ON)
  Serial.print("CheckForI2CMultiplexer(). ReturnValue from Wire.endTransmission: ");
  Serial.println(ReturnValue);
#endif

  if (ReturnValue == 0)
    return (true);
  else
    return (false);
}

float gx, gy, gz;
int ax, ay, az;


boolean readMPU6050Data()
{
  if (!dmpReady) return;

  while (!mpuInterrupt && fifoCount < packetSize) {
    Serial.println("stuck in loop: getTempReadings()");
  }

  mpuInterrupt = false;
  mpuIntStatus = mpu.getIntStatus();

  fifoCount = mpu.getFIFOCount();

  if ((mpuIntStatus & 0x10) || fifoCount == 1024)
  {
    // reset so we can continue cleanly
    mpu.resetFIFO();
    Serial.println(F("FIFO overflow!"));
  }
  else if (mpuIntStatus & 0x02)
  {
    // wait for correct available data length, should be a VERY short wait
    while (fifoCount < packetSize) fifoCount = mpu.getFIFOCount();

    // read a packet from FIFO
    mpu.getFIFOBytes(fifoBuffer, packetSize);

    // track FIFO count here in case there is > 1 packet available
    // (this lets us immediately read more without waiting for an interrupt)
    fifoCount -= packetSize;

    // display Euler angles in degrees
    mpu.dmpGetQuaternion(&q, fifoBuffer);
    mpu.dmpGetGravity(&gravity, &q);
    mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);

    // calculate Relative Degrees of Orientation
    gx = (ypr[0] * 180 / M_PI);
    gy = (ypr[1] * 180 / M_PI);
    gz = (ypr[2] * 180 / M_PI);

#if (DEBUG_ON)
    Serial.print("ypr\t");
    Serial.print(gx);
    Serial.print("\t");
    Serial.print(gy);
    Serial.print("\t");
    Serial.println(gz);
#endif

  }
  return false;
}

boolean waitForStableGyro(float accuracy, int samples, int maxTime)
{
  float readings[samples], mean = 0.0, currentGyX = 0.0, upperLimit = 0.0, lowerLimit = 0.0; //FUTURE: validate size of readings array
  boolean returnVal = false;
  int startingTime = millis(), counter = 0;
  int i;

  while (millis() - startingTime < (maxTime * 1000))
  {
    //Collect x number of samples and store in the array.
    //Only tries to collect data if an interrupt occurred on the MPU6050.
    //FUTURE: redesign to limit time in for loop
    for (i = 0; i < samples;)
    {
      if (mpuInterrupt)
      {
        readMPU6050Data();
        readings[i] = abs(gx);
        mean += abs(gx);
        i++;
      }
    }

    mean /= (float)samples;
    upperLimit = mean + accuracy;
    lowerLimit = mean - accuracy;

    counter = 0;
    for (i = 0; i < samples; i++)
    {
      currentGyX = readings[i];
      if (currentGyX != 0.00 && ((currentGyX <= upperLimit) && (currentGyX >= lowerLimit)))
      {
        counter++;
      }
    }

    if (counter >= samples)
      returnVal = true;

    mean = 0.0;

    if (returnVal) break;
  }

  return returnVal;
}
//End all


