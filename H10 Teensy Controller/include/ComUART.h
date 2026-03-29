#pragma once

#include <Arduino.h>
#include "defs.h"
#include "ICom.h"

constexpr byte MAX_RX_LINE_LEN = 144;

class ComUART : public ICom
{
public:
	ComUART(HardwareSerialIMXRT& serialPort, uint8_t rtsPin, uint8_t ctsPin);

	void begin(uint32_t baudRate = 115200, uint16_t config = SERIAL_8N1) override;
	void setEcho(bool enabled);
	bool hasLine() const override;
	size_t readLine(char* outBuffer, size_t outBufferSize) override;
	void clearLine() override;
	bool overflowed() const override;

	size_t write(uint8_t data) override;
	size_t writeLine(const char* text) override;


	void serviceRx();

private:

	HardwareSerialIMXRT& _serialPort;
	uint8_t _rtsPin;
	uint8_t _ctsPin;
	// buffers are big enough for a 64 byte hex record
	byte _txBuffer[146];
	byte _rxBuffer[146];

	mutable char _lineBuffer[kMaxCommandSize];
	mutable size_t _lineLength;
	mutable bool _lineReady;
	mutable bool _overflow;
	bool _echo;
};
