#include <Arduino.h>
#include <SPI.h>
#include "ComUART.h"
#include "H10_Manager.h"
#include "H10_Controller.h"

#include "defs.h"

H10_Controller h10Controller(PunchStart, PunchReady, ReaderReady, ReaderStart, ReadDataLoad, PunchDataLatch);
ComUART comUART1(Serial8, FTDI_RTS, FTDI_CTS);
H10_Manager h10Manager(comUART1);

// put function declarations here:
// SPISettings spisettings(1000000, MSBFIRST, SPI_MODE2);
void SimpleTest(int blinkPeriod);


void setup() 
{
	// Initialize serial communication for console output
	Serial.begin(115200);

	pinMode(LED1, OUTPUT);
	pinMode(LED2, OUTPUT);
	pinMode(PB1, INPUT_PULLUP);

	// Initialize RS232 port with RTS/CTS handshaking.
	comUART1.begin(115200);
	h10Controller.begin();

	SPI.setMISO(MISO);
	SPI.setMOSI(MOSI);
	SPI.setSCK(SCK);
	SPI.begin();

	pinMode(PunchStart, OUTPUT);
	pinMode(PunchReady, INPUT);
	pinMode(ReaderReady, INPUT);
	pinMode(ReaderStart, OUTPUT);

	pinMode(ReadDataLoad, OUTPUT);
	pinMode(PunchDataLatch, OUTPUT);

	// Debug
	pinMode(PB1, INPUT_PULLUP); 
	pinMode(LED1, OUTPUT);
	pinMode(LED2, OUTPUT);

	pinMode(STATUS_LED, OUTPUT); 
}

void loop() 
{
	SimpleTest(1000);

	// h10Manager.update();
}

void SimpleTest(int blinkPeriod)
{
	static int lastLedMillis = 0;
	static int lastTickMillis = 0;
	auto curMillis = millis();
	
	if ((curMillis - lastTickMillis) > 1000)
	{
		Serial.print("Tick: ");
		Serial.println(curMillis/1000);
		lastTickMillis = curMillis;
	}

	if ((curMillis - lastLedMillis) > blinkPeriod)
	{
		digitalWrite(STATUS_LED, !digitalRead(STATUS_LED));
		lastLedMillis = curMillis;
	}
}