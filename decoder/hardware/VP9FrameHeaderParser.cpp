/*
 *    MIT License
 *
 *    Copyright (c) 2016 Błażej Szczygieł
 * 	  Copyright (c) 2026-present The Simplicity Group & contributors
 * 	  See AUTHORS.md for more information
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

/*
 *    This file was ported by Joyless from vp9-parser (Rust) by Nils Hasenbanck.
 *
 *    Link:
 *    https://github.com/hasenbanck/vp9-parser/blob/master/src/lib.rs
 *
 *    License:
 *    Copyright (c) 2021 Nils Hasenbanck
 *
 *    Permission is hereby granted, free of charge, to any
 *    person obtaining a copy of this software and associated
 *    documentation files (the "Software"), to deal in the
 *    Software without restriction, including without
 *    limitation the rights to use, copy, modify, merge,
 *    publish, distribute, sublicense, and/or sell copies of
 *    the Software, and to permit persons to whom the Software
 *    is furnished to do so, subject to the following
 *    conditions:
 *
 *    The above copyright notice and this permission notice
 *    shall be included in all copies or substantial portions
 *    of the Software.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
 *    ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 *    TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 *    PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
 *    SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *    OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 *    IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *    DEALINGS IN THE SOFTWARE.
 */

#include "include/decoder/hardware/VP9FrameHeaderParser.h"

using namespace VP9FrameHeaderParser;

BitReader::BitReader(Slice<uint8_t> slice)
{
    this->data = slice.ptr;
    this->sizeInBits = slice.len * 8;
}

Result<uint64_t> BitReader::readU64(uint8_t count)
{
    return readUnsigned<uint64_t>(count);
}

Result<uint32_t> BitReader::readU32(uint8_t count)
{
    return readUnsigned<uint32_t>(count);
}

Result<uint16_t> BitReader::readU16(uint8_t count)
{
    return readUnsigned<uint16_t>(count);
}

Result<uint8_t> BitReader::readU8(uint8_t count)
{
    return readUnsigned<uint8_t>(count);
}

Result<bool> BitReader::readBool()
{
    Result<uint8_t> result = readU8(1);
    if (result.error)
    {
        return { result.error };
    }

    return { OK, result.value != 0 };
}

Result<int8_t> BitReader::readInverseI8(uint8_t count)
{
    return readInverseSigned<int8_t, uint8_t>(count);
}

Result<int16_t> BitReader::readInverseI16(uint8_t count)
{
    return readInverseSigned<int16_t, uint16_t>(count);
}

size_t BitReader::getRemainingBits() const
{
    return this->sizeInBits - this->positionInBits;
}

bool BitReader::isAligned(uint32_t alignmentBytes) const
{
    return this->positionInBits % (static_cast<uint64_t>(alignmentBytes) * 8) == 0;
}

template <typename T>
Result<T> BitReader::readUnsigned(uint8_t count)
{
    if (count > 64)
    {
        return { ERROR_BIT_READER };
    }

    if (count > getRemainingBits())
    {
        return { ERROR_BIT_READER };
    }

    T value = 0;

    for (uint8_t i = 0; i < count; ++i)
    {
        size_t byteIndex = this->positionInBits / 8;
        uint8_t byte = this->data[byteIndex];
        T shift = 7 - (this->positionInBits % 8);
        T bit = static_cast<T>(byte >> shift) & 1;
        value = (value << 1) | bit;

        ++this->positionInBits;
    }

    return {OK, value};
}

template <typename T, typename U>
Result<T> BitReader::readInverseSigned(uint8_t count)
{
    // The sign bit is at the start and not the end (even though it's BE).

    Result<U> result = readUnsigned<U>(count);
    if (result.error)
    {
        return { result.error };
    }
    T value = static_cast<T>(result.value);

    Result<bool> negativeResult = readBool();
    if (negativeResult.error)
    {
        return { negativeResult.error };
    }
    bool negative = negativeResult.value;

    if (negative)
    {
        return { OK, static_cast<T>(-value) };
    }
    else
    {
        return { OK, value };
    }
}

constexpr PROFILE VP9FrameHeaderParser::u8ToProfile(uint8_t value)
{
    switch (value)
    {
        case 0:
            return PROFILE_0;
        case 1:
            return PROFILE_1;
        case 2:
            return PROFILE_2;
        case 3:
            return PROFILE_3;
        default:
            return PROFILE_UNKNOWN;
    }
}

constexpr METADATA_SUBSAMPLING VP9FrameHeaderParser::u8ToMetadataSubsampling(uint8_t value)
{
    switch (value)
    {
        case 0:
            return METADATA_SUBSAMPLING_YUV_420;
        case 1:
            return METADATA_SUBSAMPLING_YUV_420_COLOCATED;
        case 2:
            return METADATA_SUBSAMPLING_YUV_422;
        case 3:
            return METADATA_SUBSAMPLING_YUV_444;
        default:
            return METADATA_SUBSAMPLING_UNKNOWN;
    }
}

