/*
 *    MIT License
 *
 *    Copyright (c) 2016 Błażej Szczygieł
 * 	  Copyright (c) 2026-present DaveTheEggman & contributors
 *
 *    Permission is hereby granted, free of charge, to any person obtaining a copy
 *    of this software and associated documentation files (the "Software"), to deal
 *    in the Software without restriction, including without limitation the rights
 *    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *    copies of the Software, and to permit persons to whom the Software is
 *    furnished to do so, subject to the following conditions:
 *
 *    The above copyright notice and this permission notice shall be included in all
 *    copies or substantial portions of the Software.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *    SOFTWARE.
 */
#include "include/decoder/OpusVorbisDecoder.hpp"
#include "include/decoder/VPXDecoder.hpp"

#include <mkvparser/mkvparser.h>

#include <cstdio>
#include <memory>

class MkvReader : public mkvparser::IMkvReader
{
public:
	explicit MkvReader(const char *filePath) : m_file(std::fopen(filePath, "rb"))
	{
	}

	~MkvReader()
	{
		if (m_file)
		{
			std::fclose(m_file);
		}
	}

	int Read(long long pos, long len, unsigned char *buf) override
	{
		if (!m_file)
		{
			return -1;
		}

		if (std::fseek(m_file, static_cast<long>(pos), SEEK_SET) != 0)
		{
			return -1;
		}

		const size_t size = std::fread(buf, 1, static_cast<size_t>(len), m_file);
		return size == static_cast<size_t>(len) ? 0 : -1;
	}

	int Length(long long *total, long long *available) override
	{
		if (!m_file)
		{
			return -1;
		}

		const long current_position = std::ftell(m_file);
		if (current_position < 0)
		{
			return -1;
		}

		if (std::fseek(m_file, 0, SEEK_END) != 0)
		{
			return -1;
		}

		const long file_size = std::ftell(m_file);
		if (file_size < 0)
		{
			std::fseek(m_file, current_position, SEEK_SET);
			return -1;
		}

		if (total)
		{
			*total = file_size;
		}

		if (available)
		{
			*available = file_size;
		}

		std::fseek(m_file, current_position, SEEK_SET);
		return 0;
	}

	bool isOpen() const
	{
		return m_file != nullptr;
	}

private:
	FILE *m_file = nullptr;
};

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		std::fprintf(stderr, "Usage: %s <file.webm>\n", argv[0]);
		return 1;
	}

	std::unique_ptr<MkvReader> reader(new MkvReader(argv[1]));

	if (!reader->isOpen())
	{
		std::fprintf(stderr, "Failed to open file: %s\n", argv[1]);
		return 1;
	}

	WebMDemuxer demuxer(reader.release());

	if (!demuxer.isOpen())
	{
		std::fprintf(stderr, "Failed to open WebM file: %s\n", argv[1]);
		return 1;
	}

	VPXDecoder videoDec(demuxer, 8);
	OpusVorbisDecoder audioDec(demuxer);

	std::fprintf(stderr, "File: %s\n", argv[1]);
	std::fprintf(stderr, "Length: %.3f seconds\n", demuxer.getLength());

	if (videoDec.isOpen())
	{
		std::fprintf(stderr, "Video decoder: opened\n");
	}
	else
	{
		std::fprintf(stderr, "Video decoder: unavailable\n");
	}

	if (audioDec.isOpen())
	{
		std::fprintf(stderr, "Audio decoder: opened\n");
	}
	else
	{
		std::fprintf(stderr, "Audio decoder: unavailable\n");
	}

	WebMFrame videoFrame;
	WebMFrame audioFrame;
	VPXDecoder::Image image;

	const int channels = demuxer.getChannels();
	const int bufferSamples = audioDec.getBufferSamples();

	std::unique_ptr<short[]> pcm;

	if (audioDec.isOpen() && channels > 0 && bufferSamples > 0)
	{
		pcm.reset(new short[bufferSamples * channels]);
	}

	unsigned int videoFrames = 0;
	unsigned int audioFrames = 0;
	unsigned int decodedImages = 0;

	bool success = true;

	while (demuxer.readFrame(&videoFrame, &audioFrame))
	{
		if (videoDec.isOpen() && videoFrame.isValid())
		{
			if (!videoDec.decode(videoFrame))
			{
				std::fprintf(stderr, "Video decode error\n");
				success = false;
				break;
			}

			++videoFrames;

			while (videoDec.getImage(image) == VPXDecoder::NO_ERROR)
			{
				++decodedImages;
			}
		}

		if (audioDec.isOpen() && audioFrame.isValid())
		{
			int numOutSamples = 0;

			if (!audioDec.getPCMS16(audioFrame, pcm.get(), numOutSamples))
			{
				std::fprintf(stderr, "Audio decode error\n");
				success = false;
				break;
			}

			++audioFrames;
		}
	}

	std::fprintf(stderr, "Decoded video frames: %u\n", videoFrames);
	std::fprintf(stderr, "Decoded video images: %u\n", decodedImages);
	std::fprintf(stderr, "Decoded audio frames: %u\n", audioFrames);

	return success ? 0 : 1;
}
