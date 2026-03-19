#include <Arduino.h>
#include <spi.h>
#include "H10_Controller.h"
#include "H10_Manager.h"

#include "defs.h"

H10_Controller h10Controller(PunchStart, PunchReady, ReaderReady, ReaderStart, ReadDataLoad, PunchDataLatch);
H10_Manager h10Manager(Serial1, RS232_RX, RS232_TX, RS232_RTS, RS232_CTS);

// put function declarations here:
// SPISettings spisettings(1000000, MSBFIRST, SPI_MODE2);

void setup() 
{
	// Initialize serial communication for console output
	Serial.begin(115200);

	Serial.begin(115200);

	pinMode(LED1, OUTPUT);
	pinMode(LED2, OUTPUT);
	pinMode(PB1, INPUT_PULLDOWN);

	// Initialize RS232 port with RTS/CTS handshaking.
	h10Manager.begin(115200);

	SPI.setRX(MISO);
	SPI.setTX(MOSI);
	SPI.setSCK(SCK);
	SPI.begin(true);

	pinMode(PunchStart, OUTPUT);
	pinMode(PunchReady, INPUT);
	pinMode(ReaderReady, INPUT);
	pinMode(ReaderStart, OUTPUT);

	pinMode(ReadDataLoad, OUTPUT);
	pinMode(PunchDataLatch, OUTPUT);

	// Debug
	pinMode(PB1, INPUT_PULLDOWN); 
	pinMode(LED1, OUTPUT);
	pinMode(LED2, OUTPUT);

	pinMode(VBUS_SENSE, INPUT); 
	pinMode(PICO_LED, OUTPUT); 
}

void loop() 
{
	static int lastMillis = 0;
	static char rxLine[129];
	auto curMillis = millis();
	if ((curMillis - lastMillis) > 1000)
	{
		Serial.print("tick ");
		Serial.println(curMillis/1000);
		lastMillis = curMillis;
		digitalWrite(PICO_LED, !digitalRead(PICO_LED));
	}

	if (h10Manager.hasLine())
	{
		size_t len = h10Manager.readLine(rxLine, sizeof(rxLine));
		if (len > 0)
		{
			Serial.print("RX: ");
			Serial.print(rxLine);
			if (rxLine[len - 1] != '\n')
			{
				Serial.println();
			}
		}
	}
}