constexpr COLOR_SPACE VP9FrameHeaderParser::u8ToColorSpace(uint8_t value)
{
    switch (value)
    {
        case 1:
            return COLOR_SPACE_BT601;
        case 2:
            return COLOR_SPACE_BT709;
        case 3:
            return COLOR_SPACE_SMPTE170;
        case 4:
            return COLOR_SPACE_SMPTE240;
        case 5:
            return COLOR_SPACE_BT2020;
        case 6:
            return COLOR_SPACE_RESERVED;
        case 7:
            return COLOR_SPACE_RGB;
        default:
            return COLOR_SPACE_UNKNOWN;
    }
}

constexpr COLOR_DEPTH VP9FrameHeaderParser::u8ToColorDepth(uint8_t value)
{
    switch (value)
    {
        case 8:
            return COLOR_DEPTH_8;
        case 10:
            return COLOR_DEPTH_10;
        case 12:
            return COLOR_DEPTH_12;
        default:
            return COLOR_DEPTH_UNKNOWN;
    }
}

constexpr COLOR_RANGE VP9FrameHeaderParser::boolToColorRange(bool value)
{
    switch (value)
    {
        case false:
            return COLOR_RANGE_STUDIO_SWING;
        case true:
            return COLOR_RANGE_FULL_SWING;
    }
}

constexpr FRAME_TYPE VP9FrameHeaderParser::boolToFrameType(bool value)
{
    switch (value)
    {
        case false:
            return FRAME_TYPE_KEY_FRAME;
        case true:
            return FRAME_TYPE_NON_KEY_FRAME;
    }
}

constexpr RESET_FRAME_CONTEXT VP9FrameHeaderParser::u8ToResetFrameContext(uint8_t value)
{
    switch (value)
    {
        case 0:
            return RESET_FRAME_CONTEXT_NO_0;
        case 1:
            return RESET_FRAME_CONTEXT_NO_1;
        case 2:
            return RESET_FRAME_CONTEXT_SINGLE_RESET;
        case 3:
            return RESET_FRAME_CONTEXT_FULL_RESET;
        default:
            return RESET_FRAME_CONTEXT_UNKNOWN;
    }
}

constexpr LEVEL VP9FrameHeaderParser::u8ToLevel(uint8_t value)
{
    switch (value)
    {
        case 10:
            return LEVEL_1;
        case 11:
            return LEVEL_1_1;
        case 20:
            return LEVEL_2;
        case 21:
            return LEVEL_2_1;
        case 30:
            return LEVEL_3;
        case 31:
            return LEVEL_3_1;
        case 40:
            return LEVEL_4;
        case 41:
            return LEVEL_4_1;
        case 50:
            return LEVEL_5;
        case 51:
            return LEVEL_5_1;
        case 52:
            return LEVEL_5_2;
        case 60:
            return LEVEL_6;
        case 61:
            return LEVEL_6_1;
        case 62:
            return LEVEL_6_2;
        default:
            return LEVEL_UNKNOWN;
    }
}

Result<Metadata> Metadata::create(Slice<uint8_t> data)
{
    Metadata metadata;

    bool hasProfile = false;
    bool hasLevel = false;
    bool hasColorDepth = false;
    bool hasChromaSubsampling = false;

    int32_t pos = 0;

    while (pos < data.len)
    {
        if (pos + 1 >= data.len)
        {
            return { ERROR_INVALID_METADATA };
        }

        uint8_t id = data.ptr[pos];
        uint8_t value = data.ptr[pos + 1];
        pos += 2;

        switch (id)
        {
            case 1:
                hasProfile = true;
                metadata.profile = u8ToProfile(value);
                break;
            case 2:
                hasLevel = true;
                metadata.level = u8ToLevel(value);
                break;
            case 3:
                hasColorDepth = true;
                metadata.colorDepth = u8ToColorDepth(value);
                break;
            case 4:
                hasChromaSubsampling = true;
                metadata.chromaSubsampling = u8ToMetadataSubsampling(value);
                break;
        }
    }

    if (!hasProfile || !hasLevel || !hasColorDepth || !hasChromaSubsampling)
    {
        return { ERROR_INVALID_METADATA };
    }

    return { OK, metadata };
}

