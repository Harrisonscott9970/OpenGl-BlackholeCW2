#pragma once
// SceneHelpers.h — shared inline helpers used across scene .cpp files

#include <cstdlib>

inline float randf(float minVal, float maxVal)
{
    float t = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    return minVal + t * (maxVal - minVal);
}

inline const char* getBitmapGlyph(char c)
{
    switch (c)
    {
    case 'A': return "010,101,111,101,101";
    case 'B': return "110,101,110,101,110";
    case 'C': return "011,100,100,100,011";
    case 'D': return "110,101,101,101,110";
    case 'E': return "111,100,110,100,111";
    case 'F': return "111,100,110,100,100";
    case 'G': return "011,100,101,101,011";
    case 'H': return "101,101,111,101,101";
    case 'I': return "111,010,010,010,111";
    case 'J': return "001,001,001,101,010";
    case 'K': return "101,101,110,101,101";
    case 'L': return "100,100,100,100,111";
    case 'M': return "101,111,111,101,101";
    case 'N': return "101,111,111,111,101";
    case 'O': return "111,101,101,101,111";
    case 'P': return "110,101,110,100,100";
    case 'Q': return "111,101,101,111,001";
    case 'R': return "110,101,110,101,101";
    case 'S': return "011,100,111,001,110";
    case 'T': return "111,010,010,010,010";
    case 'U': return "101,101,101,101,111";
    case 'V': return "101,101,101,101,010";
    case 'W': return "101,101,111,111,101";
    case 'X': return "101,101,010,101,101";
    case 'Y': return "101,101,010,010,010";
    case 'Z': return "111,001,010,100,111";
    case '0': return "111,101,101,101,111";
    case '1': return "010,110,010,010,111";
    case '2': return "111,001,111,100,111";
    case '3': return "111,001,111,001,111";
    case '4': return "101,101,111,001,001";
    case '5': return "111,100,111,001,111";
    case '6': return "011,100,111,101,111";
    case '7': return "111,001,001,001,001";
    case '8': return "111,101,111,101,111";
    case '9': return "111,101,111,001,110";
    case ':': return "000,010,000,010,000";
    case '/': return "001,001,010,100,100";
    case '-': return "000,000,111,000,000";
    case '.': return "000,000,000,000,010";
    case '!': return "010,010,010,000,010";
    default:  return "000,000,000,000,000";
    }
}

inline bool bitmapGlyphPixel(const char* glyph, int row, int col)
{
    int idx = row * 4 + col;
    return glyph[idx] == '1';
}
