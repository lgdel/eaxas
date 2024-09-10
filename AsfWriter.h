#pragma once
#include <stdint.h>
#include <fstream>
#include "Writer.h"

using namespace std;

class AsfWriter : public Writer
{
public:
	AsfWriter(int revision, WavParser& inputWav): revision(revision), Writer(inputWav) {}

	bool writeFile(fstream& out);

private:
	uint8_t* writeHeader(int& rSize);
	void writeNumberOfBlocksInHeader(fstream& outFile, uint32_t nBlocks);
	uint8_t* writeSCDlBlock(int nbSamples, int16_t* samples, bool firstBlock, int& rSize);

	int revision;
	int offsetSCClValue = 0;
};
