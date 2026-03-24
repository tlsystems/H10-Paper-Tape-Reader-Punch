#pragma once

#include <Arduino.h>
#include "ICom.h"

class SerialUART;
extern "C" typedef struct uart_inst uart_inst_t;

constexpr byte MAX_RX_LINE_LEN = 144;

class ComUART : public ICom
{
public:
	ComUART(
		SerialUART& serialPort,
		uint8_t rxPin,
		uint8_t txPin,
		uint8_t rtsPin,
		uint8_t ctsPin
	);

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
	static constexpr size_t kTxBufferSize = 256;

	static void onUartIRQ();
	void haldleUartIRQ();

	SerialUART& _serialPort;
	uart_inst_t* _uart;
	uint8_t _rxPin;
	uint8_t _txPin;
	uint8_t _rtsPin;
	uint8_t _ctsPin;
	volatile char _lineBuffer[kLineBufferSize];
	volatile size_t _lineLength;
	volatile bool _lineReady;
	volatile bool _overflow;
	volatile bool _skipNextLF;
	volatile char _txBuffer[kTxBufferSize];
	volatile size_t _txHead;
	volatile size_t _txTail;

	static ComUART* _activeInstance;
};
