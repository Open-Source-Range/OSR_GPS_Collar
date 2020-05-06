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
    Serial.begin(9600);
    Serial.println("Starting...");

  }


void loop()
{
  if(collar.AttemptGPS())//Is the GPS valid ?
  {
    Serial.println("GPS Data Valid");
    if(collar.BurnIn(3000)) //How much time before it gives up
      Serial.println("Burn in success");
    else
    {
      Serial.println("Burn in timeout - - Values not stabilized");
    }
    if(collar.WriteAverageGPS(10,200))//Print the average, use PrintGPS() to avoid the average.
    {
      collar.GPSPowerDown();
      collar.Blink(_GREENLED);//Visual feedback, not required.
      collar.WaitSleep();//If successful print, go to sleep.
      //Can also use the following:
      //collar.WaitSleep(5);//This allows the user to set sleep time.
      collar.GPSPowerUp();
    }
    else
    {
      Serial.println("SD Card error..");
      collar.GPSPowerDown();
      collar.Blink(_GREENLED);//Visual feedback, not required.
      collar.WaitSleep();//If successful print, go to sleep.
      //Can also use the following:
      //collar.WaitSleep(5);//This allows the user to set sleep time.
      collar.GPSPowerUp();
    }
  }
}