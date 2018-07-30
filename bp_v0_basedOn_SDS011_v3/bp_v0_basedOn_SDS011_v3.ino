#include <TinyGPS++.h>
#include <SPI.h>
#include <SD.h>
#include <sensorSDS011.h>
//#include <Wire.h>

#define LED_BUILTIN PB12
#define CSPIN PA4

// The TinyGPS++ object
TinyGPSPlus gps;
File myFile;
String filename;
const int MPU_addr = 0x68; // I2C address of the MPU-6050
double ref[7], acc[7]; //AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;
uint32_t mtime;

float p10, p25;
bool status;
int counter = 0, i_acc = 0, n_cali = 2000;
SDS021 nova;
uint8_t fwdate[3], workmode, asleep;
int ret, gpsUpdated = 0;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  nova.begin(&Serial1);
  nova.setDebug ( false );

  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  Serial2.begin(9600); // GPS baudrate is 9600
  delay(50);
  Serial.println("Initializing SD card...");

  if (!SD.begin(CSPIN)) {
    Serial.println("Initialization failed!");
//    while (1);
  }
  Serial.println("Initialization done.");

  for (int i = 0; i < 7; i++) ref[i] = 0.0;
//  Serial.println(F("Calibrating accelerometer..."));
//  Wire.begin();
//  Wire.beginTransmission(MPU_addr);
//  Wire.write(0x6B);  // PWR_MGMT_1 register
//  Wire.write(0);     // set to zero (wakes up the MPU-6050)
//  Wire.endTransmission(true);
//  averageAcc(n_cali, true); // calibrate the accelerometer
//  mtime = millis();
//  Serial.println(F("Calibration done."));
//  for (int i = 0; i < 7; i++) {
//    Serial.print(ref[i]); Serial.print(",");
//  }
//  Serial.println(n_cali);

  ///////////////Nova init begin
  nova.sleepWork(&asleep, SDS021_WORKING, SDS021_SET );
  // If module is sleeping, then do not expect an answer. Better ask again after a short delay.
  delay(10);

  nova.sleepWork(&asleep, SDS021_WORKING, SDS021_ASK );
  if ( asleep != 0xFF ) {
    Serial.println(""); Serial.print( "Module set to "); Serial.println( asleep == SDS021_SLEEPING ? "Sleep" : "Work" );
  }

  nova.firmwareVersion( fwdate );
  if ( fwdate[0] != 0xFF ) {
    Serial.print( "fwdate = 20"); Serial.print( fwdate[0]); Serial.print( " month "); Serial.print( fwdate[1]); Serial.print("day "); Serial.println( fwdate[2] );
  }

  nova.workMode( &workmode, SDS021_QUERYMODE, SDS021_SET);
  Serial.println("Mode set");
  if ( workmode != 0xFF ) {
    Serial.print( "workmode set to "); Serial.println( workmode == SDS021_REPORTMODE ? "Reportmode" : "Querymode" );
  }
  ///////////////Nova init end
  
  if (Serial2.available() > 0) Serial.println(F("GPS communication enabled."));
  else Serial.println(F("GPS not available."));

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  filename = getNextName() + ".csv";
  digitalWrite(LED_BUILTIN, LOW);
  myFile = SD.open(filename, FILE_WRITE);

  // if the file opened okay, write to it:
  if (myFile) {
    Serial.print(F("Writing to "));
    Serial.println(filename);
    myFile.print("Acc_ref");
    myFile.print(","); myFile.print(mtime);
    for (int i = 0; i < 7; i++) {
      myFile.print(","); myFile.print(ref[i]);
    }
    myFile.print(","); myFile.print(n_cali);
    myFile.println();
    // close the file:
    myFile.close();
    Serial.println(F("Done."));
  } else {
    // if the file didn't open, print an error:
    Serial.println("Error while writing SD");
  }
  digitalWrite(LED_BUILTIN, HIGH);
  mtime = millis();
}

