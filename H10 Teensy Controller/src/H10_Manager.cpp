// H10_Manager.cpp

#include "H10_Manager.h"

#include "H10_Controller.h"
#include "H10_Font5x8.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

H10_Manager::H10_Manager(ComUART& comDevice)
	: _comDevice(comDevice), _lineLength(0), bEcho(false), _hexDataLength(0)
{
	memset(_cmdLine, 0, sizeof(_cmdLine));
	_lastCommand.cmd = eCommand::Invalid;
	_lastCommand.data = nullptr;
	_lastCommand.length = 0;
	memset(_hexData, 0, sizeof(_hexData));
}

void H10_Manager::begin()
{
	h10Controller.begin();
}

void H10_Manager::update()
{
	// Check if there's a line ready
	if (!_comDevice.hasLine())
		return;

	// Read the line from the com device
	_lineLength = _comDevice.readLine(_cmdLine, sizeof(_cmdLine));
	//.Serial.printf("Received line: %s\n", _cmdLine);

	// Parse the command
	if (_lineLength > 0)
	{
		_lastCommand = parseCommand(_cmdLine, _lineLength);
	}

	//.Serial.printf("Parsed command: %s, cmdStr: %s\n", getCommandName(_lastCommand.cmd), _cmdLine);


	bool ok = false;
	switch (_lastCommand.cmd)
	{
		case eCommand::Help:		ok = doHelp(_lastCommand);		 break;
		case eCommand::Echo:		ok = doEcho(_lastCommand); 		 break;
		case eCommand::PunchByte:	ok = doPunchByte(_lastCommand);	 break;
		case eCommand::PunchText:	ok = doPunchText(_lastCommand);	 break;	
		case eCommand::PunchHex:	ok = doPunchHex(_lastCommand);	 break;
		case eCommand::PunchLeader:	ok = doPunchLeader(_lastCommand); break;
		case eCommand::ReadByte:	ok = doReadByte(_lastCommand);	 break;
		case eCommand::ReadHex:		doReadHex(_lastCommand);	 break;
		default:
			break;
	}

	if (ok)
	{
		_comDevice.writeLine("OK");
		//.Serial.printf("Executed command: %s\n", getCommandName(_lastCommand.cmd));
	}	
	else
	{
		_comDevice.writeLine("ERROR");
		//.Serial.printf("Failed to execute command: %s\n", getCommandName(_lastCommand.cmd));
	}	
	
}

H10_Manager::sCommand H10_Manager::parseCommand(char* line, size_t length)
{
	sCommand command;
	command.cmd = eCommand::Invalid;
	command.data = nullptr;
	command.length = 0;

	if (line == nullptr || length == 0)
	{
		command.cmd = eCommand::Invalid;
		return command;
	}	
	
	// Check if first character is '?' (single character help command)
	if (line[0] == '?')
	{
		command.cmd = eCommand::Help;
		return command;
	}

	// For other commands, need at least 2 characters
	if (length < 2)
	{
		command.cmd = eCommand::Invalid;
	}

	// Extract the first two characters as the command
	char cmdStr[3] = {0};
	cmdStr[0] = static_cast<char>(toupper(line[0]));
	cmdStr[1] = static_cast<char>(toupper(line[1]));
	cmdStr[2] = '\0';
	//.Serial.printf("Command string: %s\n", cmdStr);	

	if (strcmp(cmdStr, "EC") == 0) 		command.cmd = eCommand::Echo;
	else if (strcmp(cmdStr, "PB") == 0) command.cmd = eCommand::PunchByte;
	else if (strcmp(cmdStr, "PT") == 0) command.cmd = eCommand::PunchText;
	else if (strcmp(cmdStr, "PH") == 0)	command.cmd = eCommand::PunchHex;
	else if (strcmp(cmdStr, "PL") == 0) command.cmd = eCommand::PunchLeader;
	else if (strcmp(cmdStr, "RB") == 0) command.cmd = eCommand::ReadByte;
	else if (strcmp(cmdStr, "RH") == 0) command.cmd = eCommand::ReadHex;
	else 
		command.cmd = eCommand::Unknown;

	if (command.cmd == eCommand::ReadByte)
		return command;

	// remaining commands all require data
	if ((length < 4) || (line[2] != ' '))
	{
		command.cmd = eCommand::NoData;
		command.data = nullptr;
		command.length = 0;
		return command;
	}

	// data starts at 4th character
	command.data = &line[3];
	command.length = length - 3;

	return command;
}

