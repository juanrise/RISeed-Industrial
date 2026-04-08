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
#include "Lp1.h"
#include "ModulatedDelay.h"
#include "AllpassDiffuser.h"
#include "Biquad.h"

namespace Cloudseed
{
	class CircularBuffer
	{
		std::vector<float> buffer;
		int idxRead;
		int idxWrite;
		int count;
		int N;
	public:
		CircularBuffer(int capacity = 192000)
		{
			N = capacity;
			buffer.resize(N, 0.0f);
			Reset();
		}

		void Reset()
		{
			for (int i = 0; i < N; i++)
				buffer[i] = 0.0f;
			idxRead = 0;
			idxWrite = 0;
			count = 0;
		}

		int GetCount()
		{
			return count;
		}

		int PushZeros(float* data, int bufSize)
		{
			if (bufSize <= 0) return 0;
			int countBefore = count;
			for (int i = 0; i < bufSize; i++)
			{
				buffer[idxWrite] = 0.0f;
				idxWrite = (idxWrite + 1) % N;
				count++;
				if (count >= N)
					break; // overflow
			}

			return count - countBefore;
		}

		int Push(float* data, int bufSize)
		{
			if (bufSize <= 0) return 0;
			int countBefore = count;
			for (int i = 0; i < bufSize; i++)
			{
				buffer[idxWrite] = data[i];
				idxWrite = (idxWrite + 1) % N;
				count++;
				if (count >= N)
					break; // overflow
			}

			return count - countBefore;
		}

		int Pop(float* destination, int bufSize)
		{
			if (bufSize <= 0) return 0;
			int countBefore = count;
			for (int i = 0; i < bufSize; i++)
			{
				if (count > 0)
				{
					destination[i] = buffer[idxRead];
					idxRead = (idxRead + 1) % N;
					count--;
				}
				else
				{
					destination[i] = 0.0f;
				}
			}

			return countBefore - count;
		}
	};

	class DelayLine
	{
	private:
		ModulatedDelay delay;
		AllpassDiffuser diffuser;
		Biquad lowShelf;
		Biquad highShelf;
		Lp1 lowPass;
		CircularBuffer feedbackBuffer;
		float feedback;

		// Shimmer components
		static const int PITCH_BUF_SIZE = 8192;
		std::vector<float> pitchBuffer;
		std::vector<float> tempBuffer;
		int pitchWriteIdx = 0;
		float pitchPhase = 0.0f;
		float windowSize = 4096.0f; // Crossfade window

	public:
		bool DiffuserEnabled;
		bool LowShelfEnabled;
		bool HighShelfEnabled;
		bool CutoffEnabled;
		bool TapPostDiffuser;

		DelayLine() :
			feedbackBuffer(192000), // Default large allocation
			lowShelf(Biquad::FilterType::LowShelf, 48000),
			highShelf(Biquad::FilterType::HighShelf, 48000)
		{
			feedback = 0;

			pitchBuffer.resize(PITCH_BUF_SIZE, 0.0f);
			tempBuffer.resize(8192, 0.0f); // Pre-allocate larger than maximum expected bufSize

			lowShelf.SetGainDb(-20);
			lowShelf.Frequency = 20;

			highShelf.SetGainDb(-20);
			highShelf.Frequency = 19000;

			lowPass.SetCutoffHz(1000);
			lowShelf.Update();
			highShelf.Update();
			SetSamplerate(48000);
			SetDiffuserSeed(1, 0.0);
		}

		void SetSamplerate(int samplerate)
		{
			diffuser.SetSamplerate(samplerate);
			lowPass.SetSamplerate(samplerate);
			lowShelf.SetSamplerate(samplerate);
			highShelf.SetSamplerate(samplerate);
		}

		void SetDiffuserSeed(int seed, float crossSeed)
		{
			diffuser.SetSeed(seed);
			diffuser.SetCrossSeed(crossSeed);
		}

		void SetDelay(int delaySamples)
		{
			delay.SampleDelay = delaySamples;
		}

		void SetFeedback(float feedb)
		{
			feedback = feedb;
		}

		void SetDiffuserDelay(int delaySamples)
		{
			diffuser.SetDelay(delaySamples);
		}

		void SetDiffuserFeedback(float feedb)
		{
			diffuser.SetFeedback(feedb);
		}

		void SetDiffuserStages(int stages)
		{
			diffuser.Stages = stages;
		}

		void SetLowShelfGain(float gainDb)
		{
			lowShelf.SetGainDb(gainDb);
			lowShelf.Update();
		}

		void SetLowShelfFrequency(float frequency)
		{
			lowShelf.Frequency = frequency;
			lowShelf.Update();
		}

		void SetHighShelfGain(float gainDb)
		{
			highShelf.SetGainDb(gainDb);
			highShelf.Update();
		}

