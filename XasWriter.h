#pragma once
#include <stdint.h>
#include <fstream>
#include "Writer.h"

using namespace std;

class XasWriter : public Writer
{
public:
	XasWriter(bool loop, WavParser& inputWav) : Writer(inputWav), isLoop(loop) {}

	bool writeFile(fstream& out);

private:
	uint8_t* writeHeader(int& rSize);
	void writeBlockSizeInHeader(fstream& outFile);
	void encodeXasBlock(int16_t* samples, uint8_t*& output, int nSamples);

	int offsetBlockSizeValue = 0;
	bool isLoop;
};
