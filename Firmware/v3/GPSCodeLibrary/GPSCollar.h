
#ifndef GPSCollar_h
#define GPSCollar_h
#include "Arduino.h"

//needed for the library to function properly.

#include <SD.h>
#include <SPI.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <NMEAGPS.h>

//hardware configuration

#define _RXPin 4
#define _TXPin 3
#define _LED1 5
#define _LED2 6
#define _GPSpower 2
#define _SDCHIPSELECT 8
#define _GREENLED _LED1
#define _REDLED _LED2
#define _ARDUINOGPSRX 3 // GPS TX, Arduino RX pin
#define _ARDUINOGPSTX 4 // GPS RX, Arduino TX pin
#define _Second 1000


extern SoftwareSerial _SS;



class GPSCollar
{
	public:
		GPSCollar(File);//initialize the system, no interrupts
		
		void GPSPowerDown(); //shuts off the software serial and the gps
		void GPSPowerUp(); //boots both back up
		void Blink(int pin); // turns off the pin, then on, then off again.
		void WaitSleep(int MinutesToSleep); //uses a simple loop to wait an amount of time. Higher power consumption.
		void WaitSleep();//Overloaded to use the default short sleep.
		int CollarSD(); //returns the SD card pin number, allowing users access to the SD card in a readable manner.
		void WDTInit();//gets the WDT ready to use, does not enable system interrupts.  The user is required to do so by using the "Interrupts();" command.

		
		/*
			The low power option to sleeping the system.  Currently not 100% stable, and some odd issues have occured.
			If using this make sure to initialize the WTD, using the WTD initialize function, otherwise it may never
			wake up.
		*/
		void WDTSleep(int MinutesToSleep);  // Uses the watch dog timer to wait an amount of time. WDT needs to be initialized first.  
		void WDTSleep(); //Overloaded for ease of use. ALWAYS short sleeps.
		
		
		int DumpGPSInfo(double,double); //Not intended to be used, gets used by other functions.
		
		
		/*
			Load settings attempts to read from an SD card (already defined in the library) from a file called
			"settings.csv".  It returns 1 on success, but will not check for valid data.  It returns 0 if it cannot open
			a file named "settings.csv".  It will fail quitely here, and the user needs to create an event to happen
			in the event of a failure.
		*/
		int LoadSettings(); //Not in use yet.
		
		/*
			Checks to see if there is GPS data available.  If there is, it grabs it and puts it in the fix variable.
			It can be written with the DumpGPSInfo() option.
			
			It returns 0 on failure.
		*/
		int AttemptGPS();
		int WriteAverageGPS(int iterations,int ms);// Write the average to the SD card. iterations: number of times run ms: time between reads.
		int PrintGPS();//print without averaging.
		int BurnIn(int);//waits for either a set amount of time (return 0) or variation to go below thresh hold (return 1)
	private:	
		//In software configurables, changed by LoadSettings. 
		//May see less space used if these are #defined instead of created in this manner
		int _SHORTSLEEP=1;
		int _LONGSLEEP=8;
		int _BEGINNIGHT=25;
		int _ENDNIGHT=25;
		int _GPS_BAUD=9600;
		int _ENDMONTH=-1;
	    int _ENDDAY=-1;
		int _waitingForFix = 1;
		double _GPS_TIMEOUT   = 90; // 1.5 minutes
		double _GPS_TIME      = 0;
		int _turnGPSoff = 0;
		int _DESIREDHDOP = 2500;
		int _TIMEZONEADJUST =0;
		int _THRESHHOLD=.00001;
		
		
		//NMEAGPS *_GPS;
		//gps_fix *_fix;
		//NeoSWSerial *_gpsPort(int,int); //possible source of error here
		File _dataFile;
		TinyGPSPlus _GPS;
		
		
		//Hidden functions
		int NumFromSD(); // a hidden function that helps read our settings file.

};

#endif