#include "MaxisXaWriter.h"
#include <iostream>
#include <cstring>
#include <memory>
#include "utility.h"

using namespace std;

uint8_t* MaxisXaWriter::writeHeader()
{
    uint8_t* data = new uint8_t[24];
    uint8_t* dataIte = data;

    memcpy(dataIte, "XA\x00\x00", 4); // leaving the XA tag as 00
    dataIte += 4;

    bufferWriteValue<uint32_t>(dataIte, input.nSamples * 2 * input.nChannels); // Uncompressed data size
    bufferWriteValue<uint16_t>(dataIte, 1); // wFormatTag (PCM)
    bufferWriteValue<uint16_t>(dataIte, input.nChannels);
    bufferWriteValue<uint32_t>(dataIte, input.sampleRate);
    bufferWriteValue<uint32_t>(dataIte, input.nChannels * input.sampleRate * 2); // nAvgBytesPerSec
    bufferWriteValue<uint16_t>(dataIte, input.nChannels * 2); // nBlockAlign
    bufferWriteValue<uint16_t>(dataIte, 16); // wBitsPerSample

    return data;
}

void MaxisXaWriter::encodeAndInterleave(int16_t* samples, uint8_t* output, int nSamples)
{
    int nBytes = 1 + (nSamples + 1) / 2;
    for (unsigned c = 0; c < input.nChannels; c++) {
        int16_t* inputSamples = samples + c * nSamples;
        uint8_t encoded[15];
        encoders[c].encodeSubblock(inputSamples, encoded, nSamples);
        for (int b = 0; b < nBytes; b++) { // Interleave
            output[c + b * input.nChannels] = encoded[b];
        }
    }
}

bool MaxisXaWriter::writeFile(fstream& out)
{
    unique_ptr<uint8_t[]> header(writeHeader());
    out.write((char*)header.get(), 24);

    encoders.resize(input.nChannels);

    unsigned int codedSamples = 0;
    bool lastBlock = false;
    int blockSize = 15 * input.nChannels;

    unique_ptr<uint8_t[]> block(new uint8_t[blockSize]);
    unique_ptr<int16_t[]> samples(new int16_t[28 * input.nChannels]);

    while (!lastBlock) {
        int samplesInBlock = 28;
        codedSamples += samplesInBlock;
        if (codedSamples >= input.nSamples) {
            int toRemove = codedSamples - input.nSamples;
            samplesInBlock -= toRemove;
            codedSamples = input.nSamples;
            lastBlock = true;
        }

        if (!input.readSamples(samples.get(), samplesInBlock)) return false;

        if (samplesInBlock < 28) memset(block.get(), 0, blockSize);
        encodeAndInterleave(samples.get(), block.get(), samplesInBlock);

        out.write((char*)block.get(), blockSize);
    }

    return true;
}