		void SetHighShelfFrequency(float frequency)
		{
			highShelf.Frequency = frequency;
			highShelf.Update();
		}

		void SetCutoffFrequency(float frequency)
		{
			lowPass.SetCutoffHz(frequency);
		}

		void SetLineModAmount(float amount)
		{
			delay.ModAmount = amount;
		}

		void SetLineModRate(float rate)
		{
			delay.ModRate = rate;
		}

		void SetDiffuserModAmount(float amount)
		{
			diffuser.SetModulationEnabled(amount > 0.0);
			diffuser.SetModAmount(amount);
		}

		void SetDiffuserModRate(float rate)
		{
			diffuser.SetModRate(rate);
		}

		void SetInterpolationEnabled(bool value)
		{
			diffuser.SetInterpolationEnabled(value);
		}

		void Process(float* input, float* output, int bufSize)
		{
			if (bufSize <= 0) return;

			if (tempBuffer.size() < bufSize) 
				tempBuffer.resize(bufSize, 0.0f);
				
			float* tBuf = tempBuffer.data();
			
			feedbackBuffer.Pop(tBuf, bufSize);

			for (int i = 0; i < bufSize; i++)
				tBuf[i] = input[i] + tBuf[i] * feedback;

			delay.Process(tBuf, tBuf, bufSize);
			
			if (!TapPostDiffuser)
				Utils::Copy(output, tBuf, bufSize);
			if (DiffuserEnabled)
				diffuser.Process(tBuf, tBuf, bufSize);
			if (LowShelfEnabled)
				lowShelf.Process(tBuf, tBuf, bufSize);
			if (HighShelfEnabled)
				highShelf.Process(tBuf, tBuf, bufSize);
			if (CutoffEnabled)
				lowPass.Process(tBuf, tBuf, bufSize);

			// --- Shimmer (+12st) and Hard Limit ---
			const float limitThreshold = 0.94406087f; // -0.5 dBFS roughly
			const float shiftRatio = 2.0f; // +12 semitones

			for (int i = 0; i < bufSize; ++i)
			{
				float x = tBuf[i];

				// Write to pitch buffer
				pitchBuffer[pitchWriteIdx] = x;
				pitchWriteIdx = (pitchWriteIdx + 1) % PITCH_BUF_SIZE;

				// Read from pitch buffer with 2 playheads crossfaded
				pitchPhase += (shiftRatio - 1.0f);
				if (pitchPhase >= windowSize)
					pitchPhase -= windowSize;

				float readIdx1 = pitchWriteIdx - pitchPhase;
				if (readIdx1 < 0) readIdx1 += PITCH_BUF_SIZE;
				
				float readIdx2 = readIdx1 - (windowSize * 0.5f);
				if (readIdx2 < 0) readIdx2 += PITCH_BUF_SIZE;

				auto interpolate = [&](float idx) {
					int idxInt = (int)idx;
					float frac = idx - idxInt;
					float y1 = pitchBuffer[idxInt % PITCH_BUF_SIZE];
					float y2 = pitchBuffer[(idxInt + 1) % PITCH_BUF_SIZE];
					return y1 + frac * (y2 - y1);
				};

				float y1 = interpolate(readIdx1);
				float y2 = interpolate(readIdx2);

				// Crossfade window: triangle or cos
				float crossfade = pitchPhase / windowSize; 
				float fade1 = crossfade * 2.0f;
				if (fade1 > 1.0f) fade1 = 2.0f - fade1; // Triangle wave 0 to 1 back to 0
				
				float fade2 = fmod(crossfade + 0.5f, 1.0f) * 2.0f;
				if (fade2 > 1.0f) fade2 = 2.0f - fade2; // 180 deg out of phase

				float shifted = y1 * fade1 + y2 * fade2;
				
				// Hard limiter clamped at -0.5 dB
				if (shifted > limitThreshold) shifted = limitThreshold;
				else if (shifted < -limitThreshold) shifted = -limitThreshold;

				tBuf[i] = shifted;
			}
			// --------------------------------------

			feedbackBuffer.Push(tBuf, bufSize);

			if (TapPostDiffuser)
				Utils::Copy(output, tBuf, bufSize);
		}

		void ClearDiffuserBuffer()
		{
			diffuser.ClearBuffers();
		}

		void ClearBuffers()
		{
			delay.ClearBuffers();
			diffuser.ClearBuffers();
			lowShelf.ClearBuffers();
			highShelf.ClearBuffers();
			lowPass.Output = 0;
			feedbackBuffer.Reset();
			
			for (int i = 0; i < PITCH_BUF_SIZE; ++i) pitchBuffer[i] = 0.0f;
			pitchWriteIdx = 0;
			pitchPhase = 0.0f;
		}
	};
}
