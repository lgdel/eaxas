#include "XasWriter.h"
#include "utility.h"
#include <stdint.h>
#include <iostream>
#include <fstream>

using namespace std;

uint8_t* XasWriter::writeHeader(int& rSize)
{
    uint8_t* data = new uint8_t[20];
    uint8_t* dataIte = data;

    *dataIte++ = 0x04; // XAS codec
    *dataIte++ = (input.nChannels - 1) * 4;
    bufferWriteValue<uint16_t>(dataIte, BSWAP16(input.sampleRate));
    bufferWriteValue<uint32_t>(dataIte, BSWAP32(input.nSamples | (isLoop << 29))); // total samples + flags
    if (isLoop) {
        bufferWriteValue<uint32_t>(dataIte, 0); // Starting part samples
    }

    offsetBlockSizeValue = dataIte - data; // first block size
    dataIte += 4;
    bufferWriteValue<uint32_t>(dataIte, BSWAP32(input.nSamples)); // first (unique) block samples

    rSize = dataIte - data;
    return data;
}

void XasWriter::writeBlockSizeInHeader(fstream& outFile)
{
    auto endOfBlockPos = outFile.tellg();
    outFile.seekg(offsetBlockSizeValue, ios::beg);
    uint32_t sizeBE = BSWAP32((uint32_t)endOfBlockPos - offsetBlockSizeValue);
    outFile.write((char*)&sizeBE, 4);
    outFile.seekg(endOfBlockPos);
}

void XasWriter::encodeXasBlock(int16_t* samples, uint8_t* &output, int nSamples)
{
    for (unsigned c = 0; c < input.nChannels; c++) {
        int16_t* inputSamples = samples + c * nSamples;
        int16_t inputSamplesPadded[128];
        if (nSamples < 128) {
            memset(inputSamplesPadded, 0, sizeof(inputSamplesPadded));
            memcpy(inputSamplesPadded, inputSamples, nSamples * 2);
            inputSamples = inputSamplesPadded;
        }
        int16_t startSamples[4][2];
        uint8_t encoded[4][16];
        EaXaEncoder& encoder = encoders[c];

        for (int i = 0; i < 4; i++) { // 128 samples split in 4 groups of 32
            startSamples[i][0] = clipInt16(inputSamples[0] + 8) & 0xFFF0;
            startSamples[i][1] = clipInt16(inputSamples[1] + 8) & 0xFFF0;
            encoder.previousSample = startSamples[i][0];
            encoder.currentSample = startSamples[i][1];
            encoder.clearErrors();
            encoder.encodeSubblock(inputSamples + 2, encoded[i], 30);
            inputSamples += 32;
        }

        int16_t* startSamplesWithInfos = (int16_t*)output;
        for (int i = 0; i < 4; i++) {
            uint8_t infoByte = encoded[i][0];
            *startSamplesWithInfos++ = startSamples[i][0] | (infoByte >> 4); // Coef info
            *startSamplesWithInfos++ = startSamples[i][1] | (infoByte & 0x0F); // Shift info
        }
        output = (uint8_t*)startSamplesWithInfos;
        for (int j = 1; j <= 15; j++) {
            for (int i = 0; i < 4; i++) {
                *output++ = encoded[i][j];
            }
        }
    }
}

bool XasWriter::writeFile(fstream& out)
{
    if (input.sampleRate > 0xFFFF) {
        cerr << "Sample rate is too big for this format." << endl;
        return false;
    }
    if (input.nSamples & 0xE0000000) {
        cerr << "Audio is too long for this format." << endl;
        return false;
    }

    int headerSize;
    unique_ptr<uint8_t[]> header(writeHeader(headerSize));
    out.write((char*)header.get(), headerSize);

    encoders.resize(input.nChannels);

    unsigned int codedSamples = 0;
    bool lastBlock = false;

    unique_ptr<uint8_t[]> block(new uint8_t[76 * input.nChannels]);
    unique_ptr<int16_t[]> samples(new int16_t[128 * input.nChannels]);

    while (!lastBlock) {
        int samplesInBlock = 128;
        codedSamples += samplesInBlock;
        if (codedSamples >= input.nSamples) {
            int toRemove = codedSamples - input.nSamples;
            samplesInBlock -= toRemove;
            codedSamples = input.nSamples;
            lastBlock = true;
        }

        if (!input.readSamples(samples.get(), samplesInBlock)) return false;

        uint8_t* dataIte = block.get();
        encodeXasBlock(samples.get(), dataIte, samplesInBlock);

        out.write((char*)block.get(), dataIte - block.get());
    }

    writeBlockSizeInHeader(out);

    return true;
}
