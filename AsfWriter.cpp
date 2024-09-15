#include "AsfWriter.h"
#include <iostream>
#include <memory>
#include <cstring>
#include <cstdlib>
#include "utility.h"

using namespace std;

struct SCBlockHead {
    char id[4];
    uint32_t size;
};

int writeHeaderTag(uint8_t* data, int32_t value) {
    int sizeNeeded;
    if (value > 127 || value < -128) {
        if (value > 32767 || value < -32768) {
            if (value > 8388352 || value < -8388608) sizeNeeded = 4;
            else sizeNeeded = 3;
        } else sizeNeeded = 2;
    } else sizeNeeded = 1;
    *data = sizeNeeded;
    uint32_t valueBE = BSWAP32(value);
    memcpy(data + 1, (char*)&valueBE + 4 - sizeNeeded, sizeNeeded);
    return sizeNeeded + 1;
}

uint8_t* AsfWriter::writeHeader(int& rSize)
{
    uint8_t* data = new uint8_t[64];
    uint8_t* dataIte = data;

    memcpy(&((SCBlockHead*)data)->id, "SCHl", 4);
    dataIte += sizeof(SCBlockHead);
    if (revision == 3) {
        memcpy(dataIte, "GSTR\x01\x00\x00\x00", 8); // GSTR with unknown version/platform id (1)
        dataIte += 8;
    } else {
        memcpy(dataIte, "PT\x00\x00", 4); // PT with 00 for PC platform
        dataIte += 4;
    }
    memcpy(dataIte, "\x06\x01\x65", 3); // unknown static tag of id 06
    dataIte += 3;
    if (input.nChannels != 1 && revision == 1) {
        *dataIte++ = 0x0B;
        *dataIte++ = 1;
        *dataIte++ = input.nChannels;
    }

    *dataIte++ = 0xFD; // Audio subheader

    *dataIte++ = 0x80;
    *dataIte++ = 1;
    *dataIte++ = revision;

    *dataIte++ = 0x85;
    dataIte += writeHeaderTag(dataIte, input.nSamples);

    if (input.nChannels != 1) {
        *dataIte++ = 0x82;
        *dataIte++ = 1;
        *dataIte++ = input.nChannels;
    }

    if ((revision < 3 && input.sampleRate != 22050) || (revision == 3 && input.sampleRate != 48000)) {
        *dataIte++ = 0x84;
        dataIte += writeHeaderTag(dataIte, input.sampleRate);
    }

    *dataIte++ = 0xFF; // End of SCHl header (then padding)

    int padding = 4 - (reinterpret_cast<uintptr_t>(dataIte) % 4);
    if (padding < 4) {
        memset(dataIte, 0, padding);
        dataIte += padding;
    }

    // SCHl size
    ((SCBlockHead*)data)->size = dataIte - data;

    memcpy(&((SCBlockHead*)dataIte)->id, "SCCl", 4);
    ((SCBlockHead*)dataIte)->size = 0x0C;
    dataIte += sizeof(SCBlockHead);
    offsetSCClValue = dataIte - data;
    dataIte += 4;

    rSize = dataIte - data;
    return data;
}

void AsfWriter::writeNumberOfBlocksInHeader(fstream& outFile, uint32_t nBlocks)
{
    auto previousPos = outFile.tellg();
    outFile.seekg(offsetSCClValue, ios::beg);
    if (revision == 3) nBlocks = BSWAP32(nBlocks);
    outFile.write((char*)&nBlocks, 4);
    outFile.seekg(previousPos);
}