Frame Frame::create(VP9Parser &parser, size_t uncompressedHeaderSize, size_t compressedHeaderSize, size_t tileSize, std::vector<uint8_t> data)
{
    Frame frame;

    frame.data = data;
    frame.profile = parser.profile;
    frame.showExistingFrame = parser.showExistingFrame;
    frame.frameToShowMapIdx = parser.frameToShowMapIdx;
    frame.lastFrameType = parser.lastFrameType;
    frame.frameType = parser.frameType;
    frame.showFrame = parser.showFrame;
    frame.errorResilientMode = parser.errorResilientMode;
    frame.intraOnly = parser.intraOnly;
    frame.resetFrameContext = parser.resetFrameContext;
    memcpy(frame.refFrameIndices, parser.refFrameIndices, sizeof(parser.refFrameIndices));
    memcpy(frame.refFrameSignBias, parser.refFrameSignBias, sizeof(parser.refFrameSignBias));
    frame.allowHighPrecisionMv = parser.allowHighPrecisionMv;
    frame.refreshFrameContext = parser.refreshFrameContext;
    frame.refreshFrameFlags = parser.refreshFrameFlags;
    frame.frameParallelDecodingMode = parser.frameParallelDecodingMode;
    frame.frameContextIdx = parser.frameContextIdx;
    frame.uncompressedHeaderSize = uncompressedHeaderSize;
    frame.compressedHeaderSize = compressedHeaderSize;
    frame.tileSize = tileSize;
    frame.colorDepth = parser.colorDepth;
    frame.colorSpace = parser.colorSpace;
    frame.colorRange = parser.colorRange;
    frame.subsamplingX = parser.subsamplingX;
    frame.subsamplingY = parser.subsamplingY;
    frame.width = parser.width;
    frame.height = parser.height;
    frame.renderWidth = parser.renderWidth;
    frame.renderHeight = parser.renderHeight;
    frame.miCols = parser.miCols;
    frame.miRows = parser.miRows;
    frame.tileRowsLog2 = parser.tileRowsLog2;
    frame.tileColsLog2 = parser.tileColsLog2;
    frame.interpolationFilter = parser.interpolationFilter;
    frame.loopFilterLevel = parser.loopFilterLevel;
    frame.loopFilterSharpness = parser.loopFilterSharpness;
    frame.loopFilterDeltaEnabled = parser.loopFilterDeltaEnabled;
    frame.updateRefDelta = parser.updateRefDelta;
    memcpy(frame.loopFilterRefDeltas, parser.loopFilterRefDeltas, sizeof(parser.loopFilterRefDeltas));
    frame.updateModeDelta = parser.updateModeDelta;
    memcpy(frame.loopFilterModeDeltas, parser.loopFilterModeDeltas, sizeof(parser.loopFilterModeDeltas));
    frame.baseQIdx = parser.baseQIdx;
    frame.deltaQYDc = parser.deltaQYDc;
    frame.deltaQUvDc = parser.deltaQUvDc;
    frame.deltaQUvAc = parser.deltaQUvAc;
    frame.lossless = parser.lossless;
    frame.segmentationEnabled = parser.segmentationEnabled;
    frame.segmentationUpdateMap = parser.segmentationUpdateMap;
    memcpy(frame.segmentTreeProbs, parser.segmentTreeProbs, sizeof(parser.segmentTreeProbs));
    memcpy(frame.segmentPredProbs, parser.segmentPredProbs, sizeof(parser.segmentPredProbs));
    frame.segmentationTemporalUpdate = parser.segmentationTemporalUpdate;
    frame.segmentationUpdateData = parser.segmentationUpdateData;
    frame.segmentationAbsOrDeltaUpdate = parser.segmentationAbsOrDeltaUpdate;
    memcpy(frame.segmentFeatureEnabled, parser.segmentFeatureEnabled, sizeof(parser.segmentFeatureEnabled));
    memcpy(frame.segmentFeatureData, parser.segmentFeatureData, sizeof(parser.segmentFeatureData));

    return frame;
}

Slice<uint8_t> Frame::compressedHeaderData()
{
    return {
        this->data.data() + this->uncompressedHeaderSize,
        this->compressedHeaderSize,
    };
}

Slice<uint8_t> Frame::compressedHeaderAndTileData()
{
    return {
        this->data.data() + this->uncompressedHeaderSize,
        this->data.size() - this->uncompressedHeaderSize,
    };
}

Slice<uint8_t> Frame::tileData()
{
    return {
        this->data.data() + this->uncompressedHeaderSize + this->compressedHeaderSize,
        this->data.size() - this->uncompressedHeaderSize - this->compressedHeaderSize,
    };
}

uint8_t Frame::lastRefFrameIndex()
{
    return this->refFrameIndices[0];
}

uint8_t Frame::goldenRefFrameIndex()
{
    return this->refFrameIndices[1];
}

uint8_t Frame::altRefFrameIndex()
{
    return this->refFrameIndices[2];
}

