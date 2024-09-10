#include <stdint.h>
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include "WavParser.h"
#include "AsfWriter.h"
#include "MaxisXaWriter.h"
#include "XasWriter.h"

using namespace std;
namespace fs = std::filesystem;

#define VERSION "1.0.0"

#define OPTION_HELP "-h"
#define OPTION_R1 "--xa-r1"
#define OPTION_R2 "--xa-r2"
#define OPTION_R3 "--xa-r3"
#define OPTION_MAXIS_XA "--maxis-xa"
#define OPTION_XAS "--xas"
#define OPTION_XAS_LOOP "--loop"
#define OPTION_OUTPUT_FILE "-o"

enum WriterType {
    Unknown,
    Asf,
    MaxisXa,
    Xas
};


void printHelp() {
    cout << "EaXas " VERSION << endl;
    cout << endl;
    cout << "Usage: eaxas InputWavFile [Options]" << endl;
    cout << endl;
    cout << "Options:" << endl;
    cout << "   " OPTION_HELP "\t\t\t Show this help" << endl;
    cout << "   " OPTION_OUTPUT_FILE " OutputFile\t Specify output filepath (optional)" << endl;
    cout << "   " OPTION_R1 "\t\t Output to EA XA ADPCM R1 (\"SCxl\" file)" << endl;
    cout << "   " OPTION_R2 "\t\t Output to EA XA ADPCM R2" << endl;
    cout << "   " OPTION_R3 "\t\t Output to EA XA ADPCM R3" << endl;
    cout << "   " OPTION_MAXIS_XA "\t\t Output to EA Maxis CDROM XA ADPCM" << endl;
    cout << "   " OPTION_XAS "\t\t Output to EA XAS ADPCM (snr/cdata file)" << endl;
    cout << "   " OPTION_XAS_LOOP "\t\t For XAS only: mark the file as loop" << endl;
}

int main(int Argc, char** Argv)
{
    bool showHelp = Argc <= 1;
    fs::path inputPath, outputPath;
    int asfRevision;
    bool xasLoop = false;
    WriterType writerType = Unknown;

    for (int a = 1; a < Argc; a++) {
        string arg(Argv[a]);

        if (arg[0] == '-') {
            if (arg == OPTION_HELP) {
                showHelp = true;
                break;
            }
            if (arg == OPTION_R1) {
                asfRevision = 1;
                writerType = Asf;
                continue;
            }
            if (arg == OPTION_R2) {
                asfRevision = 2;
                writerType = Asf;
                continue;
            }
            if (arg == OPTION_R3) {
                asfRevision = 3;
                writerType = Asf;
                continue;
            }
            if (arg == OPTION_MAXIS_XA) {
                writerType = MaxisXa;
                continue;
            }
            if (arg == OPTION_XAS) {
                writerType = Xas;
                continue;
            }
            if (arg == OPTION_XAS_LOOP) {
                xasLoop = true;
                continue;
            }
            if (arg == OPTION_OUTPUT_FILE) {
                a++;
                if (a == Argc) {
                    cerr << "No output file given with option" << endl;
                    return 1;
                }
                outputPath = Argv[a];
                continue;
            }

            cerr << "Unknown option: " << arg << endl;
            return 1;
        } else {
            if (inputPath.empty()) {
                inputPath = arg;
            }
        }
    }

    if (showHelp) {
        printHelp();
        return 0;
    }

    if (writerType == Unknown) {
        cerr << "No format chosen. See help with option -h." << endl;
        return 1;
    }

    if (inputPath.empty()) {
        cerr << "No input file given" << endl;
        return 1;
    }
    fstream inFile;
    inFile.open(inputPath, ios::in | ios::binary);
    if (!inFile.good()) {
        cerr << "Could not open input file " << inputPath << endl;
        perror(nullptr);
        return 1;
    }
    cout << "Input file: " << inputPath.filename() << endl;

    WavParser wavPa(inFile);
    if (!wavPa.initFile()) {
        return 1;
    }

    if (outputPath.empty()) {
        outputPath = inputPath;
        const char* outExt = "asf";
        if (writerType == MaxisXa) outExt = "xa";
        else if (writerType == Xas) outExt = "snr";
        outputPath.replace_extension(outExt);
    }
    fstream outFile;
    outFile.open(outputPath, ios::out | ios::binary);
    if (!outFile.good()) {
        cerr << "Could not create output file " << outputPath << endl;
        perror(nullptr);
        return 1;
    }
    cout << "Output file: " << outputPath.filename() << endl;

    bool successEncoding;
    switch (writerType) {
        case Asf: {
            AsfWriter writer(asfRevision, wavPa);
            successEncoding = writer.writeFile(outFile);
            break;
            }
        case MaxisXa: {
            MaxisXaWriter writer(wavPa);
            successEncoding = writer.writeFile(outFile);
            break;
            }
        default: {
            XasWriter writer(xasLoop, wavPa);
            successEncoding = writer.writeFile(outFile);
            }
    }

    if (!successEncoding) {
        if (!inFile.good()) cerr << "Error reading input file" << endl;
        return 1;
    }

    if (!outFile.good()) {
        cerr << "Error writing to output file" << endl;
        return 1;
    }

    cout << "Done." << endl;

    return 0;
}
