#pragma once

#include <Arduino.h>

class SerialUART;
extern "C" typedef struct uart_inst uart_inst_t;

constexpr byte MAX_RX_LINE_LEN = 144;

class H10_Manager
{
public:
	enum eCmd
	{
		cmdInvalid,		
		cmdHelp,		// ?  - Displays help information
		cmdEcho,		// EC - Echo Mode 
		cmdPunchByte,	// PB - Punch byte
		cmdPunchText,	// PT - Punch text
		cmdPunchHex,	// PH - Punch Hex record
		cmdPunchLeader,	// PL - Punch empty leader
		cmdReadByte,	// RB - Read byte
		cmdReadHex,		// RH - Read Hex record
	};

public:
	H10_Manager(
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
	
	void processCommand(class UART_Communications& controller);
private:
	void parseCommand();
	// high level commands
	void doCommand(eCmd cmd);
	void doHelpCommand();
	void doEchoCommand();
	void doPunchByteCommand();
	void doPunchTextCommand();
	void doPunchHexCommand();
	void doPunchLeaderCommand();
	void doReadByteCommand();
	void doReadHexCommand();

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

	static H10_Manager* _activeInstance;
};