bool H10_Manager::doHelp(const sCommand& command)
{
	_comDevice.writeLine("H10 Command Help:");
	_comDevice.writeLine("  ?            - Show this help");
	_comDevice.writeLine("  EC <0|1>     - Echo mode off/on");
	_comDevice.writeLine("  PB <byte>    - Punch one byte (0-255)");
	_comDevice.writeLine("  PT <text>    - Punch ASCII text");
	_comDevice.writeLine("  PH <record>  - Punch one Intel HEX record");
	_comDevice.writeLine("  PL <count>   - Punch <count> leader bytes");
	_comDevice.writeLine("  RB           - Read and return one byte");
	_comDevice.writeLine("  RH <count>   - Read <count> bytes as Intel HEX");
	return true;
}

bool H10_Manager::doEcho(const sCommand& command)
{
	bEcho = command.data[0] == '1';
	_comDevice.setEcho(bEcho);
	if (bEcho)
		_comDevice.writeLine("Echo mode enabled");
	else
		_comDevice.writeLine("Echo mode disabled");
	return true;
}

bool H10_Manager::doPunchByte(const sCommand& command)
{
	uint8_t outValue = 0;
	if (!parseByteValue(command, outValue))
		return false;

	return h10Controller.queuePunchByte(outValue);
}

bool H10_Manager::doPunchText(const sCommand& command)
{
	if (command.data == nullptr || command.length == 0)
		return false;

	for (size_t charIndex = 0; charIndex < command.length; ++charIndex)
	{
		uint8_t glyph[H10_Font5x8::kGlyphWidth] = {0};
		if (!H10_Font5x8::getGlyph(command.data[charIndex], glyph))
			continue;

		for (size_t glyphIndex = 0; glyphIndex < H10_Font5x8::kGlyphWidth; ++glyphIndex)
		{
			if (!h10Controller.queuePunchByte(glyph[glyphIndex]))
				return false;
		}
		// punch character 

		if (!h10Controller.queuePunchByte(0x00))
			return false;
	}
	
	return true;
}

bool H10_Manager::doPunchHex(const sCommand& command)
{
	return true;
	_hexDataLength = 0;

	if (command.data == nullptr || command.length < 11)
		return false;

	const char* data = command.data;
	const size_t len = command.length;
	if (data[0] != ':')
		return false;

	uint8_t byteCount = 0;
	uint8_t addrHigh = 0;
	uint8_t addrLow = 0;
	uint8_t recordType = 0;

	if (!parseHexByte(data, len, 1, byteCount))
		return false;
	if (!parseHexByte(data, len, 3, addrHigh))
		return false;
	if (!parseHexByte(data, len, 5, addrLow))
		return false;
	if (!parseHexByte(data, len, 7, recordType))
		return false;

	if (byteCount > sizeof(_hexData))
		return false;

	const size_t expectedLen = 1 + 2 + 4 + 2 + (static_cast<size_t>(byteCount) * 2) + 2;
	if (len != expectedLen)
		return false;

	uint8_t checksum = 0;
	uint8_t sum = 0;
	sum = static_cast<uint8_t>(sum + byteCount);
	sum = static_cast<uint8_t>(sum + addrHigh);
	sum = static_cast<uint8_t>(sum + addrLow);
	sum = static_cast<uint8_t>(sum + recordType);

	for (size_t i = 0; i < byteCount; ++i)
	{
		uint8_t value = 0;
		if (!parseHexByte(data, len, 9 + (i * 2), value))
		{
			_hexDataLength = 0;
			return false;
		}

		_hexData[i] = value;
		sum = static_cast<uint8_t>(sum + value);
	}

	if (!parseHexByte(data, len, 9 + (static_cast<size_t>(byteCount) * 2), checksum))
	{
		_hexDataLength = 0;
		return false;
	}

	sum = static_cast<uint8_t>(sum + checksum);
	if (sum != 0)
	{
		_hexDataLength = 0;
		return false;
	}

	_hexDataLength = byteCount;
	return true;
}

