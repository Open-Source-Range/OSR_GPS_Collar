#include <GPSCollar.h>
volatile int f_timer=0;
//The interrupt used
ISR(TIMER1_OVF_vect)
{
  /* set the flag. */
   if(f_timer == 0)
   {
     f_timer = 1;
   }
}





GPSCollar::GPSCollar(File dataFile)
{
	_dataFile=dataFile;
	TinyGPSPlus _GPS;
	//_GPS=&gps;
	//NeoSWSerial _SS(4,3);
	//_SS=myserial;


  pinMode(_GPSpower, OUTPUT);
  pinMode(_REDLED,OUTPUT);
  pinMode(_GREENLED,OUTPUT);
  LoadSettings();
}

void GPSCollar::GPSPowerDown()
{
	_SS.end();
	digitalWrite(_GPSpower,LOW);
}
void GPSCollar::GPSPowerUp()
{
	digitalWrite(_GPSpower,HIGH);
	delay(100);
	_SS.begin(_GPS_BAUD);
}

int GPSCollar::AttemptGPS()
{
	if(_SS.available()>0)
	{
		while(_SS.available()>0)
			{_GPS.encode(_SS.read());}
		
		if(_GPS.location.isUpdated()) 
		{
			if(_GPS.location.isValid())
			{
				if(_GPS.hdop.isValid()&&_GPS.hdop.value()<_DESIREDHDOP)
				{
					if(_GPS.date.isValid())
					{
						if(_GPS.time.isValid()&&_GPS.time.isUpdated())//the && time prevents double writtings
						{
							//if(_GPS->date.month()==ENDMONTH&&_GPS->.date.day()>=ENDDAY)//end of tests, wait forever.
							//{
							return 1;
							//}
						}
					}
				}
			}
		}
    }
	
	return 0;
}


int GPSCollar::DumpGPSInfo(double lat, double lng)
{
	_dataFile = SD.open("gpslog.csv", FILE_WRITE); //open SD
	int Error=0;
	while(!_dataFile&&Error<10)
    {
      SD.end();
      delay(500);
      SD.begin(_SDCHIPSELECT);
      _dataFile = SD.open("settings.csv", FILE_WRITE);
      Blink(_REDLED);
      Error++;
    }
	if (_dataFile)
	{
	//Print to SD
	_dataFile.print(String(_GPS.date.year())  +",");
	_dataFile.print(String(_GPS.date.month())+",");
	_dataFile.print(String(_GPS.date.day())+",");
 
	_dataFile.print(String(_GPS.time.hour())+",");
	_dataFile.print(String(_GPS.time.minute())+",");
	_dataFile.print(String(_GPS.time.second())+",");

	_dataFile.print(String(_GPS.hdop.value())+",");
	_dataFile.print(String(_GPS.satellites.value())+",");

	_dataFile.print(lat,10);_dataFile.print(",");
	_dataFile.println(lng,10);// Printing the commas by themselves allows me to force the length of the
                                                      // float printed both to the terminal, and to the SD card.
   delay(100);
   _dataFile.close();
   return 1;
	}
	else 
	{
		_dataFile.close();
		return 0;
	}
	
	
}

void GPSCollar::WaitSleep(int MinutesToSleep)
{//Written this way to avoid overflowing an int and getting no wait time.
	//Serial.println("going to sleep");
	while(MinutesToSleep--)
	{
		unsigned long time;
		time=millis();
		while(millis()-time>60000)
		{
			
		}
	}
	//Serial.println("wokeup");
}


void GPSCollar::WaitSleep()
{
	WaitSleep(_SHORTSLEEP);
}


void GPSCollar::WDTSleep(int MinutesToSleep)
{
   for(int sec=0, minutes=0;minutes<MinutesToSleep;sec+=4) //Actual waiting happens here
	{
		set_sleep_mode(SLEEP_MODE_IDLE);
		sleep_enable();
		power_adc_disable();
		power_spi_disable();
		power_timer0_disable();
		power_timer2_disable();
		power_twi_disable();
		/* Now enter sleep mode. */
		sleep_mode();

		/* The program will continue from here after the WDT timeout*/
		sleep_disable(); /* First thing to do is disable sleep. */
		//Consider moving this to after everything?
		/* Re-enable the peripherals. */ 
		power_all_enable();
		f_timer=0;
		if(sec>=60)
		{
			minutes++;
			sec=0;
		}
	}
}

