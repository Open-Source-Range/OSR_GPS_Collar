/*
 * Finalized gps code for board version 3.
 * 
 * 
 */
//Included Libraries
#include <NMEAGPS.h>
#include <SD.h>
#include <SPI.h>
#include <NeoSWSerial.h>

//Defined Pins
#define LED_PIN (5)
#define RXPin (4)
#define TXPin (3)
#define LED1 (5)
#define LED2 (6)
#define GPSpower (2)
#define SDCHIPSELECT (8)
#define GREENLED (LED1)
#define ARDUINO_GPS_RX 3 // GPS TX, Arduino RX pin
#define ARDUINO_GPS_TX 4 // GPS RX, Arduino TX pin
#define Second 1000
#define REDLED (LED2)

//Prototypes
void LoadSettings();
void Blink(int pin);
void SystemInitialize();
void Sleep(int MinutesToSleep);
int printGPSInfo(int);


//objects
NMEAGPS GPS;
gps_fix fix;
NeoSWSerial gpsPort(ARDUINO_GPS_TX, ARDUINO_GPS_RX);
File dataFile;

//global settings
uint8_t SHORTSLEEP=10;  // in minutes
uint8_t LONGSLEEP=8;
uint8_t BEGINNIGHT=25;
uint8_t ENDNIGHT=25;
int GPS_BAUD=9600;
uint8_t ENDMONTH=-1;
uint8_t ENDDAY=-1;
unsigned long GPS_TIMEOUT   = 60000; // 1 minutes

//changing globals
unsigned long GPS_TIME      = 0;
uint8_t turnGPSoff = 0;
uint8_t waitingForFix = 1;


void setup() {
  SystemInitialize();//required - also calls load settings
  digitalWrite(GPSpower,HIGH);
  gpsPort.begin(GPS_BAUD);
}


void loop() {
  
  // Is a GPS fix available?*******************
  if (GPS.available( gpsPort )) 
  {
    fix = GPS.read();
    Blink(GREENLED); //serves as a heartbeat
    if (waitingForFix) 
    {
        if (fix.valid.location&&fix.valid.date&&fix.valid.time)//is gps valid?
        {
          printGPSInfo(1);//Attempt to print
          waitingForFix = 0;
          turnGPSoff    = 1;
        }
        else
        {
          Blink(REDLED);
        }
    }
  }


  
  // Have we waited too long for a GPS fix?
  if (waitingForFix && (millis() - GPS_TIME > GPS_TIMEOUT)) 
  {
    waitingForFix = 0;
    turnGPSoff    = 1;
    Blink(REDLED);
    Blink(REDLED);
    printGPSInfo(-1);//Attempt to print a failure
  }

 
 //Sleep 
  if (turnGPSoff) 
  {
    digitalWrite(GPSpower, LOW);
    /*if((int)fix.dateTime.month==ENDMONTH) Removed for now, plan to re-add after testing confirms this is not the error
      if((int)fix.dateTime.day==ENDDAY)
      {
        while(1)
        {
          //sleep forever
          Blink(GREENLED);
          Blink(REDLED);
          Sleep(1);
        }
      }*/
      
    if(fix.dateTime.hours==BEGINNIGHT)
    {
      Sleep(60*LONGSLEEP);
    }
    else
    {
      Sleep(SHORTSLEEP);
    }
    digitalWrite(GPSpower,HIGH);
    Blink(GREENLED);
     
    waitingForFix = 1;
    turnGPSoff    = 0;
    GPS_TIME=millis();
  }
} // loop


int printGPSInfo(int TimeOut)
{
    int maxtries=100;
    dataFile = SD.open("gpslog.csv", FILE_WRITE); //open SD
    while(!dataFile&&maxtries>0)
    {
      maxtries--;
      SD.end();
      delay(200);
      SD.begin(SDCHIPSELECT);
      dataFile = SD.open("gpslog.csv", FILE_WRITE);
    }
    if (dataFile)
    {
      Blink(GREENLED); 
      dataFile.print(String(TimeOut));
      dataFile.print(",");
      dataFile.print(String(fix.valid.location));
      dataFile.print(",");
      dataFile.print(String(fix.valid.date));
      dataFile.print(",");
      dataFile.print(String(fix.valid.time));
      dataFile.print(",");
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
    Blink(GREENLED);//1
  }
  LoadSettings();
  digitalWrite(GREENLED,HIGH);
  delay(1000);
  digitalWrite(GREENLED,LOW);

  
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
    Blink(GREENLED);
    SHORTSLEEP=NumFromSD();
    Blink(GREENLED);
    LONGSLEEP=NumFromSD();
    Blink(GREENLED);
    BEGINNIGHT=NumFromSD();
    Blink(GREENLED);
    ENDNIGHT=NumFromSD();
    Blink(GREENLED);
    GPS_BAUD=NumFromSD();
    Blink(GREENLED);
    ENDMONTH=NumFromSD();
    Blink(GREENLED);
    ENDDAY=NumFromSD();
    Blink(GREENLED);
    GPS_TIMEOUT=NumFromSD();
    Blink(GREENLED);
  }
  else
  {
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
