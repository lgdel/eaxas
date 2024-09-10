#pragma once
#include <stdint.h>
#include <fstream>

using namespace std;

class WavParser {
public:
	WavParser(fstream& F) : in(F) {}

	bool initFile();
	bool readSamples(int16_t* output, int nSamples);

	unsigned int nChannels = 0;
	unsigned int sampleRate = 0;
	unsigned int nSamples = 0;
	fstream& in;
private:
	unsigned int audioStart = 0;
	unsigned int sampleOffset = 0;
};