void loop() {
//  readAcc();
//  Serial.println("Loop........");
  while (Serial2.available() > 0) {
    gps.encode(Serial2.read());
    if (millis() - mtime > 3000) {
      mtime = millis();
      if (gps.location.isUpdated()) gpsUpdated = 1;
  //    finishAcc();
      status = nova.queryData(&p10, &p25 ); // datasheet advises 3 seconds as minimum poll time.
      if (status) {
        digitalWrite(LED_BUILTIN, LOW);
        myFile = SD.open(filename, FILE_WRITE);
        if (myFile) {
          myFile.print(counter);
          myFile.print(","); myFile.print(mtime);
          myFile.print(","); myFile.print(gpsUpdated);
          myFile.print(","); myFile.print(formatDate());
          myFile.print(","); myFile.print(formatTime());
          myFile.print(","); myFile.print(gps.location.lat(), 6);
          myFile.print(","); myFile.print(gps.location.lng(), 6);
          myFile.print(","); myFile.print(gps.speed.kmph()); 
          myFile.print(","); myFile.print(gps.altitude.meters()); 
          myFile.print(","); myFile.print(gps.satellites.value());  
          for (int i = 0; i < 7; i++) {
            myFile.print(","); myFile.print(acc[i]);
          }
          myFile.print(","); myFile.print(i_acc);
          myFile.print(","); myFile.print( p10, 1 );
          myFile.print(","); myFile.println( p25, 1);
          myFile.close();
        }
        else {
          Serial.println("Error while writing SD");
        }
        digitalWrite(LED_BUILTIN, HIGH);
        Serial.print(counter);
        Serial.print(","); Serial.print(mtime);
        Serial.print(","); Serial.print(gpsUpdated);
        Serial.print(","); Serial.print(formatDate());
        Serial.print(","); Serial.print(formatTime());
        Serial.print(","); Serial.print(gps.location.lat(), 6);
        Serial.print(","); Serial.print(gps.location.lng(), 6);
        Serial.print(","); Serial.print(gps.speed.kmph()); 
        Serial.print(","); Serial.print(gps.altitude.meters()); 
        Serial.print(","); Serial.print(gps.satellites.value());  
        for (int i = 0; i < 7; i++) {
          Serial.print(","); Serial.print(acc[i]);
        }
        Serial.print(","); Serial.print(i_acc);
        Serial.print(","); Serial.print( p10, 1 );
        Serial.print(","); Serial.println( p25, 1);
      }
      else {
        Serial.println("Error while reading SDS011");
        p10 = p25 = 0;
      }
  
      mtime = millis();
      resetAcc();
      counter++;
      gpsUpdated = 0;
    }
  }
}

//void readAcc() {
//  Wire.beginTransmission(MPU_addr);
//  Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
//  Wire.endTransmission(false);
//  Wire.requestFrom(MPU_addr, 14, true); // request a total of 14 registers
//  acc[0]+=AcX=Wire.read()<<8|Wire.read();  // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)    
//  acc[1]+=AcY=Wire.read()<<8|Wire.read();  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
//  acc[2]+=AcZ=Wire.read()<<8|Wire.read();  // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
//  acc[3]+=Tmp=Wire.read()<<8|Wire.read();  // 0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
//  acc[4]+=GyX=Wire.read()<<8|Wire.read();  // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
//  acc[5]+=GyY=Wire.read()<<8|Wire.read();  // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
//  acc[6]+=GyZ=Wire.read()<<8|Wire.read();  // 0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)
//  i_acc++;
//}

//void finishAcc() {
//  for (int i = 0; i < 7; i++) 
//    if (i != 3) acc[i] = acc[i]/i_acc - ref[i];
//    else acc[i] = acc[i] / i_acc / 340.00 + 36.53;
//}

void resetAcc() {
  i_acc = 0;
  for (int i = 0; i < 7; i++) acc[i] = 0.0;
}

