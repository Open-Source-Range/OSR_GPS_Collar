#include <NMEAGPS.h>
#include <SD.h>
#include <SPI.h>
#include <avr/wdt.h>
#include <avr/sleep.h>

#define LED_PIN (5)
#define RXPin (4)
#define TXPin (3)
#define LED1 (5)
#define LED2 (6)
#define GPSpower (2)
#define SDCHIPSELECT (8)
#define GREENLED (LED1)
#define REDLED (LED2)
#include <NeoSWSerial.h>
#define ARDUINO_GPS_RX 3 // GPS TX, Arduino RX pin
#define ARDUINO_GPS_TX 4 // GPS RX, Arduino TX pin
#define Second 1000

void LoadSettings();
void Blink(int pin);
void SystemInitialize();
void Sleeping(int MinutesToSleep);
int printGPSInfo();

NMEAGPS GPS;
gps_fix fix;
NeoSWSerial gpsPort(ARDUINO_GPS_TX, ARDUINO_GPS_RX);
      int SHORTSLEEP=1;
      int LONGSLEEP=8;
      uint8_t BEGINNIGHT=2;
      int GPS_BAUD=9600;
      char ENDMONTH="-1";
      char ENDDAY="-1";
      int  waitingForFix = 1;
const unsigned long GPS_TIMEOUT   = 120000; // 2 minutes
      unsigned long GPS_TIME      = 0;
      int turnGPSoff = 0;
      File dataFile;
ISR (WDT_vect)
{
  MCUSR = 0;
}
void setup() {
  SystemInitialize();
  //LoadSettings();
  digitalWrite(GPSpower,HIGH);
  gpsPort.begin(GPS_BAUD);
  Blink(REDLED);
  Blink(GREENLED);
}


void loop() {
  digitalWrite(GREENLED,!digitalRead(GREENLED)); //heartbeat
  digitalWrite(REDLED,!digitalRead(REDLED)); //heartbeat  
  // Is a GPS fix available?*******************
  if (GPS.available( gpsPort )) 
  {
    digitalWrite(REDLED,LOW);
    digitalWrite(GREENLED,LOW);
    fix = GPS.read();
    Blink(GREENLED);
    if (waitingForFix) 
    {
      if (fix.valid.location&&fix.valid.date&&fix.valid.time)
      {
        Blink(GREENLED);
        printGPSInfo();//Attempt to print
        waitingForFix = 0;
        turnGPSoff    = 1;
      }
      else
      {
        Blink(REDLED);
      }
    }
  }
  //***************************************

  
  // Have we waited too long for a GPS fix?
  if (waitingForFix && (millis() - GPS_TIME > GPS_TIMEOUT)) 
  {
    waitingForFix = 0;
    turnGPSoff    = 1;
    digitalWrite(REDLED,HIGH);
    delay(2000);
    digitalWrite(REDLED,LOW);
    dataFile = SD.open("gpslog.csv", FILE_WRITE); //open SD
    while(!dataFile)
    {
      SD.end();
      delay(200);
      SD.begin(SDCHIPSELECT);
      dataFile = SD.open("gpslog.csv", FILE_WRITE);
    }
    if (dataFile)
    {
    dataFile.println("0,0,0,0,0,0,0,0,0"); //print a blank
    dataFile.close();
    }   
 }

//******************************************
 
 //Sleep 
  if (turnGPSoff) 
  {
    gpsPort.end();
    digitalWrite(GPSpower, LOW);
    //digitalWrite(REDLED,HIGH);
    /*if(fix.dateTime.month==ENDMONTH) Removed for now, plan to re-add after testing confirms this is not the error
      if(fix.dateTime.day==ENDDAY)
      {
        while(1)
        {
          //sleep forever
          Blink(GREENLED);
          Blink(REDLED);
          Sleeping(1);
        }
      }*/
      
    if(fix.dateTime.hours==BEGINNIGHT)
    {
      Sleeping(60*LONGSLEEP);
    }
    else
    {
      Sleeping(SHORTSLEEP);
    }
    digitalWrite(GPSpower,HIGH);
    Blink(GREENLED);
    gpsPort.begin(GPS_BAUD); 
    waitingForFix = 1;
    turnGPSoff    = 0;
    GPS_TIME=millis();
  }


  //******************************
} // loop


int printGPSInfo()
{

    //Serial.println(fix.dateTime.hours);
    dataFile = SD.open("gpslog.csv", FILE_WRITE); //open SD
    while(!dataFile)
    {
      SD.end();
      delay(200);
      SD.begin(SDCHIPSELECT);
      dataFile = SD.open("gpslog.csv", FILE_WRITE);
      Blink(REDLED);
    }
    if (dataFile)
    {
      
      dataFile.print(String(fix.dateTime.year));
      dataFile.print(",");
      dataFile.print(String(fix.dateTime.month));
      dataFile.print(",");
      dataFile.print(String(fix.dateTime.date));
      dataFile.print(",");
      dataFile.print(String(fix.dateTime.hours));
      dataFile.print(",");
      dataFile.print(String(fix.dateTime.minutes));
      dataFile.print(",");
      dataFile.print(String(fix.dateTime.seconds));
      dataFile.print(",");
      dataFile.print(String(fix.valid.satellites));
      dataFile.print(",");
      dataFile.print(fix.latitude(),10);dataFile.print(",");
      dataFile.println(fix.longitude(),10);
      dataFile.close();
      return 1;
    }
}

