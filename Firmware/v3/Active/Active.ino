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
  }


void loop()
{
  if(collar.AttemptGPS())//Is the GPS valid ?
  {
                                //Valid GPS data found
    //if(collar.BurnIn(3000));    //How much time before it gives up
      //Burn in Success
   // else
    //{
      //Unstable GPS values after burn in
    //}

    
    if(collar.PrintGPS())//Print the average, use PrintGPS() to avoid the average.
    {
      //successful print
      collar.GPSPowerDown();
      collar.Blink(_GREENLED);//Visual feedback, not required.
      collar.WaitSleep();//If successful print, go to sleep.
      //Can also use the following:
      //collar.WaitSleep(5);//This allows the user to set sleep time.
      collar.GPSPowerUp();
    }
    else
    {
      //unsucessful print, at this point probably an SD card error
      collar.GPSPowerDown();
      collar.Blink(_REDLED);//Visual feedback, not required.
      collar.WaitSleep();
      collar.GPSPowerUp();
    }
  }
  collar.Blink(_REDLED);
}
