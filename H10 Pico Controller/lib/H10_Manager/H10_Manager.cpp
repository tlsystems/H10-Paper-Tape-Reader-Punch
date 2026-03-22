#include "H10_Manager.h"
#include <string.h>
#include <ctype.h>

namespace
{
	int hexToNibble(char chHex)
	{
		if (chHex >= '0' && chHex <= '9') return chHex - '0';
		if (chHex >= 'a' && chHex <= 'f') return chHex - 'a' + 10;
		if (chHex >= 'A' && chHex <= 'F') return chHex - 'A' + 10;
		return -1;
	}

	bool parseHexByte(const char* data, size_t length, size_t index, uint8_t& out)
	{
		if ((index + 1) >= length)
		{
			return false;
		}

		const int hi = hexToNibble(data[index]);
		const int lo = hexToNibble(data[index + 1]);
		if (hi < 0 || lo < 0)
			return false;

		out = static_cast<uint8_t>((hi << 4) | lo);
		return true;
	}
}

H10_Manager::H10_Manager(ComUART& comDevice)
	: _comDevice(comDevice), _lineLength(0), bEcho(false), _hexDataLength(0)
{
	memset(_lineBuffer, 0, sizeof(_lineBuffer));
	_lastCommand.command = cmdNone;
	_lastCommand.data = nullptr;
	_lastCommand.dataLength = 0;
	memset(_hexData, 0, sizeof(_hexData));
}

H10_Manager::ParsedCommand H10_Manager::update()
{
	// Reset to no command
	ParsedCommand result;
	result.command = cmdNone;
	result.data = nullptr;
	result.dataLength = 0;

	// Check if there's a line ready
	if (!_comDevice.hasLine())
	{
		return result;
	}

	// Read the line from manager
	size_t readLen = _comDevice.readLine(_lineBuffer, kLineBufferSize);
	_lineLength = readLen;

	// Parse the command
	if (readLen > 0)
	{
		result = parseCommand(_lineBuffer, readLen);
		_lastCommand = result;
	}

	return result;
}

H10_Manager::ParsedCommand H10_Manager::parseCommand(const char* line, size_t length)
{
	ParsedCommand result;
	result.command = cmdNone;
	result.data = nullptr;
	result.dataLength = 0;

	if (line == nullptr || length < 1)
	{
		result.command = cmdUnknown;
		return result;
	}

	// Check if first character is '?' (single character help command)
	if (line[0] == '?')
	{
		result.command = cmdHelp;
		doHelp(result);
		return result;
	}

	// For other commands, need at least 2 characters
	if (length < 2)
	{
		result.command = cmdUnknown;
		return result;
	}

	// Extract the first two characters as the command
	char cmdStr[3] = {0};
	cmdStr[0] = static_cast<char>(toupper(line[0]));
	cmdStr[1] = static_cast<char>(toupper(line[1]));
	cmdStr[2] = '\0';

	// Identify the command directly in parseCommand using string compares.
	if (strcmp(cmdStr, "EC") == 0) 		result.command = cmdEcho;
	else if (strcmp(cmdStr, "PB") == 0) result.command = cmdPunchByte;
	else if (strcmp(cmdStr, "PT") == 0) result.command = cmdPunchText;
	else if (strcmp(cmdStr, "PH") == 0)	result.command = cmdPunchHex;
	else if (strcmp(cmdStr, "PL") == 0) result.command = cmdPunchLeader;
	else if (strcmp(cmdStr, "RB") == 0) result.command = cmdReadByte;
	else if (strcmp(cmdStr, "RH") == 0) result.command = cmdReadHex;
	else 
		result.command = cmdUnknown;

	if (result.command == cmdEcho ||
		result.command == cmdPunchByte ||
		result.command == cmdPunchText ||
		result.command == cmdPunchHex)
	{
		// Data commands must be "CC <data>".
		if ((length < 4) || (line[2] != ' '))
		{
			result.command = cmdUnknown;
			return result;
		}

		result.data = &line[3];
		result.dataLength = length - 3;
	}
	else if (length > 2)
	{
		result.data = &line[2];
		result.dataLength = length - 2;
	}

	switch (result.command)
	{
		case cmdEcho:			doEcho(result);			break;
		case cmdPunchByte:		doPunchByte(result);	break;
		case cmdPunchText:		doPunchText(result);	break;	
		case cmdPunchHex:		doPunchHex(result);		break;
		case cmdPunchLeader:	doPunchLeader(result);	break;
		case cmdReadByte:		doReadByte(result);		break;
		case cmdReadHex:		doReadHex(result);		break;
		default:
			break;
	}

	return result;
}

