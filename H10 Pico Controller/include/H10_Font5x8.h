#pragma once

#include <Arduino.h>

class H10_Font5x8
{
public:
	static constexpr size_t kGlyphWidth = 5;
	static constexpr char kFirstChar = ' ';
	static constexpr char kLastChar = '~';
	static constexpr size_t kGlyphCount = static_cast<size_t>(kLastChar - kFirstChar + 1);

	// Each glyph is 5 columns wide, each column is an 8-bit vertical bitmap.
	// Bit 0 is the top pixel, bit 7 is the bottom pixel.
	static const uint8_t FontAscii[kGlyphCount][kGlyphWidth];

	// Copies the glyph columns for printable ASCII characters into outGlyph.
	// Returns false for non-printable characters.
	static bool getGlyph(char c, uint8_t outGlyph[kGlyphWidth]);
};
