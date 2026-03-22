// The H10-Controller class provides low-level control of the H10 punch and reader hardware. 
// 	It abstracts away the pin control and timing details, providing a simple interface for reading and punching bytes.
//
//		
#pragma once

#include <Arduino.h>
#include <SPI.h>
#include "CircularBuffer.h"

class UART_Communications
{
public:
	UART_Communications(
		uint8_t punchStart,
		uint8_t punchReady,
		uint8_t readerReady,
		uint8_t readerStart,
		uint8_t readDataLoad,
		uint8_t punchDataLatch
	);

	void begin();

	// Reader
	bool isReaderReady();
	uint8_t readByte();

	// Punch
	bool isPunchReady();
	void punchByte(uint8_t data);

private:

	uint8_t _punchStart;
	uint8_t _punchReady;
	uint8_t _readerReady;
	uint8_t _readerStart;
	uint8_t _readDataLoad;
	uint8_t _punchDataLatch;

	static UART_Communications* _activeInstance;

	CircularBuffer _punchBuf;
	CircularBuffer _readerBuf;

	static void onPunchReadyISR();
	void onPunchReady();

	static void onReaderReadyISR();
	void onReaderReady();
};
