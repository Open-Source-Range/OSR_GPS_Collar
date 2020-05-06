#include <GPSCollar.h>

/*
Extreme basic functionality of the GPSCollar Library.  This code just initializes to the defaults, 
checks the gps for valid data, dumps it to the SD card, then sleeps.
*/
//Create the File handle
File dataFile; 
SoftwareSerial _SS(4,3);
//Create the object and pass the file handle into it
GPSCollar collar(dataFile); 


void setup() 
  {
    collar.GPSPowerUp();
    collar.WDTInit();
    Serial.begin(9600);
    Serial.println("Starting...");

  }


void loop()
{
  if(collar.AttemptGPS())//Is the GPS valid ?
  {
    Serial.println("GPS Data Valid");
    if(collar.BurnIn(2000)) //time in mS until timeout
      Serial.println("Burn in success");
    else
    {
      Serial.println("Burn in timeout - - Values not stabilized");
    }
    if(collar.WriteAverageGPS(10,200))//Print its average with (number,mS between reads)
    {
      collar.GPSPowerDown(); //stop gps properly
      collar.Blink(_GREENLED);//Visual feedback, not required.
      collar.WDTSleep();//If successful print, go to sleep.
      //Can also use the following:
      //collar.WaitSleep(5);//This allows the user to set sleep time.
      collar.GPSPowerUp(); //start gps properly.
    }
    else
    {
      Serial.println("SD Card error.."); //sleeps here anyway, just to show it.
      collar.GPSPowerDown();
      collar.WDTSleep();
      collar.GPSPowerUp();
    }
  }
}