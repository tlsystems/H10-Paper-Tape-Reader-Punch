// H10_Manager.h 
//
// The H10_Manager class is responsible for handling and responding to commands issued by a 
//	host computer. It looks for commands comming from either of the two serial port interfaces
//	or the Wi-Fi connection. It parses incoming commands, executes them, and provides utility 
// 	functions for handling command data. The following commands are implemented:T
//
// 
//  ? 			Help		Returns a list of available commands and their usage
//  EC <x>		Echo Mode	x=0 (off) or 1 (on)
//  PB <bt>		Punch Byte 	bt=byte to punch (0-255)
//  PT <text>	Punch Text 	text=string to punch	
//  PH <hex>	Punch Hex 	hex is a Intel Hex record
//  PL <x>		Punch Leader x=number of 0x00 bytes to punch as leader
//  RB			Read Byte 	reads and return on byte 
//  RH <x>		Read Hex  	reads x bytes and return an Intel Hex record

#pragma once

#include <Arduino.h>
#include <ComUART.h>
#include "H10_Controller.h"

class H10_Manager
{
public:
	enum class eCommand
	{
		Invalid,		// No command parsed
		Help,			// ? - Help
		Echo,			// EC - Echo Mode
		PunchByte,		// PB - Punch byte
		PunchText,		// PT - Punch text
		PunchHex,		// PH - Punch Hex record
		PunchLeader,	// PL - Punch leader
		ReadByte,		// RB - Read byte
		ReadHex,		// RH - Read Hex record
		Unknown,		// Unknown command
		NoData			// No data for command	
	};

	struct sCommand
	{
		// Command format is "xx ddddd...", 
		// 	where xx is 1 or 2 character command code, followed by a space, then optional data.
		eCommand cmd;		// command
		char* 	 data;		// pointer to data (null terminated string)
		size_t 	 length;	// length of data
	};

public:
	H10_Manager(ComUART& comDevice);
	
	void begin();
	void update();

	sCommand getLastCommand() const;

	const char* getLineBuffer() const;

	size_t getLineLength() const;

	void clearLine();

	static const char* getCommandName(eCommand cmd);


	static uint8_t reverseBits(uint8_t value);

private:
	static constexpr size_t kLineBufferSize = 144;

	sCommand parseCommand(char* line, size_t length);

	bool TryParseHexChar(char ch, byte& bt);
	bool parseHexByte(const char* data, size_t dataLen, size_t pos, uint8_t& outValue);
	bool parseByteValue(const sCommand& command, uint8_t& outValue);
	bool parseHexValue(const sCommand& command, uint8_t& outValue);

	bool doHelp(const sCommand& command);
	bool doEcho(const sCommand& command);
	bool doPunchByte(const sCommand& command);
	bool doPunchText(const sCommand& command);
	bool doPunchHex(const sCommand& command);
	bool doPunchLeader(const sCommand& Command);
	bool doReadByte(const sCommand& command);
	bool doReadHex(const sCommand& command);


	H10_Controller h10Controller;
	ComUART& 	_comDevice;
	char 		_cmdLine[kMaxCommandSize];
	size_t 		_lineLength;
	sCommand     _lastCommand;
	bool 		bEcho;
	uint8_t 	_hexData[128];
	size_t 		_hexDataLength;
};
