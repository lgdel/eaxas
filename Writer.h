#pragma once
#include <stdint.h>
#include <fstream>
#include <vector>
#include "WavParser.h"
#include "EaXaEncoder.h"

using namespace std;

class Writer
{
public:
	Writer(WavParser& inputWav): input(inputWav) {}

	virtual bool writeFile(fstream& out) = 0;

protected:
	WavParser& input;
	vector<EaXaEncoder> encoders;
};

