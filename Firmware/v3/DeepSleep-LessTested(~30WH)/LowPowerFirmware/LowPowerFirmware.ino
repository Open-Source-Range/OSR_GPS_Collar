/*
 * Low power firmware for GPS tracking collar.
 * 
 * Device power on and does the following tasks:
 * 
 * -Creates the global objects and does everything in their constructors.
 * -Through SytemInitialize() all required pins are set to input/output as needed.
 * -Through SytemInitialize() the device checks for an SD card to save to, if none is present the device stops and waits for a power cycle. It blinks red here.
 * -Through SytemInitialize() the device checks the SD card for a settings file and attempts to read from it in a specific order. Every successful settings load blinks green. There can be no blanks in the settings.
 * -The device does a longer green light than usual to indicate it is running as it expected to and will continue.
 * -Through wdtInitialize() the watch dog timer is configured not to trigger system reset, and to wait as long as it can between interrupts (~8seconds)
 * -GPS is powered on, and the port for data is opened.
 * 
 * After this a loop happes of the following logic:
 * -Is there data in the GPS port? if so read it and blink to inform the user.
 *      -Is the data valid? If so print to the SD with a valid tag, turn off the GPS, and signal sleep/not waiting
 * 
 * -Are we still waiting for data? Have we waited too long? If so print a failure with whatever data we DO have blink red twice to indicate failure. Also turn off power.
 * 
 * 
 * -Are we ready to sleep? If so turn off GPS (to make sure, though it is likely redundant), then prep the system be clearing the interrupts that have triggered, and re-enabling them.
 *      -Set sleep mode and then go to sleep.
 *      -Wake on WDT Interrupt.
 *      -Compare how long device has slept to settings file / defaults.
 *      -Have we slept long enough?
 *            -Wake and reset flags to continue again
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
#define RXPin (3)
#define TXPin (4)
#define LED1 (5)
#define LED2 (6)
#define GPSpower (2)
#define SDCHIPSELECT (8)
#define GREENLED (LED1)
#define REDLED (LED2)
#define ARDUINO_GPS_RX 3 // These may get swapped between gps brands. Beitian has RX on 3 TX on 4
#define ARDUINO_GPS_TX 4 // 



//Prototypes
void LoadSettings();
void Blink(int pin);
void SystemInitialize();
void DeepSleep(int MinutesToSleep);
int printGPSInfo(int);
void wdtInitialize();

//objects
NMEAGPS GPS;
gps_fix fix;
NeoSWSerial gpsPort(ARDUINO_GPS_TX, ARDUINO_GPS_RX);
File dataFile;

//global settings
uint8_t SHORTSLEEP=3;  // in minutes
uint8_t LONGSLEEP=8;
uint8_t BEGINNIGHT=25;
int GPS_BAUD=9600;
uint8_t ENDMONTH=-1;
uint8_t ENDDAY=-1;
unsigned long GPS_TIMEOUT   = 60; // 1 minutes

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
  GPS_TIME=millis();
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
  if (millis() - GPS_TIME > (GPS_TIMEOUT*1000))
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
      dataFile.print(fix.satellites);
      dataFile.print(",");
      dataFile.print(fix.latitude(),10);
      dataFile.print(",");
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

void DeepSleep(int MinutesToSleep)
{
  unsigned long reference;
  sei();//enable interrupts
  for(int sec=0, minutes=0;minutes<MinutesToSleep;sec+=8) //Actual waiting happens here
  {
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_mode();
    sleep_disable();
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
