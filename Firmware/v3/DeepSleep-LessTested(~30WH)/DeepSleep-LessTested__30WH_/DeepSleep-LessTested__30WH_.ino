/*
 * Finalized gps code for board version 3.
 * 
 * 
 */
//Included Libraries
#include <NMEAGPS.h>
#include <SD.h>
#include <SPI.h>
#include <avr/wdt.h> 
#include <avr/interrupt.h>      
#include <avr/sleep.h>          
#include <avr/power.h>      
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
void DeepSleep(int MinutesToSleep);
int printGPSInfo(int);
void wdtInitialize();

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

//Interrupts
ISR (WDT_vect)
{
 //We don't actually do anything other than reset the WDT here. We just use this to wake up. 
 wdt_reset();
}

void setup() {
  SystemInitialize();//required - also calls load settings
  wdtInitialize();//required for deep sleep
  digitalWrite(GPSpower,HIGH);
  gpsPort.begin(GPS_BAUD);
}


void loop() {
  
  // Is a GPS fix available?*******************
  if (GPS.available( gpsPort )) 
  {
    fix = GPS.read();
    Blink(GREENLED); //serves as a heartbeat

    if (fix.valid.location&&fix.valid.date&&fix.valid.time)//is gps valid?
    {
      digitalWrite(GPSpower, LOW);
      turnGPSoff    = 1;
      printGPSInfo(1);//Attempt to print
    }
    else
    {
      Blink(REDLED);
    }
   
  }


  
  // Have we waited too long for a GPS fix?
  if (millis() - GPS_TIME > GPS_TIMEOUT)
  {
    turnGPSoff    = 1;
    digitalWrite(GPSpower, LOW);
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
      DeepSleep(60*LONGSLEEP);
    }
    else
    {
      DeepSleep(SHORTSLEEP);
    }
    digitalWrite(GPSpower,HIGH);
    Blink(GREENLED);
     
    turnGPSoff    = 0;
    GPS_TIME=millis();
  }
} // loop


int printGPSInfo(int TimeOut)
{
    int maxtries=15;
    SD.begin(SDCHIPSELECT);
    dataFile = SD.open("gpslog.csv", FILE_WRITE); //open SD
    while(!dataFile&&maxtries>0)
    {
      maxtries--;
      dataFile.flush();
      SD.end();
      delay(2000);
      SD.begin(SDCHIPSELECT);
      dataFile = SD.open("gpslog.csv", FILE_WRITE);
    }
    if (dataFile)
    {
      Blink(GREENLED); 
      
      dataFile.print(TimeOut);
      dataFile.print(",");
      dataFile.print(fix.valid.location);
      dataFile.print(",");
      dataFile.print(fix.valid.date);
      dataFile.print(",");
      dataFile.print(fix.valid.time);
      dataFile.print(",");
      dataFile.print(fix.dateTime.year);
      dataFile.print(",");
      dataFile.print(fix.dateTime.month);
      dataFile.print(",");
      dataFile.print(fix.dateTime.date);
      dataFile.print(",");
      dataFile.print(fix.dateTime.hours);
      dataFile.print(",");
      dataFile.print(fix.dateTime.minutes);
      dataFile.print(",");
      dataFile.print(fix.dateTime.seconds);
      dataFile.print(",");
      dataFile.print(fix.valid.satellites);
      dataFile.print(",");
      dataFile.print(fix.latitude(),10);dataFile.print(",");
      dataFile.println(fix.longitude(),10);
      
      dataFile.close(); 
      SD.end();
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
      analogWrite(REDLED,j);
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
    Blink(GREENLED);//1
  }
  LoadSettings();
  digitalWrite(GREENLED,HIGH);
  delay(1000);
  digitalWrite(GREENLED,LOW);
  SD.end();
  
}

void wdtInitialize()
{
  cli();//disable interrupts
  MCUSR = 0; //reset status register
  WDTCSR |= 0b00011000; // using an OR leaves most bits unchanged while setting WDCE and WDE for config mode. See datasheet for details on those.
  WDTCSR =  0b01000000 | 0b100001; //WDIE - enable interrupt, WDE - disable reset on interrupt. the right half sets the delay to 8 seconds.
  sei();//enable interrupts
}

void LoadSettings()
{
  int maxtries=5;
  
  dataFile= SD.open("settings.csv", FILE_READ);
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
