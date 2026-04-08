/*
Copyright (c) 2024 Ghost Note Engineering Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#pragma once

#include <vector>
#include "../Parameters.h"
#include "ReverbChannel.h"
#include "AllpassDiffuser.h"
#include "MultitapDelay.h"
#include "Utils.h"

namespace Cloudseed
{
	class ReverbController
	{
	private:
		int samplerate;

		ReverbChannel channelL;
		ReverbChannel channelR;
		double parameters[(int)Parameter::COUNT] = {0};

		std::vector<float> outLTemp;
		std::vector<float> outRTemp;
		std::vector<float> leftChannelIn;
		std::vector<float> rightChannelIn;

	public:
		ReverbController(int samplerate) :
			channelL(samplerate, ChannelLR::Left),
			channelR(samplerate, ChannelLR::Right)
		{
			this->samplerate = samplerate;
			outLTemp.resize(8192, 0.0f);
			outRTemp.resize(8192, 0.0f);
			leftChannelIn.resize(8192, 0.0f);
			rightChannelIn.resize(8192, 0.0f);
		}

		int GetSamplerate()
		{
			return samplerate;
		}

		void SetSamplerate(int samplerate)
		{
			this->samplerate = samplerate;
			channelL.SetSamplerate(samplerate);
			channelR.SetSamplerate(samplerate);
		}

		int GetParameterCount()
		{
			return Parameter::COUNT;
		}

		double* GetAllParameters()
		{
			return parameters;
		}

		void SetParameter(int paramId, double value)
		{
			parameters[paramId] = value;
			auto scaled = ScaleParam(value, paramId);
			channelL.SetParameter(paramId, scaled);
			channelR.SetParameter(paramId, scaled);
		}

		void ClearBuffers()
		{
			channelL.ClearBuffers();
			channelR.ClearBuffers();
		}

		void Process(float* inL, float* inR, float* outL, float* outR, int bufSize)
		{
			if (outLTemp.size() < bufSize) outLTemp.resize(bufSize, 0.0f);
			if (outRTemp.size() < bufSize) outRTemp.resize(bufSize, 0.0f);
			
			float* lT = outLTemp.data();
			float* rT = outRTemp.data();

			while (bufSize > 0)
			{
				int subBufSize = bufSize > BUFFER_SIZE ? BUFFER_SIZE : bufSize;
				ProcessChunk(inL, inR, lT, rT, subBufSize);
				Utils::Copy(outL, lT, subBufSize);
				Utils::Copy(outR, rT, subBufSize);
				inL = &inL[subBufSize];
				inR = &inR[subBufSize];
				outL = &outL[subBufSize];
				outR = &outR[subBufSize];
				bufSize -= subBufSize;
			}
		}

	private:
		void ProcessChunk(float* inL, float* inR, float* outL, float* outR, int bufSize)
		{
			if (leftChannelIn.size() < bufSize) leftChannelIn.resize(bufSize, 0.0f);
			if (rightChannelIn.size() < bufSize) rightChannelIn.resize(bufSize, 0.0f);
			
			float* lIn = leftChannelIn.data();
			float* rIn = rightChannelIn.data();

			float inputMix = ScaleParam(parameters[Parameter::InputMix], Parameter::InputMix);
			float cm = inputMix * 0.5;
			float cmi = (1 - cm);

			for (int i = 0; i < bufSize; i++)
			{
				lIn[i] = inL[i] * cmi + inR[i] * cm;
				rIn[i] = inR[i] * cmi + inL[i] * cm;
			}

			channelL.Process(lIn, outL, bufSize);
			channelR.Process(rIn, outR, bufSize);
		}
	};
}
