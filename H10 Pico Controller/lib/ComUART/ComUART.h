#pragma once

#include <Arduino.h>

class SerialUART;
extern "C" typedef struct uart_inst uart_inst_t;

constexpr byte MAX_RX_LINE_LEN = 144;

class ComUART
{
public:
	ComUART(
		SerialUART& serialPort,
		uint8_t rxPin,
		uint8_t txPin,
		uint8_t rtsPin,
		uint8_t ctsPin
	);

	void begin(uint32_t baudRate = 115200, uint16_t config = SERIAL_8N1);
	bool hasLine() const;
	size_t readLine(char* outBuffer, size_t outBufferSize);
	bool overflowed() const;
	void clearLine();

	int available();
	int read();
	size_t write(uint8_t data);

private:
	static constexpr size_t kLineBufferSize = 128;

	static void onUartRxIRQ();
	void handleUartRxIRQ();

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

	static ComUART* _activeInstance;
};
