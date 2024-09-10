#include <stdint.h>
#include <iostream>
#include <fstream>
#include "WavParser.h"
#include "utility.h"

using namespace std;

void printParseError() {
	cerr << "Error parsing WAV file" << endl;
}

bool WavParser::initFile()
{
	in.seekg(0, ios::end);
	auto filesize = in.tellg();
	if (filesize < 46) {
		printParseError();
		return false;
	}

	in.seekg(0, ios::beg);
	uint32_t idRIFF = readValue<uint32_t>(in);
	if (memcmp(&idRIFF, "RIFF", 4) != 0) {
		printParseError();
		return false;
	}
	in.seekg(4, ios::cur);
	uint32_t idWAVE = readValue<uint32_t>(in);
	if (memcmp(&idWAVE, "WAVE", 4) != 0) {
		printParseError();
		return false;
	}

	while (in.good() && in.tellg() < filesize) {
		uint32_t chunkName = readValue<uint32_t>(in);
		uint32_t chunkSize = readValue<uint32_t>(in);
		uint32_t chunkEnd = (uint32_t)in.tellg() + chunkSize;
		if (memcmp(&chunkName, "fmt ", 4) == 0) {
			if (chunkSize < 16) {
				printParseError();
				return false;
			}
			auto format = readValue<uint16_t>(in);
			if (format != 1) {
				cerr << "Only PCM WAV files accepted" << endl;
				return false;
			}
			nChannels = readValue<uint16_t>(in);
			if (nChannels == 0 || nChannels > 7) {
				cerr << "Bad number of channels" << endl;
				return false;
			}
			sampleRate = readValue<uint32_t>(in);
			in.seekg(6, ios::cur);
			auto bitsPerSample = readValue<uint16_t>(in);
			if (bitsPerSample != 16) {
				cerr << "Only 16 bit PCM WAV accepted" << endl;
				return false;
			}
		}
		else if (memcmp(&chunkName, "data", 4) == 0) {
			if (sampleRate == 0) {
				printParseError();
				return false;
			}
			audioStart = in.tellg();
			nSamples = chunkSize / 2 / nChannels;
		}

		in.seekg(chunkEnd, ios::beg);
	}

	if (!in.good() || in.tellg() > filesize || nSamples == 0) {
		printParseError();
		return false;
	}
	in.seekg(audioStart, ios::beg);
	return true;
}

bool WavParser::readSamples(int16_t* output, int nSamples)
{
	if (nChannels == 1) {
		in.read((char*)output, 2 * nSamples);
		if (!in.good()) return false;
	} else {
		int samplesToRead = nSamples * nChannels;
		int16_t* rawSamples = new int16_t[samplesToRead];
		unique_ptr<int16_t[]> rawSamplesUniqPt(rawSamples);
		in.read((char*)rawSamples, 2 * samplesToRead);
		if (!in.good()) return false;

		for (unsigned c = 0; c < nChannels; c++) {
			int16_t* rawSamplesIte = rawSamples + c;
			int16_t* outputIte = output + c * nSamples;
			for (int i = 0; i < nSamples; i++) {
				*outputIte++ = *rawSamplesIte;
				rawSamplesIte += nChannels;
			}
		}
	}

	sampleOffset += nSamples;
	return true;
}
