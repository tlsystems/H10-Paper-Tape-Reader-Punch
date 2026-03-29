#pragma once

#include <Arduino.h>

class ICom
{
public:
	virtual ~ICom() = default;

	virtual void begin(uint32_t baudRate = 115200, uint16_t config = SERIAL_8N1) = 0;
	virtual bool hasLine() const = 0;
	virtual size_t readLine(char* outBuffer, size_t outBufferSize) = 0;
	virtual bool overflowed() const = 0;
	virtual void clearLine() = 0;

	virtual size_t write(uint8_t data) = 0;
	virtual size_t writeLine(const char* text) = 0;
};
