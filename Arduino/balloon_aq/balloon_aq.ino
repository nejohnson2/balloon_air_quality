/*
  Groove Sensor:
  Pin 1 (Black Wire) > Arduino Gnd
  Pin 3 (Red wire) - Arduino +5v
  Pin 4 (Yellow wire) - Arduino Digital pin 2
  
  DHT 22 (Am2302)
  Black - Gnd
  Red - +5v
  Yellow - Arduino pin 3

  SD Card:
  MOSI - pin 11
  MISO - pin 12
  CLK  - pin 13
  CS   - pin 9

  I2C Device Addresses:
  RTC I2C Address = 0x68
  BMP180 I2C Address = 0x77

  MQ-4:
  Yellow Wire - 220K resisotr connected to Gnd and A0
  Red - +5v
  White - +5v
  Black - Gnd
*/

// include SD libraries
#include <SPI.h>
#include <SD.h>

#include "DHT.h" // DHT22 library
#include <Adafruit_BMP085.h>  // include Pressure sensor library

#define DHTPIN 3     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)

// include libraries
#include <Wire.h>
#include "RTClib.h"

// initialize library
RTC_DS1307 RTC;
Adafruit_BMP085 bmp;
DHT dht(DHTPIN, DHTTYPE);

const int chipSelect = 9; // chip to communicate with SD module

File dataFile;

int pin = 2;
unsigned long duration;
unsigned long starttime;
unsigned long sampletime_ms = 30000;//sampe 30s ;
unsigned long lowpulseoccupancy = 0;
float ratio = 0;
float concentration = 0;

int mq4;

void setup() {
  // set up the LCD's number of columns and rows:
  pinMode(6, INPUT);
  if(initSD()){
  }
  Serial.begin(9600);
  Wire.begin();
  RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));

  // Make sure the clock is running
  if (! RTC.isrunning()) {
    Serial.println("RTC NOT running");
  }  
    // Make sure the pressure sensor is running
  if (!bmp.begin()) {
    Serial.println("Could not find BMP085");
    while (1) {}
  }
  dht.begin();

  starttime = millis();
}

void loop() {
  DateTime now = RTC.now();

  duration = pulseIn(pin, LOW);
  lowpulseoccupancy = lowpulseoccupancy + duration;

  if ((millis() - starttime) > sampletime_ms) //if the sampel time == 30s
  {
    ratio = lowpulseoccupancy / (sampletime_ms * 10.0); // Integer percentage 0=>100
    concentration = 1.1 * pow(ratio, 3) - 3.8 * pow(ratio, 2) + 520 * ratio + 0.62; // using spec sheet curve
    String dataString = ""; // create blank string to write to SD
  
    dataString += lowpulseoccupancy;
    dataString += ",";
    dataString += ratio;
    dataString += ",";
    dataString += concentration;
    dataString += ",";
    dataString += now.month();
    dataString += "/";
    dataString += now.day();
    dataString += ",";

    dataString += now.hour();
    dataString += ":";    
    dataString += now.minute();    
    dataString += ":";
    dataString += now.second();
    dataString += ",";

    dataString += bmp.readTemperature(); // in celcius
    dataString += ",";
    
    // Calculate altitude assuming 'standard' barometric
    // pressure of 1013.25 millibar = 101325 Pascal
    dataString += bmp.readPressure();  
    dataString += ",";
    
    // This is assumed a 'standard' pressure of 1013.25
    dataString += bmp.readAltitude(); //meters
    dataString += ",";

    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature(); 
    // Compute heat index in Celsius (isFahreheit = false)
    float hic = dht.computeHeatIndex(t, h, false);   

    dataString += h;
    dataString += ",";
    dataString += t;
    dataString += ",";    
    dataString += hic;
    dataString += ",";
    
    // MQ4 Sensor
    mq4 = analogRead(0);
    dataString += mq4;
    
    Serial.println(dataString); // print to serial 
    dataFile.println(dataString);  // write to the SD
    dataFile.flush();   //

    lowpulseoccupancy = 0;
    starttime = millis();
  }
}
boolean initSD() {
  // this is the default slave select.  must be set to output
  pinMode(SS, OUTPUT); 

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    // don't do anything more:
    while (1) ;
  }
  // Open up the file we're going to log to!
  dataFile = SD.open("datafile.txt", FILE_WRITE);
  if (!dataFile) {
    // Wait forever since we cant write data
    while (1) ;
  }
  return true;
}
