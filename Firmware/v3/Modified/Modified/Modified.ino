
//Libraries ***************************************
#include <TinyGPS++.h> //use with SoftwareSerial
#include <SoftwareSerial.h>
#include <SD.h>
#include <SPI.h>
#include <avr/wdt.h> 
#include <avr/interrupt.h>      
#include <avr/sleep.h>          
#include <avr/power.h>          


//Defines****************************************
#define LED_PIN (5)
#define RXPin (4)
#define TXPin (3)
#define LED1 (5)
#define LED2 (6)
#define GPSpower (2)
#define SDCHIPSELECT (8)
#define GREENLED (LED1)
#define REDLED (LED2)
static const int ARDUINO_GPS_RX = 4; // GPS TX, Arduino RX pin
static const int ARDUINO_GPS_TX = 3; // GPS RX, Arduino TX pin
#define Second 1000

//Prototpyes**************************************
void LoadSettings();
void Blink(int pin);
void SystemInitialize();
void Sleep(int MinutesToSleep);
void DeepSleep(int MinutesToSleep);
int printGPSInfoNEO(int);
int printGPSInfoTINY(int);
void wdtInitialize();


//Globals******************************************

TinyGPSPlus GPS;
SoftwareSerial gpsPort(ARDUINO_GPS_RX, ARDUINO_GPS_TX);

File dataFile;
unsigned long GPS_TIME= 0;
int turnGPSoff = 0;


//Global Settings*************************************
int SHORTSLEEP=1;  // in minutes
int LONGSLEEP=8;    //hours
int BEGINNIGHT=25;
int ENDNIGHT=25;
static const uint32_t GPS_BAUD=9600;//tinygps
int ENDMONTH=-1;
int ENDDAY=-1;
unsigned long GPS_TIMEOUT   = 60000; 

ISR (WDT_vect)
{
 //We don't actually do anything other than reset the WDT here. We just use this to wake up. 
 wdt_reset();
}
void setup() {
  SystemInitialize();
  //wdtInitialize();
  Blink(GREENLED);
  Blink(REDLED);
  Blink(GREENLED);
  digitalWrite(GPSpower,HIGH);
  gpsPort.begin(GPS_BAUD);
  GPS_TIME=millis();
}

void loop() {  
  //Encode stream
  while (gpsPort.available() > 0) //leaves if not available
    GPS.encode(gpsPort.read());//encodes
   
  if(GPS.time.isValid()&&GPS.location.isValid()&&GPS.date.isValid()) //checks to see if valid
  {
    Blink(GREENLED);//shows valid data
    printGPSInfoTINY(1);//Attempt to print
    turnGPSoff = 1;
  }
  else if ((millis() - GPS_TIME > GPS_TIMEOUT)) //have we waited too long?
  {
    turnGPSoff    = 1;
    Blink(REDLED);
    printGPSInfoTINY(-1);//Attempt to print
  }
  else
  {

  }
  
//******************************************
 
 //Sleep 
  if (turnGPSoff) 
  {
    gpsPort.end();
    digitalWrite(GPSpower, LOW);
    if(GPS.time.hour()==BEGINNIGHT)//tinyGPS++
    {
      Sleep(LONGSLEEP);
    }
    else
    {
      Sleep(SHORTSLEEP);
    }
    digitalWrite(GPSpower,HIGH);
    gpsPort.begin(GPS_BAUD); 
    turnGPSoff = 0;
    GPS_TIME=millis();
  }
} // loop


/***
 * printGPSInfoTINY
 * a basic print to SD function using the TinyGPS++ 
 * 
 * int TimeOut is just a number that gets printed to the very first cell on the line, meant as a debugging tool to say where the
 * function call happened.  Here a 1 is good and came from proper sleep. a -1 is bad and came from timeout.
 */
int printGPSInfoTINY(int TimeOut)
{
    int maxtries=10;
    //Serial.println(fix.dateTime.hours);
    dataFile = SD.open("gpslog.csv", FILE_WRITE); //open SD
    while(!dataFile&&maxtries>0)
    {
      maxtries--;
      SD.end();
      delay(200);
      SD.begin(SDCHIPSELECT);
      dataFile = SD.open("gpslog.csv", FILE_WRITE);
      //Blink(REDLED);
    }
    if (dataFile)
    {
      Blink(GREENLED); 
      dataFile.print(String(TimeOut));
      dataFile.print(",");
      dataFile.print(String(GPS.location.isValid()));
      dataFile.print(",");
      dataFile.print(String(GPS.date.isValid()));
      dataFile.print(",");
      dataFile.print(String(GPS.time.isValid()));
      dataFile.print(",");
      dataFile.print(String(GPS.date.year()));
      dataFile.print(",");
      dataFile.print(String(GPS.date.month()));
      dataFile.print(",");
      dataFile.print(String(GPS.date.day()));
      dataFile.print(",");
      dataFile.print(String(GPS.time.hour()));
      dataFile.print(",");
      dataFile.print(String(GPS.time.minute()));
      dataFile.print(",");
      dataFile.print(String(GPS.time.second()));
      dataFile.print(",");
      dataFile.print(GPS.location.lat(), 10);
      dataFile.print(",");
      dataFile.println(GPS.location.lng(),10);
      dataFile.close();

      return 1;
    }
  else
  {
    for(int j=0;j<255;j++)
    {
      analogWrite(REDLED,j);
      analogWrite(GREENLED,j);
      delay(5);
    }
    for(int j=255;j>0;j--)
    {
      analogWrite(GREENLED,j);
      delay(5);
    }
    return 0;
  }
}