void H10_Manager::doHelp(const ParsedCommand& parsedCommand)
{
	(void)parsedCommand;
}

void H10_Manager::doEcho(const ParsedCommand& parsedCommand)
{
	if (parsedCommand.dataLength == 0 || parsedCommand.data == nullptr)
	{
		return;
	}

	bEcho = (parsedCommand.data[0] != '0');
}

void H10_Manager::doPunchByte(const ParsedCommand& parsedCommand)
{
	(void)parsedCommand;
}

void H10_Manager::doPunchText(const ParsedCommand& parsedCommand)
{
	(void)parsedCommand;
}

void H10_Manager::doPunchHex(const ParsedCommand& parsedCommand)
{
	_hexDataLength = 0;

	if (parsedCommand.data == nullptr || parsedCommand.dataLength < 11)
	{
		return;
	}

	const char* data = parsedCommand.data;
	const size_t len = parsedCommand.dataLength;
	if (data[0] != ':')
	{
		return;
	}

	uint8_t byteCount = 0;
	uint8_t addrHigh = 0;
	uint8_t addrLow = 0;
	uint8_t recordType = 0;

	if (!parseHexByte(data, len, 1, byteCount))
	{
		return;
	}
	if (!parseHexByte(data, len, 3, addrHigh))
	{
		return;
	}
	if (!parseHexByte(data, len, 5, addrLow))
	{
		return;
	}
	if (!parseHexByte(data, len, 7, recordType))
	{
		return;
	}

	if (byteCount > sizeof(_hexData))
	{
		return;
	}

	const size_t expectedLen = 1 + 2 + 4 + 2 + (static_cast<size_t>(byteCount) * 2) + 2;
	if (len != expectedLen)
	{
		return;
	}

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
			return;
		}

		_hexData[i] = value;
		sum = static_cast<uint8_t>(sum + value);
	}

	if (!parseHexByte(data, len, 9 + (static_cast<size_t>(byteCount) * 2), checksum))
	{
		_hexDataLength = 0;
		return;
	}

	sum = static_cast<uint8_t>(sum + checksum);
	if (sum != 0)
	{
		_hexDataLength = 0;
		return;
	}

	_hexDataLength = byteCount;
}

void H10_Manager::doPunchLeader(const ParsedCommand& parsedCommand)
{
	(void)parsedCommand;
}

void H10_Manager::doReadByte(const ParsedCommand& parsedCommand)
{
	(void)parsedCommand;
}

void H10_Manager::doReadHex(const ParsedCommand& parsedCommand)
{
	(void)parsedCommand;
}

H10_Manager::ParsedCommand H10_Manager::getLastCommand() const
{
	return _lastCommand;
}

const char* H10_Manager::getLineBuffer() const
{
	return _lineBuffer;
}

size_t H10_Manager::getLineLength() const
{
	return _lineLength;
}

void H10_Manager::clearLine()
{
	memset(_lineBuffer, 0, sizeof(_lineBuffer));
	_lineLength = 0;
	_lastCommand.command = cmdNone;
	_lastCommand.data = nullptr;
	_lastCommand.dataLength = 0;
	_comDevice.clearLine();
}

const char* H10_Manager::getCommandName(eCommand cmd)
{
	switch (cmd)
	{
		case cmdNone:
			return "None";
		case cmdHelp:
			return "Help (?)";
		case cmdEcho:
			return "Echo (EC)";
		case cmdPunchByte:
			return "Punch Byte (PB)";
		case cmdPunchText:
			return "Punch Text (PT)";
		case cmdPunchHex:
			return "Punch Hex (PH)";
		case cmdPunchLeader:
			return "Punch Leader (PL)";
		case cmdReadByte:
			return "Read Byte (RB)";
		case cmdReadHex:
			return "Read Hex (RH)";
		case cmdUnknown:
			return "Unknown";
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
