#ifndef OPUS_DECODER_H
#define OPUS_DECODER_H

#include "include/demuxer/WebMDemuxer.h"

struct OpusDecoderState;

class OpusDecoder
{
	OpusDecoder(const OpusDecoder &);
	void operator=(const OpusDecoder &);

public:
	explicit OpusDecoder(const WebMDemuxer &demuxer);
	~OpusDecoder();

	bool isOpen() const;

	inline int getBufferSamples() const
	{
		return m_numSamples;
	}

	bool getPCMS16(const WebMFrame &frame, short *buffer, int &numOutSamples);

private:
	bool open(const WebMDemuxer &demuxer);
	void close();

	OpusDecoderState *m_decoder;
	int m_numSamples;
	int m_channels;
};

#endif // OPUSDECODER_H