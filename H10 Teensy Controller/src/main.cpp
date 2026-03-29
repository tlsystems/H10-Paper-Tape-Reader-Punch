#include <Arduino.h>
#include <SPI.h>
#include "ComUART.h"
#include "H10_Manager.h"
#include "H10_Controller.h"

#include "defs.h"

//ComUART comRS232(Serial7, FTDI_RTS, FTDI_CTS);
ComUART comFTDI(Serial8, FTDI_RTS, FTDI_CTS);
H10_Manager h10Manager(comFTDI);

// put function declarations here:
// SPISettings spisettings(1000000, MSBFIRST, SPI_MODE2);
void SendTickMessage();
void FlashStatusLED(uint32_t blinkPeriod);


void setup() 
{
	// Initialize serial communication for console output
	Serial.begin(115200);

	pinMode(PB1, INPUT_PULLDOWN);
	pinMode(LED1, OUTPUT);
	pinMode(LED2, OUTPUT);

	//Serial8.begin(115200);
	comFTDI.begin(115200);
	h10Manager.begin();
	
	// Debug
	pinMode(STATUS_LED, OUTPUT); 
	pinMode(LED1, OUTPUT);
	pinMode(LED2, OUTPUT);
	pinMode(PB1, INPUT_PULLDOWN); 

}

void loop() 
{
	//SendTickMessage();
	FlashStatusLED(100);

	h10Manager.update();
}

 void serialEvent8()
 {
 	comFTDI.serviceRx();
 }

// void serialEvent7()
// {
// 	//comRS232.serviceRx();


void SendTickMessage()
{
	static uint32_t lastTickMillis = 0;

	auto curMillis = millis();
	
	if ((curMillis - lastTickMillis) > 1000)
	{
		Serial.print("Tick: ");
		Serial.println(curMillis/1000);
		Serial8.print("Tock ");
		Serial8.println(curMillis/1000);
		lastTickMillis = curMillis;
	}
}

void FlashStatusLED(uint32_t blinkPeriod)
{
	static uint32_t lastLedMillis = 0;
	auto curMillis = millis();
	if ((curMillis - lastLedMillis) > blinkPeriod)
	{
		digitalWrite(STATUS_LED, !digitalRead(STATUS_LED));
		lastLedMillis = curMillis;
	}
}