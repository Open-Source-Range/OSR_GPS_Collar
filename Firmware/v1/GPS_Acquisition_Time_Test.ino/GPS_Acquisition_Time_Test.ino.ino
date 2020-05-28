#include <SD.h>
#include <SPI.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

// Set up the done pin for the Low Power breakout
const int DONEPIN = 5;

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
    doneSignal();
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
    doneSignal();
    while(true);
  }

  // Set up the time logging for checking the GPS
  unsigned long startTime = millis(), fixTime = 60000, bestTime = 0, endTime = 0;

  // Poll GPS unit, and iteraate either until a desirable HDOP or 1 minute has elapsed.
  String HDOP = "9999", bestHDOP = "9999";
  String GPSdata, bestGPS;

  Serial.println("GPS Initialized. Start polling...");
  int i = 1;
  while (endTime < 60000)
  {
  // Poll GPS unit, write result to SD card.
    while(ss.available()>0){gps.encode(ss.read());} // Read the GPS stream
    if(gps.location.isUpdated()) {  // If the position has updated, ...
      if (millis()-startTime<fixTime) {fixTime=millis()-startTime;}  // on first loop, means the GPS as acquired a fix. Set this as fixTime
      GPSdata = GPSline();  // Get the GPS info
      HDOP = getHDOPfromString(GPSdata);  // Extract the HDOP
      if (HDOP.toInt() <= bestHDOP.toInt()) {  // Hold the GPS fix with the best HDOP
        bestGPS = GPSdata;
        bestHDOP = HDOP;
        bestTime = millis()-startTime;
      }
      Serial.println("Run "+String(i)+": "+GPSdata+", Time: "+String(millis()-startTime)+", HDOP: "+HDOP);
      if (HDOP.toInt() < 250) {
        break;
      }
      // delay(1000);
      i++;          
    }
    endTime = millis()-startTime;   // set the time that the loop ended (either b/c time is up or HDOP criteria met).
  }
  
  //Write the GPS data to the SD card
  File dataFile = SD.open("gpslog.csv", FILE_WRITE);
  
  if (dataFile) {    //if the file is available, write to it:
    //Serial.println("Writting GPS data to SD card.");
    //Sebrial.println("Best HDOP: "+bestHDOP+", occurred at time: "+bestTime);
    //Serial.println("Best GPS fix: "+bestGPS);
    dataFile.println(bestGPS+","+String(fixTime)+","+String(bestTime)+","+String(endTime));
    dataFile.close();
  } else {     // print to the serial port too:
    //Serial.println("error opening SD card file.");
  }

  // Return signal to the AdaFruit low power board.
  Serial.println("Done.");
  doneSignal();
}


//////////////////////////////////////////////////////////////////////////////////////////////////
void loop() // This is empty b/c using the AdaFruit low power breakout board.
{
  
}

  
//////////////////////////////////////////////////////////////////////////////////////////////////
// Function to poll the GPS unit and get the location, quality info, and date/time.
String GPSline()
{
  String gpsLat = "", gpsLon = "";
  String sats = "", HDOP = "";
  String gpsdate = "", gpstime = "";
  
  // Get GPS location fields or set NoData if invalid.
  if (gps.location.isValid())
  {
    gpsLat = String(gps.location.lat(),6);
    gpsLon = String(gps.location.lng(),6);
  } else {
    gpsLat = 9999.0;
    gpsLon = 9999.0;
  }

  // Get GPS signal quality info
  if (gps.satellites.isValid())
  {
    sats = gps.satellites.value();
    HDOP = gps.hdop.value();
  } else {
    sats = 9999;
    HDOP = 9999;
  }

  // Get GPS date and time
  if (gps.date.isValid())
  {
    gpsdate = gps.date.value();
    gpstime = gps.time.value();
  } else {
    gpsdate = 9999;
    gpstime = 9999;
  }

  // return the full GPS line
  return HDOP+","+gpsLat+","+gpsLon+","+sats+","+gpsdate+","+gpstime;
}

/////////////////////////////////////////////////////////////////////////////////////
// Function to return the HDOP value from the GPSdata string.
String getHDOPfromString(String GPSstring){
  String HDOP = "9999";
  int pos = GPSstring.indexOf(",");
  HDOP = GPSstring.substring(0,pos);
  return HDOP;
}

/////////////////////////////////////////////////////////////////////////////////////
// Function to send the done signal to the low power board.
void doneSignal() {
  digitalWrite(DONEPIN, HIGH);
  delay(1);
  digitalWrite(DONEPIN, LOW);
  delay(1);
}