void Sleeping(int MinutesToSleep)
{
  sei();
  for(int sec=0,i=MinutesToSleep;i>0;sec+=8) //Actual waiting happens here
  {
    MCUSR = 0;
    noInterrupts();
    sleep_enable();
    interrupts();
    sleep_cpu();
    sleep_disable();
    if(sec>=60)
    {
      i--;
      sec=0;
    }
    
  }
  cli();
}

void Blink(int pin)
{
  digitalWrite(pin,LOW);
  delay(100);
  digitalWrite(pin,HIGH);
  delay(100);
  digitalWrite(pin,LOW);


}

void SystemInitialize()
{
  pinMode(GPSpower, OUTPUT);
  pinMode(REDLED,OUTPUT);
  pinMode(GREENLED,OUTPUT);
  if (!SD.begin(SDCHIPSELECT)) // see if the card is present and can be initialized, also sets the object to hold onto that chip select pin
  {  
    SD.end();
    while(1)
    {
      Blink(REDLED);
    }
  }
  ADCSRA = 0;
  /*** Setup the WDT ***/
  cli(); //disable interrupts
  wdt_reset(); //reset wdt 
  WDTCSR = (1<<WDCE)|(1<<WDE); //enable wdt interrupt
  WDTCSR = bit (WDIE) | bit (WDP3) | bit (WDP0);
  //sei();//allow interrupts.
  //interrupts will be allowed during sleep, but disabled elsewhere to avoid clashing
  sleep_disable();
  set_sleep_mode (SLEEP_MODE_PWR_DOWN);
}

void LoadSettings()
{
  if (!SD.begin(SDCHIPSELECT)) // see if the card is present and can be initialized, also sets the object to hold onto that chip select pin
  {  
    //Serial.println("SD fail"); //Tell PC, can be commented out
    //Serial.println("Halting...");
    digitalWrite(GREENLED,LOW); 
    digitalWrite(REDLED,HIGH); 
    SD.end();
    while(1);
  }
  if(dataFile = SD.open("settings.csv", FILE_READ))
  {
  //Serial.println("Opening settings");

    //Serial.println(F("Terminal for GPS Collar MS-R1"));
    //Serial.println(F("Created 12/19/2018"));
    //Serial.println(F("Last code update on 1/16/2018"));
    //Serial.println(F("Beginning Startup..."));
    //Serial.println(F("SD Card Detected"));
    //TIMEZONEADJ=NumFromSD();
    //Serial.print("Time Zone: ");
    //Serial.println(TIMEZONEADJ);
    SHORTSLEEP=NumFromSD();
    //Serial.print("Minute Sleep: ");
    //Serial.println(SHORTSLEEP);
    LONGSLEEP=NumFromSD();
    //Serial.print("Hour Sleep: ");
    //Serial.println(LONGSLEEP);
    BEGINNIGHT=NumFromSD();
    //Serial.print("Night (24-hour): ");
    //Serial.println(BEGINNIGHT);
    //Serial.print("Day (24-hour): ");
    //Serial.println(ENDNIGHT);
    //DESIREDHDOP=NumFromSD();
    //Serial.print("HDOP: ");
    //Serial.println(DESIREDHDOP);
    GPS_BAUD=NumFromSD();
    //Serial.print("GPS baud rate: ");
    //Serial.println(GPSBaud);
    //Serialprinting=NumFromSD();
    ENDMONTH=NumFromSD();
    //Serial.print("End Month: ");
    ENDDAY=NumFromSD();
  }
  else
  {
    //Serial.println("Settings not found, using default.");
  }
    dataFile.close();
  
}

int NumFromSD()
{
  int ToReturn=0;
  int buf;
  int num[4];
  int i=0,neg=0,j=0;
  while(dataFile.read()!=','); //read until ,
  do//begin extracting numbers
  {
    buf=dataFile.read(); //get char
    if(buf!=13&&buf!='-'&&buf>=48&&buf<=57)
    {
    buf-=48; //convert char number to int number
    num[i]=buf; //store in array
      while(j<i)
      {
        num[j]=num[j]*10;
        j++;
      }
      j=0;
      i++;
    }
    else if(buf=='-')
    {
      neg=1;
    }
  } while(buf!=13);
  while(i>j)
  {
    ToReturn=ToReturn+num[j];
    j++;
  }
  if(neg)
    {
      ToReturn=ToReturn*-1;
    }
  return(ToReturn);
}
