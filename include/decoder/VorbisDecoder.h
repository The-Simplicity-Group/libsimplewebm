#ifndef VORBIS_DECODER_H
#define VORBIS_DECODER_H

#include "include/demuxer/WebMDemuxer.h"

struct VorbisDecoderState;

class VorbisDecoder
{
	VorbisDecoder(const VorbisDecoder &);
	void operator=(const VorbisDecoder &);

public:
	explicit VorbisDecoder(const WebMDemuxer &demuxer);
	~VorbisDecoder();

	bool isOpen() const;

	inline int getBufferSamples() const
	{
		return m_numSamples;
	}

	bool getPCMS16(const WebMFrame &frame, short *buffer, int &numOutSamples);

private:
	bool open(const WebMDemuxer &demuxer);
	void close();

	VorbisDecoderState *m_decoder;
	int m_numSamples;
	int m_channels;
};

#endif // VORBISDECODER_H