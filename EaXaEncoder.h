#pragma once
#include <stdint.h>

enum UncompressedType {
	Normal,
	FadeToCompressed,
	FadeFromCompressed
};

class EaXaEncoder {
public:
	void encodeSubblock(int16_t* inputData, uint8_t* outputData, int nSamples, int16_t* outputAudioResult = nullptr);
	void writeUncompressedSubblock(int16_t* inputData, uint8_t* outputData, int nSamples, UncompressedType type);
	void clearErrors();

	int currentSample = 0;
	int previousSample = 0;

private:
	double currentQuantError = 0;
	double previousQuantError = 0;
};