void Sleep(int MinutesToSleep)
{
   for(int sec=0, minutes=0;minutes<MinutesToSleep;sec+=8) //Actual waiting happens here
    {
      //enterSleep();
      delay(8000);
      if(sec>=60)
      {
        minutes++;
        sec=0;
      }
    }
}


void DeepSleep(int MinutesToSleep)
{
  unsigned long reference;
  sei();//enable interrupts
  for(int sec=0, minutes=0;minutes<MinutesToSleep;sec+=8) //Actual waiting happens here
  {
    reference=millis();
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_mode();
    sleep_disable();
    sec+=(millis()-reference)/1000;
    while(sec>=60)
    {
      minutes++;
      sec-=60;
    }
  }
  cli();//disable interrupts
  power_all_enable();
  power_adc_disable();//We still don't need this but I think power all re-enables it. Need to go through and individually power on what's needed.
}

void Blink(int pin)
{
  digitalWrite(pin,LOW);
  delay(100);
  digitalWrite(pin,HIGH);
  delay(100);
  digitalWrite(pin,LOW);
  delay(100);

}

void SystemInitialize()
{
  pinMode(GPSpower, OUTPUT);
  pinMode(REDLED,OUTPUT);
  pinMode(GREENLED,OUTPUT);

  if (!SD.begin(SDCHIPSELECT)) // see if the card is present and can be initialized, also sets the object to hold onto that chip select pin
  {  
    //Serial.println("SD fail"); //Tell PC, can be commented out
    //Serial.println("Halting...");
    digitalWrite(GREENLED,LOW); 
    digitalWrite(REDLED,HIGH); 
    SD.end();
    while(1)
    {
      Blink(REDLED);
    }
  }
  else
  {
    
  }
  //LoadSettings();
  //power_adc_disable(); //disable, adc isn't needed here.  unsure how this works on non p units
}

void LoadSettings()
{
  int maxtries=5;
  dataFile= SD.open("sett0ings.csv", FILE_READ);
  while(!dataFile&&maxtries>0)
  {
    maxtries--;
    SD.end();
    delay(200);
    SD.begin(SDCHIPSELECT);
    dataFile= SD.open("settings.csv", FILE_READ);
  }
  if(dataFile)
  {
     Blink(GREENLED);//2
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
     Blink(GREENLED);
    //Serial.print("Minute Sleep: ");
    //Serial.println(SHORTSLEEP);
    LONGSLEEP=NumFromSD();
     Blink(GREENLED);
    //Serial.print("Hour Sleep: ");
    //Serial.println(LONGSLEEP);
    BEGINNIGHT=NumFromSD();
     Blink(GREENLED);
    //Serial.print("Night (24-hour): ");
    //Serial.println(BEGINNIGHT);
    ENDNIGHT=NumFromSD();
     Blink(GREENLED);
    //Serial.print("Day (24-hour): ");
    //Serial.println(ENDNIGHT);
    //DESIREDHDOP=NumFromSD();
    //Serial.print("HDOP: ");
    //Serial.println(DESIREDHDOP);
    //GPS_BAUD=NumFromSD();
    int buf=NumFromSD();
     Blink(GREENLED);
    //Serial.print("GPS baud rate: ");
    //Serial.println(GPSBaud);
    //Serialprinting=NumFromSD();
    ENDMONTH=NumFromSD();
     Blink(GREENLED);
    //Serial.print("End Month: ");
    ENDDAY=NumFromSD();
     Blink(GREENLED);
    GPS_TIMEOUT=NumFromSD();
    Blink(GREENLED);
  }
  else
  {
    //Serial.println("Settings not found, using default.");
    Blink(REDLED);
  }
    dataFile.close();
  
}

int NumFromSD()
{
  int ToReturn=0;
  int buf;
  int num[5];
  int i=0,neg=0,j=0;
  while(dataFile.read()!=','); //read until ,
  do//begin extracting numbers
  {
    buf=dataFile.read(); //get char
    if(buf>=48&&buf<=57)
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
  } while(buf!=10);
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

void wdtInitialize()
{
  cli();//disable interrupts
  MCUSR = 0; //reset status register
  WDTCSR |= 0b00011000; // using an OR leaves most bits unchanged while setting WDCE and WDE for config mode. See datasheet for details on those.
  WDTCSR =  0b01000000 | 0b100001; //WDIE - enable interrupt, WDE - disable reset on interrupt. the right half sets the delay to 8 seconds.
  sei();//enable interrupts
}
