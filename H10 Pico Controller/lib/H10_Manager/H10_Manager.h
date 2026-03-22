#pragma once

#include <Arduino.h>
#include <ComUART.h>

class H10_Manager
{
public:
	enum eCommand
	{
		cmdNone,			// No command parsed
		cmdHelp,			// ? - Help
		cmdEcho,			// EC - Echo Mode
		cmdPunchByte,		// PB - Punch byte
		cmdPunchText,		// PT - Punch text
		cmdPunchHex,		// PH - Punch Hex record
		cmdPunchLeader,		// PL - Punch leader
		cmdReadByte,		// RB - Read byte
		cmdReadHex,			// RH - Read Hex record
		cmdUnknown			// Unknown command
	};

	struct ParsedCommand
	{
		eCommand command;		// Parsed command
		const char* data;		// Pointer to data after command (2 characters)
		size_t dataLength;		// Length of data portion
	};

public:
	/// @brief Constructor
	/// @param manager Reference to H10_Manager instance
	H10_Manager(ComUART& comDevice);

	/// @brief Check if a line is available and parse it if present
	/// @return Parsed command structure
	ParsedCommand update();

	/// @brief Get the last parsed command
	/// @return The most recently parsed command
	ParsedCommand getLastCommand() const;

	/// @brief Get the full line buffer
	/// @return Pointer to the line buffer
	const char* getLineBuffer() const;

	/// @brief Get the line length
	/// @return Length of the current line
	size_t getLineLength() const;

	/// @brief Clear the current line and command
	void clearLine();

	/// @brief Get command name as string
	/// @param cmd The command to get the name for
	/// @return String representation of the command
	static const char* getCommandName(eCommand cmd);

	/// @brief Reverse the bit order in a byte
	/// @param value The byte to reverse
	/// @return The byte with bit order reversed
	static uint8_t reverseBits(uint8_t value);

private:
	static constexpr size_t kLineBufferSize = 144;

	/// @brief Parse a line and extract command and data
	/// @param line The line to parse
	/// @param length The length of the line
	/// @return Parsed command structure
	ParsedCommand parseCommand(const char* line, size_t length);

	/// @brief Handle help command.
	void doHelp(const ParsedCommand& parsedCommand);
	void doEcho(const ParsedCommand& parsedCommand);
	void doPunchByte(const ParsedCommand& parsedCommand);
	void doPunchText(const ParsedCommand& parsedCommand);
	void doPunchHex(const ParsedCommand& parsedCommand);
	void doPunchLeader(const ParsedCommand& parsedCommand);
	void doReadByte(const ParsedCommand& parsedCommand);
	void doReadHex(const ParsedCommand& parsedCommand);

	ComUART& _comDevice;
	char _lineBuffer[kLineBufferSize];
	size_t _lineLength;
	ParsedCommand _lastCommand;
	bool bEcho;
	uint8_t _hexData[128];
	size_t _hexDataLength;
};