bool H10_Manager::doPunchLeader(const sCommand& command)
{
	return true;
}

bool H10_Manager::doReadByte(const sCommand& command)
{
	return true;
}

bool H10_Manager::doReadHex(const sCommand& command)
{
	return true;
}

H10_Manager::sCommand H10_Manager::getLastCommand() const
{
	return _lastCommand;
}

const char* H10_Manager::getLineBuffer() const
{
	return _cmdLine;
}

size_t H10_Manager::getLineLength() const
{
	return _lineLength;
}

void H10_Manager::clearLine()
{
	memset(_cmdLine, 0, sizeof(_cmdLine));
	_lineLength = 0;
	_lastCommand.cmd = eCommand::Invalid;
	_lastCommand.data = nullptr;
	_lastCommand.length = 0;
	_comDevice.clearLine();
}

const char* H10_Manager::getCommandName(eCommand cmd)
{
	switch (cmd)
	{
		case eCommand::Invalid:		return "Invalid";
		case eCommand::Help:		return "Help (?)";
		case eCommand::Echo:		return "Echo (EC)";
		case eCommand::PunchByte:	return "Punch Byte (PB)";
		case eCommand::PunchText: 	return "Punch Text (PT)";
		case eCommand::PunchHex: 	return "Punch Hex (PH)";
		case eCommand::PunchLeader: return "Punch Leader (PL)";
		case eCommand::ReadByte: 	return "Read Byte (RB)";
		case eCommand::ReadHex: 	return "Read Hex (RH)";
		case eCommand::Unknown: 	return "Unknown";
		case eCommand::NoData: 		return "No Data";
		default: 
			return "Invalid";
	}
}

uint8_t H10_Manager::reverseBits(uint8_t value)
{
	value = static_cast<uint8_t>(((value & 0xF0U) >> 4) | ((value & 0x0FU) << 4));
	value = static_cast<uint8_t>(((value & 0xCCU) >> 2) | ((value & 0x33U) << 2));
	value = static_cast<uint8_t>(((value & 0xAAU) >> 1) | ((value & 0x55U) << 1));
	return value;
}

// parsing functions
bool H10_Manager::TryParseHexChar(char ch, byte& bt)
{
	if (ch >= '0' && ch <= '9')
	{
		bt = ch - '0';
		return true;
	} 
	else if (ch >= 'A' && ch <= 'F') 
	{
		bt = ch - 'A' + 10;
		return true;
	}
	else if (ch >= 'a' && ch <= 'f') 
	{
		bt = ch - 'a' + 10;
		return true;
	}
	return false;
};

bool H10_Manager::parseHexByte(const char* data, size_t dataLen, size_t pos, uint8_t& outValue)
{
	if (!TryParseHexChar(data[pos], outValue) )
		return false;
	
	if (dataLen - pos < 2)
		return true;
	
	byte btVal;
	if (!TryParseHexChar(data[pos + 1], btVal))
		return false;	

	outValue = (outValue << 4) + btVal;

	return true;
}

bool H10_Manager::parseByteValue(const sCommand& command, uint8_t& outValue)
	{
		sCommand cmd = command;
		
		// parse as decimal
		char* endPtr = cmd.data;
		long intVal = strtol(command.data, &endPtr, 0);
		if (endPtr == command.data)
			return false;
		if (intVal < 0 || intVal > 255)
			return false;

		outValue = static_cast<uint8_t>(intVal);
		return true;
	};

bool H10_Manager::parseHexValue(const sCommand& command, uint8_t& outValue)
	{
		if (command.length < 2)
			return false;

		uint8_t highNibble, lowNibble;
		if (!TryParseHexChar(command.data[0], highNibble))
			return false;
		if (!TryParseHexChar(command.data[1], lowNibble))
			return false;

		outValue = (highNibble << 4) | lowNibble;

		return true;
	}