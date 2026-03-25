#pragma once

#include <Arduino.h>
#include "ICom.h"

constexpr byte MAX_RX_LINE_LEN = 144;

class ComUART : public ICom
{
public:
	ComUART(HardwareSerialIMXRT& serialPort, uint8_t rtsPin, uint8_t ctsPin);

	void begin(uint32_t baudRate = 115200, uint16_t config = SERIAL_8N1) override;
	bool hasLine() const override;
	size_t readLine(char* outBuffer, size_t outBufferSize) override;
	bool overflowed() const override;
	void clearLine() override;

	int available() override;
	int read() override;
	size_t write(uint8_t data) override;
	size_t writeLine(const char* text) override;

private:
	static constexpr size_t kLineBufferSize = 128;

	void pollRx() const;

	HardwareSerialIMXRT& _serialPort;
	uint8_t _rtsPin;
	uint8_t _ctsPin;
	mutable char _lineBuffer[kLineBufferSize];
	mutable size_t _lineLength;
	mutable bool _lineReady;
	mutable bool _overflow;
	mutable bool _skipNextLF;
};
