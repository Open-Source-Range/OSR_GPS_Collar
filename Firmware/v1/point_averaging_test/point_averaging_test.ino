#include <SD.h>
#include <SPI.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

// Set up the done pin for the Low Power breakout
const int DONEPIN = 5;

// Set maximum runtime before giving up (milliseconds)
unsigned long maxTime = 35000;

// Set up GPS pins and baud rate
static const int RXPin = 4, TXPin = 3;
static const uint32_t GPSBaud = 9600;

// Initialize the TinyGPS++ object and serial connection to the GPS device
TinyGPSPlus gps;
SoftwareSerial ss(RXPin, TXPin);

// Set up the SD card module
const int chipSelect = 8;


//////////////////////////////////////////////////////////////////////////////////////////////////
void setup()
{
  // Set up some logging to the serial monitor.
  Serial.begin(115200);

  // Initialize low power done pin
  pinMode(DONEPIN, OUTPUT);
  digitalWrite(DONEPIN, LOW);

  // Initialize SD card
  //Serial.print("Initializing SD card...");
  if (!SD.begin(chipSelect)) {     // see if the card is present and can be initialized:
    //Serial.println("Card failed, or not present");
    return;      // don't do anything more:
  }
  //Serial.println("card initialized.");
  
  // Initialize & Check GPS
  //Serial.println("Initializing GPS...");
  ss.begin(GPSBaud);
  // Check for GPS connected.
  if (millis() > 5000 && gps.charsProcessed() < 10)
  {
    //Serial.println(F("No GPS detected: check wiring."));
    while(true);
  }

  
}


//////////////////////////////////////////////////////////////////////////////////////////////////
void loop() // This is empty b/c using the AdaFruit low power breakout board.
{
  String HDOP, sats, gpsdate, gpstime, GPSdata;
  float gpsLat, gpsLon;
  float totalLat = 0.0, totalLon = 0.0;
  int counter = 0;

  Serial.println("GPS Initialized. Start polling...");

  // Poll GPS unit, write result to SD card.
  while(counter<20){
    
    while(ss.available()>0){
      if(gps.encode(ss.read())){  // Read the GPS stream
        
        // Get Lat/Lon
        if (gps.location.isUpdated()) {
            if (gps.location.isValid())
              {
                Serial.println(String(gps.location.lat(),6));
                totalLat = totalLat + gps.location.lat();
                totalLon = totalLon + gps.location.lng();
                counter += 1;
                if (gps.satellites.isValid())
                  {
                    sats = gps.satellites.value();
                    HDOP = gps.hdop.value();
                  }
                if (gps.date.isValid())
                  {
                    gpsdate = gps.date.value();
                    gpstime = gps.time.value();
                  } 
              }          
        }
      
      }
    }
      
    //delay(1000);
    }

      gpsLat = totalLat / counter;
      gpsLon = totalLon / counter;
      
      GPSdata = String(gpsLat,6)+","+String(gpsLon,6)+","+HDOP+","+sats+","+gpsdate+","+gpstime+","+String(counter)+","+String(totalLat,6)+","+String(totalLon,6);
      Serial.println(GPSdata);
    
  
    //Write the GPS data to the SD card
    File dataFile = SD.open("gpslog.csv", FILE_WRITE);
  
    if (dataFile) {    //if the file is available, write to it:
      Serial.println("Writting GPS data to SD card.");
      dataFile.println(GPSdata);
      dataFile.close();
    } else {     // print to the serial port too:
      Serial.println("error opening SD card file.");
    }
  
    // Return signal to the AdaFruit low power board.
    Serial.println("Done.");
    delay(1000*60*5); //delay 5 min.
}

  