SUBSAMPLING Frame::subsampling()
{
    if (!this->subsamplingX && !this->subsamplingY)
    {
        return SUBSAMPLING_YUV_444;
    }
    else if (!this->subsamplingX && this->subsamplingY)
    {
        return SUBSAMPLING_YUV_440;
    }
    else if (this->subsamplingX && !this->subsamplingY)
    {
        return SUBSAMPLING_YUV_422;
    }
    else
    {
        return SUBSAMPLING_YUV_420;
    }
}

Result<std::vector<Frame>> VP9Parser::parsePacket(std::vector<uint8_t> packet)
{
    if (packet.empty())
    {
        return { OK, {} };
    }

    // Test for a super frame.
    size_t lastByteIndex = packet.size() - 1;
    uint8_t lastByte = packet[lastByteIndex];
    if (lastByte & 0b1110'0000 == 0b1100'0000)
    {
        uint8_t bytesPerFramesizeMinus1 = (lastByte & 0b11000) >> 3;
        uint8_t framesInSuperframeMinus1 = lastByte & 0b111;
        size_t bytesSize = bytesPerFramesizeMinus1 + 1;
        size_t frameCount = framesInSuperframeMinus1 + 1;
        size_t indexSize = 2 + (frameCount * bytesSize);
        size_t firstByteIndex = packet.size() - indexSize;
        uint8_t firstByte = packet[firstByteIndex];

        // Found a super frame.
        if (firstByte == lastByte)
        {
            std::vector<Frame> frames = {};
            frames.reserve(frameCount);

            size_t indexStart = firstByteIndex + 1;
            size_t entrySize = frameCount * bytesSize;

            std::vector<uint8_t> entryData = {};
            entryData.reserve(entrySize);
            entryData.insert(entryData.end(), packet.begin() + indexStart, packet.begin() + indexStart + entrySize);

            switch (frameCount)
            {
                case 1:
                {
                    // Odd, but valid bitstream configuration.
                    Result<size_t> frameSizeResult = readFrameSize({ entryData.data(), entryData.size() }, bytesSize, 0);
                    if (frameSizeResult.error)
                    {
                        return { frameSizeResult.error };
                    }

                    packet.resize(frameSizeResult.value);
                    Result<Frame> frameResult = parseFrame(packet);
                    if (frameResult.error)
                    {
                        return { frameResult.error };
                    }

                    frames.push_back(frameResult.value);
                    break;
                }
                case 2:
                {
                    // Most common case. The first frame produces a frame that is not displayed but
                    // stored as a reference frame. The second frame is mostly empty and references
                    // the previously stored frame.
                    Result<size_t> firstFrameSizeResult = readFrameSize({ entryData.data(), entryData.size() }, bytesSize, 0);
                    if (firstFrameSizeResult.error)
                    {
                        return { firstFrameSizeResult.error };
                    }
                    std::vector<uint8_t> leftOver = std::vector<uint8_t>(packet.begin() + firstFrameSizeResult.value, packet.end());
                    packet.resize(firstFrameSizeResult.value);
                    Result<Frame> firstFrame = parseFrame(packet);
                    if (firstFrame.error)
                    {
                        return { firstFrame.error };
                    }

                    Result<size_t> secondFrameSizeResult = readFrameSize({ entryData.data(), entryData.size() }, bytesSize, 1);
                    if (secondFrameSizeResult.error)
                    {
                        return { secondFrameSizeResult.error };
                    }
                    leftOver.resize(secondFrameSizeResult.value);
                    Result<Frame> secondFrame = parseFrame(leftOver);
                    if (secondFrame.error)
                    {
                        return { secondFrame.error };
                    }

                    frames.push_back(firstFrame.value);
                    frames.push_back(secondFrame.value);
                    break;
                }
                default:
                {
                    // Odd, but also a valid bitstream configuration.
                    for (size_t frameIndex = 0; frameIndex < frameCount; ++frameIndex)
                    {
                        Result<size_t> frameSizeResult = readFrameSize({ entryData.data(), entryData.size() }, bytesSize, frameIndex);
                        if (frameSizeResult.error)
                        {
                            return { frameSizeResult.error };
                        }

                        std::vector<uint8_t> leftOver = std::vector<uint8_t>(packet.begin() + frameSizeResult.value, packet.end());
                        packet.resize(frameSizeResult.value);
                        Result<Frame> frameResult = parseFrame(packet);
                        if (frameResult.error)
                        {
                            return { frameResult.error };
                        }
                        frames.push_back(frameResult.value);

                        packet = leftOver;
                    }
                    break;
                }
            }

            return { OK, frames };
        }
    }

    // Normal frame.
    Result<Frame> frame = parseFrame(packet);
    if (frame.error)
    {
        return { frame.error };
    }
    return { OK, { frame.value } };
}