uint8_t* AsfWriter::writeSCDlBlock(int nbSamples, int16_t* samples, bool firstBlock, int& rSize)
{
    auto dv = div(nbSamples, 28);
    int nCompleteSubblocks = dv.quot;
    int nSamplesExtraSubblock = dv.rem;
    uint8_t* output = new uint8_t[16 + input.nChannels * (184 + 15 + nCompleteSubblocks * 15)];
    uint8_t* dataIte = output;

    memcpy(&((SCBlockHead*)output)->id, "SCDl", 4);
    dataIte += sizeof(SCBlockHead);
    bufferWriteValue<uint32_t>(dataIte, revision == 3 ? BSWAP32(nbSamples) : nbSamples);

    int32_t* channelsOffsetsToData = (int32_t*)dataIte;
    dataIte += 4 * input.nChannels;
    uint8_t* channelBlocksStart = dataIte;

    for (unsigned c = 0; c < input.nChannels; c++) {
        channelsOffsetsToData[c] = revision == 3 ? BSWAP32(dataIte - channelBlocksStart) : (dataIte - channelBlocksStart);
        int16_t* channelSamples = samples + c * nbSamples;
        EaXaEncoder& encoder = encoders[c];

        if (revision == 1) {
            int16_t* predictionStartSamples = (int16_t*)dataIte;
            predictionStartSamples[0] = encoder.currentSample;
            predictionStartSamples[1] = encoder.previousSample;
            dataIte += 4;
            encoder.clearErrors();
        }

        int i = 0;
        if (firstBlock && revision > 1) {
            for (; i < 3 && i < nCompleteSubblocks; i++) {
                encoder.writeUncompressedSubblock(channelSamples, dataIte, 28, i == 2 && nCompleteSubblocks != 3 ? FadeToCompressed : Normal);
                channelSamples += 28;
                dataIte += 61;
            }
        }
        for (; i < nCompleteSubblocks; i++) {
            encoder.encodeSubblock(channelSamples, dataIte, 28);
            channelSamples += 28;
            dataIte += 15;
        }
        if (nSamplesExtraSubblock != 0) {
            if (revision == 1) {
                memset(dataIte, 0, 15);
                encoder.encodeSubblock(channelSamples, dataIte, nSamplesExtraSubblock);
                dataIte += 15;
            } else {
                encoder.writeUncompressedSubblock(channelSamples, dataIte, nSamplesExtraSubblock, nCompleteSubblocks <= 3 ? Normal : FadeFromCompressed);
                dataIte += 61;
            }
        }
        if (reinterpret_cast<uintptr_t>(dataIte) & 1) { // Padding
            *dataIte++ = 0;
        }
    }

    uint32_t SCDlSize = dataIte - output;
    if (SCDlSize & 2) { // Padding
        *((int16_t*)dataIte) = 0;
        SCDlSize += 2;
    }

    ((SCBlockHead*)output)->size = SCDlSize;
    rSize = SCDlSize;
    return output;
}

bool AsfWriter::writeFile(fstream& out)
{
    int headerSize;
    unique_ptr<uint8_t[]> header(writeHeader(headerSize));
    out.write((char*)header.get(), headerSize);

    encoders.resize(input.nChannels);

    unique_ptr<uint8_t[]> block;
    unique_ptr<int16_t[]> samples;
    unsigned int codedSamples = 0;
    int blockIndex = 1;
    bool lastBlock = false;
    int blocksPerSecond = revision == 3 ? 5 : 15;

    while (!lastBlock) {
        unsigned int samplesInBlock = ((blockIndex * input.sampleRate) / blocksPerSecond) - codedSamples;
        int modulo28 = samplesInBlock % 28;
        if (modulo28 != 0) {
            samplesInBlock = samplesInBlock + 28 - modulo28;
        }
        codedSamples += samplesInBlock;
        if (codedSamples >= input.nSamples) {
            int toRemove = codedSamples - input.nSamples;
            samplesInBlock -= toRemove;
            codedSamples = input.nSamples;
            lastBlock = true;
        }

        samples.reset(new int16_t[samplesInBlock * input.nChannels]);
        if (!input.readSamples(samples.get(), samplesInBlock)) return false;

        if (blockIndex == 1 && revision == 1) {
            for (unsigned c = 0; c < input.nChannels; c++) {
                encoders[c].currentSample = encoders[c].previousSample = samples[c * samplesInBlock];
            }
        }

        int blockSize;
        block.reset(writeSCDlBlock(samplesInBlock, samples.get(), blockIndex == 1, blockSize));
        out.write((char*)block.get(), blockSize);

        blockIndex++;
    }

    writeNumberOfBlocksInHeader(out, blockIndex - 1);

    out.write("SCEl\x08\x00\x00\x00", 8); // EOF block

    return true;
}
