#include <stdint.h>
#include <iostream>
#include "utility.h"
#include "EaXaEncoder.h"

using namespace std;

// KFilters: -ea_adpcm_table / 256
static const double XaFiltersOpposite[][2] = {
	{ 0,		 0 },	   { -0.9375,  0 },
	{ -1.796875, 0.8125 }, { -1.53125, 0.859375 }
};

void EaXaEncoder::encodeSubblock(int16_t* inputData, uint8_t* outputData, int nSamples, int16_t* outputAudioResult)
{
	double maxErrors[4];
	int chosenCoeff = 0;
	double predErrors[4][30];

	double minError = 1e21;

	int loopCurSample, loopPrevSample;

	for (int i = 0; i < 4; i++) {
		double maxAbsError = 0;
		loopCurSample = currentSample;
		loopPrevSample = previousSample;
		double* coefPredErrors = predErrors[i];
		for (int j = 0; j < nSamples; j++) {
			int nextSample = inputData[j];
			double predictionError = loopPrevSample * XaFiltersOpposite[i][1] + loopCurSample * XaFiltersOpposite[i][0] + nextSample;
			*coefPredErrors = predictionError;
			predictionError = std::abs(predictionError);
			if (maxAbsError < predictionError) {
				maxAbsError = predictionError;
			}
			loopPrevSample = loopCurSample;
			loopCurSample = nextSample;
			coefPredErrors++;
		}
		maxErrors[i] = maxAbsError;
		if (minError > maxAbsError) {
			minError = maxAbsError;
			chosenCoeff = i;
		}
		if (i == 0 && maxErrors[0] <= 7) {
			chosenCoeff = 0;
			break;
		}
	}

	currentSample = loopCurSample;
	previousSample = loopPrevSample;
	int maxError = std::min(30000, (int)maxErrors[chosenCoeff]);
	int testShift = 0x4000;
	uint8_t shift;
	for (shift = 0; shift < (uint8_t)12; shift++) {
		if (testShift & (maxError + (testShift >> 3))) break;
		testShift = testShift >> 1;
	}
	uint8_t coeffHint = chosenCoeff << 4;
	outputData[0] = (shift & 0x0F) | coeffHint;
	uint8_t* outIte = outputData + 1;
	double coefPrev = XaFiltersOpposite[chosenCoeff][1];
	double coeffCur = XaFiltersOpposite[chosenCoeff][0];
	double* predErrorsIte = predErrors[chosenCoeff];
	double shiftMul = 1 << shift;
	for (int i = 0; i < nSamples; i++) {
		double predWithQuantizError = coefPrev * previousQuantError + coeffCur * currentQuantError + *predErrorsIte;
		int quantValue = (lround(predWithQuantizError * shiftMul) + 0x800) & 0xfffff000;
		quantValue = clipInt16(quantValue);

		if (!(i & 1)) {
			*outIte = (uint8_t)(quantValue >> 8) & 0xF0;
		} else {
			*outIte = *outIte | ((uint8_t)(quantValue >> 12) & 0x0F);
			++outIte;
		}

		previousQuantError = currentQuantError;
		currentQuantError = (quantValue >> shift) - predWithQuantizError;

		if (outputAudioResult != nullptr) {
			outputAudioResult[i] = clipInt16(inputData[i] + lround(currentQuantError));
		}

		predErrorsIte++;
	}
}


void EaXaEncoder::writeUncompressedSubblock(int16_t* inputData, uint8_t* outputData, int nSamples, UncompressedType type)
{
	uint8_t* outIte = outputData;
	*outIte++ = 0xEE; // Mark uncompressed subblock

	int16_t* audioToWrite = inputData;
	int16_t transformedAudio[28];
	uint8_t outputEncoded[15];
	if (type == FadeToCompressed) { // nSamples = 28
		currentSample = previousSample = inputData[0];
		encodeSubblock(inputData, outputEncoded, 28, transformedAudio);
		for (int i = 0; i < 28; i++) {
			transformedAudio[i] = ((int)inputData[i] * (28 - i) + (int)transformedAudio[i] * i) / 28;
		}
		audioToWrite = transformedAudio;
		clearErrors();
	} else if (type == FadeFromCompressed) {
		encodeSubblock(inputData, outputEncoded, nSamples, transformedAudio);
		for (int i = 0; i < nSamples; i++) {
			transformedAudio[i] = ((int)transformedAudio[i] * (nSamples - i) + (int)inputData[i] * i) / nSamples;
		}
		audioToWrite = transformedAudio;
	}

	if (nSamples == 28) {
		currentSample = inputData[27];
		previousSample = inputData[26];
	} else {
		currentSample = previousSample = inputData[nSamples - 1];
	}

	bufferWrite16BEUnalign(outIte, currentSample);
	bufferWrite16BEUnalign(outIte, previousSample);

	int i;
	for (i = 0; i < nSamples; i++) {
		bufferWrite16BEUnalign(outIte, audioToWrite[i]);
	}
	for (; i < 28; i++) { // Fill the rest
		*outIte++ = 'E'; // "GEND"
		*outIte++ = 'G';
	}
}

void EaXaEncoder::clearErrors()
{
	currentQuantError = previousQuantError = 0;
}
