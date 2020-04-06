IF NOT EXIST "C:\Program Files (x86)\Arduino\arduino.exe" (
echo "Arduino installation not found, please install the latest version of Arduino"
PAUSE
EXIT)
ELSE (
echo "Arduino install found, piggy backing..."
"C:\Program Files (x86)\Arduino\hardware\tools\avr\bin\avrdude.exe" -C  usbasp-clone -P m328p -U lfuse:w:0xe2:m -U hfuse:w:0xd9:m -U efuse:w:0xff:m 
"C:\Program Files (x86)\Arduino\hardware\tools\avr\bin\avrdude.exe" -P m328p -C usbasp-clone -U flash:w:myarduino_blink.hex
PAUSE
EXIT
)