// The H10-Controller class provides low-level control of the H10 punch and reader hardware. 
// 	It abstracts away the pin control and timing details, providing a simple interface for reading and punching bytes.
//
//		
#pragma once

#include <Arduino.h>
#include <SPI.h>
#include "CircularBuffer.h"

constexpr uint32_t kPunchLatchPulseTime = 200;
constexpr uint32_t kPunchStartPulseTime = 200; // must be at least 200 ns per H10 punch timing requirements

class H10_Controller
{
public:
	H10_Controller(
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
	bool queuePunchByte(uint8_t data);

private:

	uint8_t _punchStart;
	uint8_t _punchReady;
	uint8_t _readerReady;
	uint8_t _readerStart;
	uint8_t _readDataLoad;
	uint8_t _punchDataLatch;

	uint16_t _punchLatchCycles;
	uint16_t _punchStartCycles;

	static H10_Controller* _activeInstance;

	CircularBuffer _punchBuf;
	CircularBuffer _readerBuf;

	static void onPunchReadyISR();
	void onPunchReady();
	void punchByteImmediate(uint8_t data);

	static void onReaderReadyISR();
	void onReaderReady();

	void pulsePunchStart(uint32_t pulseCycles);
	void cyclePunchLatch(uint32_t pulseCycles);
};