void GPSCollar::WDTSleep()
{
	WDTSleep(_SHORTSLEEP);
}


void GPSCollar::Blink(int pin)
{
  digitalWrite(pin,LOW);
  delay(100);
  digitalWrite(pin,HIGH);
  delay(100);
  digitalWrite(pin,LOW);
  delay(100);

}

int GPSCollar::LoadSettings()
{
	_dataFile = SD.open("gpslog.csv", FILE_WRITE);
	if(_dataFile)
	{
		NumFromSD();
		_SHORTSLEEP=NumFromSD();;
		NumFromSD();
		_LONGSLEEP=NumFromSD();;
		NumFromSD();
		_BEGINNIGHT=NumFromSD();;
		NumFromSD();
		_ENDNIGHT=NumFromSD();;
		NumFromSD();
		_GPS_BAUD=NumFromSD();;
		NumFromSD();
		_ENDMONTH=NumFromSD();;
		NumFromSD();
		_ENDDAY=NumFromSD();;
		NumFromSD();
		_waitingForFix = NumFromSD();;
		NumFromSD();
		_GPS_TIMEOUT   = NumFromSD();; // 1.5 minutes
		NumFromSD();
		_GPS_TIME      = NumFromSD();;
		NumFromSD();
		_turnGPSoff = NumFromSD();;
		NumFromSD();
		_DESIREDHDOP = NumFromSD();;
		NumFromSD();
		_TIMEZONEADJUST =NumFromSD();;
		NumFromSD();
		_THRESHHOLD=NumFromSD();;
		NumFromSD();
	}
	_dataFile.close();
}

int GPSCollar::NumFromSD()
{
  int ToReturn=0;
  int buf;
  int num[4];
  int i=0,neg=0,j=0;
  while(_dataFile.read()!=','); //read until ,
  do//begin extracting numbers
  {
    buf=_dataFile.read(); //get char
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

int GPSCollar::CollarSD()
{
	return _SDCHIPSELECT;
}

void GPSCollar::WDTInit()//gets the WDT ready to use, does not enable system interrupts.  The user is required to do so by using the "Interrupts();" command.
{
	TCCR1A = 0x00;
	TCNT1=0x0000; 
	TCCR1B = 0x05;
	TIMSK1=0x01;
	delay(50);
}

int GPSCollar::WriteAverageGPS(int iterations,int ms)
{
	double buflat=0.0,buflong=0.0;
	int i=0;
	for (i;i<iterations;i++)
	{
		buflat=buflat+_GPS.location.lat();
		buflong=buflong+_GPS.location.lng();
		delay(ms);
		while(_SS.available()>0)
			{_GPS.encode(_SS.read());}
	}
	buflat=buflat/iterations;
	buflong=buflong/iterations;
	Serial.print("Average: ");
	Serial.println(buflat,6);
	Serial.print("Direct Reading: ");
	Serial.println(_GPS.location.lat(),6);
	return DumpGPSInfo(buflat,buflong);
	
}

int GPSCollar::PrintGPS()
{
	return DumpGPSInfo(_GPS.location.lat(),_GPS.location.lng());
}

int GPSCollar::BurnIn(int msTimeOut)
{
	unsigned long start=millis();
	double last=0;
	while(millis()<start+msTimeOut)
	{
		while(_SS.available()>0)
			{_GPS.encode(_SS.read());}
		if(last-_GPS.location.lat()<_THRESHHOLD&&_GPS.location.lat()-last<_THRESHHOLD)//Sign error here? Possible should be OR, not AND 
			return 1;
		else 
			last=_GPS.location.lat();
	}
	return 0;
}
