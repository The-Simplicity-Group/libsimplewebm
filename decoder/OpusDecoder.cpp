#include "include/decoder/OpusDecoder.h"

#include <opus/opus.h>

struct OpusDecoderState
{
	::OpusDecoder *decoder;
};

OpusDecoder::OpusDecoder(const WebMDemuxer &demuxer) : m_decoder(NULL), m_numSamples(0), m_channels(demuxer.getChannels())
{
	if (!open(demuxer))
		close();
}

OpusDecoder::~OpusDecoder()
{
	close();
}

bool OpusDecoder::isOpen() const
{
	return m_decoder != NULL;
}

bool OpusDecoder::getPCMS16(const WebMFrame &frame, short *buffer, int &numOutSamples)
{
	numOutSamples = 0;

	if (!m_decoder || !m_decoder->decoder || !buffer || frame.bufferSize <= 0)
		return false;

	const int samples = opus_decode(m_decoder->decoder, frame.buffer, frame.bufferSize, buffer, m_numSamples, 0);

	if (samples < 0)
		return false;

	numOutSamples = samples;
	return true;
}

bool OpusDecoder::open(const WebMDemuxer &demuxer)
{
	int opusError = OPUS_OK;

	::OpusDecoder *decoder = opus_decoder_create(static_cast<opus_int32>(demuxer.getSampleRate()), m_channels, &opusError);

	if (!decoder || opusError != OPUS_OK)
	{
		if (decoder)
			opus_decoder_destroy(decoder);

		return false;
	}

	m_decoder = new OpusDecoderState;
	m_decoder->decoder = decoder;

	// Maximum Opus frame duration is 60 ms.
	m_numSamples = static_cast<int>(demuxer.getSampleRate() * 0.06 + 0.5);

	return true;
}

void OpusDecoder::close()
{
	if (!m_decoder)
		return;

	if (m_decoder->decoder)
	{
		opus_decoder_destroy(m_decoder->decoder);
		m_decoder->decoder = NULL;
	}

	delete m_decoder;
	m_decoder = NULL;
}