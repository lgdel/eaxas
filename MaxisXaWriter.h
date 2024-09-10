#pragma once
#include <stdint.h>
#include <fstream>
#include "Writer.h"

using namespace std;

class MaxisXaWriter : public Writer
{
public:
	MaxisXaWriter(WavParser& inputWav): Writer(inputWav) {}

	bool writeFile(fstream& out);

private:
	uint8_t* writeHeader();
	void encodeAndInterleave(int16_t* samples, uint8_t* output, int nSamples);
};