//void averageAcc(int counter, boolean calibrate) {
//  digitalWrite(LED_BUILTIN, LOW);
//  for (int i = 0; i < 7; i++) acc[i] = 0.0;
//  for (int i = 0; i < counter; i++) {
//    Wire.beginTransmission(MPU_addr);
//    Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
//    Wire.endTransmission(false);
//    Wire.requestFrom(MPU_addr, 14, true); // request a total of 14 registers
//    acc[0]+=AcX=Wire.read()<<8|Wire.read();  // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)    
//    acc[1]+=AcY=Wire.read()<<8|Wire.read();  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
//    acc[2]+=AcZ=Wire.read()<<8|Wire.read();  // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
//    acc[3]+=Tmp=Wire.read()<<8|Wire.read();  // 0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
//    acc[4]+=GyX=Wire.read()<<8|Wire.read();  // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
//    acc[5]+=GyY=Wire.read()<<8|Wire.read();  // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
//    acc[6]+=GyZ=Wire.read()<<8|Wire.read();  // 0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)
//  }
//  for (int i = 0; i < 7; i++)
//    if (i != 3) acc[i] = acc[i] / counter - ref[i];
//    else acc[3] = acc[3] / counter / 340.00 + 36.53;
//  if (calibrate) for (int i = 0; i<7; i++) ref[i] = acc[i];
//  else for (int i = 0; i<7; i++) if (i != 3) acc[i]-+ref[i]; // subtract ref for measurements other than temp
//  digitalWrite(LED_BUILTIN, HIGH);
//}

String encodeMsg28(String msg) {  // encodes a 28 digit number String into a 12 byte Ascii String
  if (msg.length() != 28) {
    String ret = "" + msg.length();
    return ret;
  }

  unsigned char bytes[3];
  String out = "";

  for (int j = 0; j < 4; j++)
  {
    String temp = "";
    for (int i = 0; i < 7; i++)
      temp += msg[j * 7 + i];
    unsigned long int t = temp.toInt();

    bytes[0] = (t >> 16);
    bytes[1] = (t >> 8);
    bytes[2] = t;
    //out += (unsigned char) bytes[0] + (unsigned char) bytes[1] + (unsigned char) bytes[2];
    out += (char) bytes[0];
    out += (char) bytes[1];
    out += (char) bytes[2];
  }
  return out;
}

String trimGPS(double lati) { // returns longitude/latitude trimmed to 5 relevant digits
  int temp = ((int)(lati*100000))%100000; // also works but might cause loss of precision: =(int)((lati-(int)lati)*100000);
  String out = (String)temp;
  temp = 5-out.length(); // fill up with leading zeroes if necessary
  for (int i=0; i<temp; i++) out="0"+out;
  return out;
}

String formatDate() {
  String out = "";
  out += gps.date.year();
  out += formatInt(gps.date.month(), 2);
  out += formatInt(gps.date.day(), 2);
  return out;
}

String formatTime() {
  String out = "";
  out += formatInt(gps.time.hour(), 2);
  out += formatInt(gps.time.minute(), 2);
  out += formatInt(gps.time.second(), 2);
  return out;
}

String formatInt(int num, int precision) {
  String maximum = "";
  for (int i = 0; i < precision; i++) maximum += "9";
  if (num > maximum.toInt()) return maximum;
  String out = String(num); // fill up with leading zeroes if necessary
  String zero = "";
  for (int i = 0; i < (precision - out.length()); i++) zero += "0";
  return (zero + out);
}

String getRandomName() { // generate a "random" filename using sensor input
  uint32_t ran = 0;
  for (int i = 0; i < 7; i++) ran += ref[i];
  while (ran > 99999) ran -= ref[0];
  String out = String(ran);
  return out;
}

String getNextName() { // check filenames on SD and increment by one
  File root = SD.open("/");
  int out = 10000;
  while (true) {
    File entry =  root.openNextFile();
    if (!entry) break;
    String temp = entry.name();
    temp = temp.substring(0 ,5);
    int num = temp.toInt();
    if (num < 99999 && num >= out) out = num + 1;
    entry.close();
  }
  return String(out);
}

