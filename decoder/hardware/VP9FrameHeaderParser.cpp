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

    return { OK, value };
}

template <typename T, typename U>
Result<T> BitReader::readInverseSigned(uint8_t count)
{
    // The sign bit is at the start and not the end (even though it's BE).

    Result<U> valueResult = readUnsigned<U>(count);
    if (valueResult.error)
    {
        return { valueResult.error };
    }
    T value = static_cast<T>(valueResult.value);

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

VP9Frame VP9Frame::create(VP9Parser &parser, size_t uncompressedHeaderSize, size_t compressedHeaderSize, size_t tileSize, std::vector<uint8_t> data)
{
    VP9Frame frame;

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
    frame.loopFilterUpdateRefDelta = parser.loopFilterUpdateRefDelta;
    memcpy(frame.loopFilterRefDeltas, parser.loopFilterRefDeltas, sizeof(parser.loopFilterRefDeltas));
    frame.loopFilterUpdateModeDelta = parser.loopFilterUpdateModeDelta;
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

Slice<uint8_t> VP9Frame::compressedHeaderData()
{
    return {
        this->data.data() + this->uncompressedHeaderSize,
        this->compressedHeaderSize,
    };
}

Slice<uint8_t> VP9Frame::compressedHeaderAndTileData()
{
    return {
        this->data.data() + this->uncompressedHeaderSize,
        this->data.size() - this->uncompressedHeaderSize,
    };
}

Slice<uint8_t> VP9Frame::tileData()
{
    return {
        this->data.data() + this->uncompressedHeaderSize + this->compressedHeaderSize,
        this->data.size() - this->uncompressedHeaderSize - this->compressedHeaderSize,
    };
}

uint8_t VP9Frame::lastRefFrameIndex()
{
    return this->refFrameIndices[0];
}

uint8_t VP9Frame::goldenRefFrameIndex()
{
    return this->refFrameIndices[1];
}

uint8_t VP9Frame::altRefFrameIndex()
{
    return this->refFrameIndices[2];
}

SUBSAMPLING VP9Frame::subsampling()
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

Result<std::vector<VP9Frame>> VP9Parser::parsePacket(std::vector<uint8_t> packet)
{
    if (packet.empty())
    {
        return { OK, {} };
    }

    // Test for a super frame.
    size_t lastByteIndex = packet.size() - 1;
    uint8_t lastByte = packet[lastByteIndex];
    if ((lastByte & 0b1110'0000) == 0b1100'0000)
    {
        uint8_t bytesPerFramesizeMinus1 = (lastByte & 0b11000) >> 3;
        uint8_t framesInSuperframeMinus1 = lastByte & 0b111;
        size_t bytesSize = bytesPerFramesizeMinus1 + 1;
        size_t frameCount = framesInSuperframeMinus1 + 1;
        size_t indexSize = 2 + (frameCount * bytesSize);

        if (packet.size() < indexSize)
        {
            return { ERROR_FAIL };
        }

        size_t firstByteIndex = packet.size() - indexSize;
        uint8_t firstByte = packet[firstByteIndex];

        // Found a super frame.
        if (firstByte == lastByte)
        {
            std::vector<VP9Frame> frames = {};
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
                    Result<uint32_t> frameSizeResult = readFrameSize({ entryData.data(), entryData.size() }, bytesSize, 0);
                    if (frameSizeResult.error)
                    {
                        return { frameSizeResult.error };
                    }
                    if (frameSizeResult.value != firstByteIndex)
                    {
                        return { ERROR_FAIL };
                    }

                    ERROR packetTruncateError = truncateVectorU8(packet, frameSizeResult.value);
                    if (packetTruncateError)
                    {
                        return { packetTruncateError };
                    }

                    Result<VP9Frame> frameResult = parseFrame(packet);
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
                    Result<uint32_t> firstFrameSizeResult = readFrameSize({ entryData.data(), entryData.size() }, bytesSize, 0);
                    if (firstFrameSizeResult.error)
                    {
                        return { firstFrameSizeResult.error };
                    }
                    Result<uint32_t> secondFrameSizeResult = readFrameSize({ entryData.data(), entryData.size() }, bytesSize, 1);
                    if (secondFrameSizeResult.error)
                    {
                        return { secondFrameSizeResult.error };
                    }
                    if (firstFrameSizeResult.value > firstByteIndex || secondFrameSizeResult.value != (firstByteIndex - firstFrameSizeResult.value))
                    {
                        // (firstFrameSize + secondFrameSize) != firstByteIndex
                        return { ERROR_FAIL };
                    }

                    std::vector<uint8_t> leftOver = std::vector<uint8_t>(packet.begin() + firstFrameSizeResult.value, packet.end());

                    ERROR packetTruncateError = truncateVectorU8(packet, firstFrameSizeResult.value);
                    if (packetTruncateError)
                    {
                        return { packetTruncateError };
                    }

                    Result<VP9Frame> firstFrameResult = parseFrame(packet);
                    if (firstFrameResult.error)
                    {
                        return { firstFrameResult.error };
                    }

                    ERROR leftOverTruncateError = truncateVectorU8(leftOver, secondFrameSizeResult.value);
                    if (leftOverTruncateError)
                    {
                        return { leftOverTruncateError };
                    }

                    Result<VP9Frame> secondFrameResult = parseFrame(leftOver);
                    if (secondFrameResult.error)
                    {
                        return { secondFrameResult.error };
                    }

                    frames.push_back(firstFrameResult.value);
                    frames.push_back(secondFrameResult.value);
                    break;
                }
                default:
                {
                    // Odd, but also a valid bitstream configuration.
                    size_t remainingFrameData = firstByteIndex;
                    for (size_t frameIndex = 0; frameIndex < frameCount; ++frameIndex)
                    {
                        Result<uint32_t> frameSizeResult = readFrameSize({ entryData.data(), entryData.size() }, bytesSize, frameIndex);
                        if (frameSizeResult.error)
                        {
                            return { frameSizeResult.error };
                        }
                        if (frameSizeResult.value > remainingFrameData)
                        {
                            return { ERROR_FAIL };
                        }
                        remainingFrameData -= frameSizeResult.value;

                        std::vector<uint8_t> leftOver = std::vector<uint8_t>(packet.begin() + frameSizeResult.value, packet.end());

                        ERROR packetTruncateError = truncateVectorU8(packet, frameSizeResult.value);
                        if (packetTruncateError)
                        {
                            return { packetTruncateError };
                        }

                        Result<VP9Frame> frameResult = parseFrame(packet);
                        if (frameResult.error)
                        {
                            return { frameResult.error };
                        }
                        frames.push_back(frameResult.value);

                        packet = leftOver;
                    }
                    if (remainingFrameData != 0)
                    {
                        return { ERROR_FAIL };
                    }
                    break;
                }
            }

            return { OK, frames };
        }
    }

    // Normal frame.
    Result<VP9Frame> frameResult = parseFrame(packet);
    if (frameResult.error)
    {
        return { frameResult.error };
    }
    return { OK, { frameResult.value } };
}

Result<uint32_t> VP9Parser::readFrameSize(MutableSlice<uint8_t> entryData, size_t bytesSize, size_t index)
{
    // sic! Even though the values inside the uncompressed header are saved in BE,
    // these values are saved in LE.
    switch (bytesSize)
    {
        case 1:
        {
            return { OK, u8FromLeBytes(entryData.ptr + index) };
        }
        case 2:
        {
            return { OK, u16FromLeBytes(entryData.ptr + (index * 2)) };
        }
        case 3:
        {
            uint8_t *threeBytes = entryData.ptr + (index * 3);
            uint8_t fourBytes[4] = { threeBytes[0], threeBytes[1], threeBytes[2], 0 };
            return { OK, u32FromLeBytes(fourBytes) };
        }
        case 4:
        {
            return { OK, u32FromLeBytes(entryData.ptr + (index * 4)) };
        }
        default:
        {
            return { ERROR_INVALID_FRAME_SIZE_BYTE_SIZE };
        }
    };
}

Result<VP9Frame> VP9Parser::parseFrame(std::vector<uint8_t> data)
{
    BitReader br = BitReader({ data.data(), data.size() });

    Result<uint8_t> frameMarkerResult = br.readU8(2);
    if (frameMarkerResult.error)
    {
        return { frameMarkerResult.error };
    }
    if (frameMarkerResult.value != 2)
    {
        return { ERROR_INVALID_FRAME_MARKER };
    }

    Result<uint8_t> profileLowBitResult = br.readU8(1);
    if (profileLowBitResult.error)
    {
        return { profileLowBitResult.error };
    }
    Result<uint8_t> profileHighBitResult = br.readU8(1);
    if (profileHighBitResult.error)
    {
        return { profileHighBitResult.error };
    }
    this->profile = u8ToProfile((profileHighBitResult.value << 1) + profileLowBitResult.value);
    if (this->profile == PROFILE_3)
    {
        Result<uint8_t> reservedZeroResult = br.readU8(1);
        if (reservedZeroResult.error)
        {
            return { reservedZeroResult.error };
        }
        if (reservedZeroResult.value != 0)
        {
            return { ERROR_FAIL };
        }
    }

    Result<bool> showExistingFrameResult = br.readBool();
    if (showExistingFrameResult.error)
    {
        return { showExistingFrameResult.error };
    }
    this->showExistingFrame = showExistingFrameResult.value;

    if (this->showExistingFrame)
    {
        Result<uint8_t> frameToShowMapIdxResult = br.readU8(3);
        if (frameToShowMapIdxResult.error)
        {
            return { frameToShowMapIdxResult.error };
        }
        this->frameToShowMapIdx = { true, frameToShowMapIdxResult.value };
        this->refreshFrameFlags = 0;
        this->loopFilterLevel = 0;

        VP9Frame frame = VP9Frame::create(*this, 0, 0, 0, {});
        return { OK, frame };
    }
    else
    {
        this->frameToShowMapIdx = { false };
    }

    this->lastFrameType = this->frameType;

    Result<bool> frameTypeResult = br.readBool();
    if (frameTypeResult.error)
    {
        return { frameTypeResult.error };
    }
    this->frameType = boolToFrameType(frameTypeResult.value);

    Result<bool> showFrameResult = br.readBool();
    if (showFrameResult.error)
    {
        return { showFrameResult.error };
    }
    this->showFrame = showFrameResult.value;

    Result<bool> errorResilientModeResult = br.readBool();
    if (errorResilientModeResult.error)
    {
        return { errorResilientModeResult.error };
    }
    this->errorResilientMode = errorResilientModeResult.value;

    if (this->frameType == FRAME_TYPE_KEY_FRAME)
    {
        this->intraOnly = false;

        ERROR frameSyncCodeError = frameSyncCode(br);
        if (frameSyncCodeError)
        {
            return { frameSyncCodeError };
        }
        ERROR colorConfigError = colorConfig(br);
        if (colorConfigError)
        {
            return { colorConfigError };
        }
        ERROR frameSizeError = frameSize(br);
        if (frameSizeError)
        {
            return { frameSizeError };
        }
        ERROR renderSizeError = renderSize(br);
        if (renderSizeError)
        {
            return { renderSizeError };
        }
        this->refreshFrameFlags = 0xFF;
    }
    else
    {
        if (!this->showFrame)
        {
            Result<bool> intraOnlyResult = br.readBool();
            if (intraOnlyResult.error)
            {
                return { intraOnlyResult.error };
            }
            this->intraOnly = intraOnlyResult.value;
        }
        else
        {
            this->intraOnly = false;
        }

        if (!this->errorResilientMode)
        {
            Result<uint8_t> resetFrameContextResult = br.readU8(2);
            if (resetFrameContextResult.error)
            {
                return { resetFrameContextResult.error };
            }
            this->resetFrameContext = u8ToResetFrameContext(resetFrameContextResult.value);
        }
        else
        {
            this->resetFrameContext = RESET_FRAME_CONTEXT_NO_0;
        }

        if (this->intraOnly)
        {
            ERROR frameSyncCodeError = frameSyncCode(br);
            if (frameSyncCodeError)
            {
                return { frameSyncCodeError };
            }
            if (this->profile > PROFILE_0)
            {
                ERROR colorConfigError = colorConfig(br);
                if (colorConfigError)
                {
                    return { colorConfigError };
                }
            }
            else
            {
                this->colorDepth = COLOR_DEPTH_8;
                this->colorSpace = COLOR_SPACE_BT601;
                this->subsamplingX = true;
                this->subsamplingY = true;
            }
            Result<uint8_t> refreshFrameFlagsResult = br.readU8(8);
            if (refreshFrameFlagsResult.error)
            {
                return { refreshFrameFlagsResult.error };
            }
            this->refreshFrameFlags = refreshFrameFlagsResult.value;
            ERROR frameSizeError = frameSize(br);
            if (frameSizeError)
            {
                return { frameSizeError };
            }
            ERROR renderSizeError = renderSize(br);
            if (renderSizeError)
            {
                return { renderSizeError };
            }
        }
        else
        {
            Result<uint8_t> refreshFrameFlagsResult = br.readU8(8);
            if (refreshFrameFlagsResult.error)
            {
                return { refreshFrameFlagsResult.error };
            }
            this->refreshFrameFlags = refreshFrameFlagsResult.value;
            for (size_t i = 0; i < 3; ++i)
            {
                Result<uint8_t> refFrameIndicesResult = br.readU8(3);
                if (refFrameIndicesResult.error)
                {
                    return { refFrameIndicesResult.error };
                }
                this->refFrameIndices[i] = refFrameIndicesResult.value;

                Result<bool> refFrameSignBiasResult = br.readBool();
                if (refFrameSignBiasResult.error)
                {
                    return { refFrameSignBiasResult.error };
                }
                this->refFrameSignBias[LAST_FRAME + i] = refFrameSignBiasResult.value;
            }
            ERROR frameSizeWithRefsError = frameSizeWithRefs(br);
            if (frameSizeWithRefsError)
            {
                return { frameSizeWithRefsError };
            }
            Result<bool> allowHighPrecisionMvResult = br.readBool();
            if (allowHighPrecisionMvResult.error)
            {
                return { allowHighPrecisionMvResult.error };
            }
            ERROR readInterpolationFilterError = readInterpolationFilter(br);
            if (readInterpolationFilterError)
            {
                return { readInterpolationFilterError };
            }
        }
    }

    if (!this->errorResilientMode)
    {
        Result<bool> refreshFrameContextResult = br.readBool();
        if (refreshFrameContextResult.error)
        {
            return { refreshFrameContextResult.error };
        }
        this->refreshFrameContext = refreshFrameContextResult.value;

        Result<bool> frameParallelDecodingModeResult = br.readBool();
        if (frameParallelDecodingModeResult.error)
        {
            return { frameParallelDecodingModeResult.error };
        }
        this->frameParallelDecodingMode = frameParallelDecodingModeResult.value;
    }
    else
    {
        this->refreshFrameContext = false;
        this->frameParallelDecodingMode = false;
    }

    Result<uint8_t> frameContextIdxResult = br.readU8(2);
    if (frameContextIdxResult.error)
    {
        return { frameContextIdxResult.error };
    }
    this->frameContextIdx = frameContextIdxResult.value;

    if (this->intraOnly || this->errorResilientMode)
    {
        this->frameContextIdx = 0;
    }

    if (this->frameType == FRAME_TYPE_KEY_FRAME || this->errorResilientMode || this->intraOnly)
    {
        // Reset the loop filter deltas.
        this->loopFilterRefDeltas[INTRA_FRAME] = 1;
        this->loopFilterRefDeltas[LAST_FRAME] = 0;
        this->loopFilterRefDeltas[GOLDEN_FRAME] = -1;
        this->loopFilterRefDeltas[ALTREF_FRAME] = -1;
        this->loopFilterModeDeltas[0] = 0;
        this->loopFilterModeDeltas[1] = 0;
    }
    ERROR loopFilterParamsError = loopFilterParams(br);
    if (loopFilterParamsError)
    {
        return { loopFilterParamsError };
    }

    ERROR quantizationParamsError = quantizationParams(br);
    if (quantizationParamsError)
    {
        return { quantizationParamsError };
    }
    ERROR segmentationParamsError = segmentationParams(br);
    if (segmentationParamsError)
    {
        return { segmentationParamsError };
    }
    ERROR tileInfoError = tileInfo(br);
    if (tileInfoError)
    {
        return { tileInfoError };
    }

    Result<uint16_t> compressedHeaderSizeResult = br.readU16(16);
    if (compressedHeaderSizeResult.error)
    {
        return { compressedHeaderSizeResult.error };
    }
    size_t compressedHeaderSize = compressedHeaderSizeResult.value;
    ERROR trailingBitsError = trailingBits(br);
    if (trailingBitsError)
    {
        return { trailingBitsError };
    }
    size_t uncompressedHeaderSize = br.positionInBits / 8;

    if (data.size() < uncompressedHeaderSize || (data.size() - uncompressedHeaderSize) < compressedHeaderSize)
    {
        // data.size() < (uncompressedHeaderSize + compressedHeaderSize)
        return { ERROR_FAIL };
    }

    size_t tileSize = data.size() - (uncompressedHeaderSize + compressedHeaderSize);

    VP9Frame frame = VP9Frame::create(
        *this,
        uncompressedHeaderSize,
        compressedHeaderSize,
        tileSize,
        data
    );

    refreshRefFrames();

    return { OK, frame };
}

void VP9Parser::refreshRefFrames()
{
    uint8_t flags = this->refreshFrameFlags;
    uint32_t newWidth = this->width;
    uint32_t newHeight = this->height;

    for (size_t index = 0; index < 8; ++index)
    {
        if (((flags >> index) & 1) == 1)
        {
            refFrameSizes[index] = { newWidth, newHeight };
        }
    }
}

ERROR VP9Parser::frameSyncCode(BitReader &br)
{
    Result<uint8_t> frameSyncByte0Result = br.readU8(8);
    if (frameSyncByte0Result.error)
    {
        return frameSyncByte0Result.error;
    }
    Result<uint8_t> frameSyncByte1Result = br.readU8(8);
    if (frameSyncByte1Result.error)
    {
        return frameSyncByte1Result.error;
    }
    Result<uint8_t> frameSyncByte2Result = br.readU8(8);
    if (frameSyncByte2Result.error)
    {
        return frameSyncByte2Result.error;
    }

    if (frameSyncByte0Result.value != 0x49 || frameSyncByte1Result.value != 0x83 || frameSyncByte2Result.value != 0x42)
    {
        return ERROR_INVALID_SYNC_BYTE;
    }

    return OK;
}

ERROR VP9Parser::colorConfig(BitReader &br)
{
    if (this->profile >= PROFILE_2)
    {
        Result<bool> tenOrTwelveBitResult = br.readBool();
        if (tenOrTwelveBitResult.error)
        {
            return tenOrTwelveBitResult.error;
        }
        if (tenOrTwelveBitResult.value)
        {
            this->colorDepth = COLOR_DEPTH_12;
        }
        else
        {
            this->colorDepth = COLOR_DEPTH_10;
        }
    }
    else
    {
        this->colorDepth = COLOR_DEPTH_8;
    }

    Result<uint8_t> colorSpaceResult = br.readU8(3);
    if (colorSpaceResult.error)
    {
        return colorSpaceResult.error;
    }
    this->colorSpace = u8ToColorSpace(colorSpaceResult.value);

    if (this->colorSpace == COLOR_SPACE_RGB)
    {
        this->colorRange = COLOR_RANGE_FULL_SWING;
        if (this->profile == PROFILE_1 || this->profile == PROFILE_3)
        {
            this->subsamplingX = false;
            this->subsamplingY = false;

            Result<uint8_t> reservedZeroResult = br.readU8(1);
            if (reservedZeroResult.error)
            {
                return reservedZeroResult.error;
            }
            if (reservedZeroResult.value != 0)
            {
                return { ERROR_FAIL };
            }
        }
    }
    else
    {
        Result<bool> colorRangeResult = br.readBool();
        if (colorRangeResult.error)
        {
            return colorRangeResult.error;
        }
        this->colorRange = boolToColorRange(colorRangeResult.value);
        if (this->profile == PROFILE_1 || this->profile == PROFILE_3)
        {
            Result<bool> subsamplingXResult = br.readBool();
            if (subsamplingXResult.error)
            {
                return subsamplingXResult.error;
            }
            this->subsamplingX = subsamplingXResult.value;

            Result<bool> subsamplingYResult = br.readBool();
            if (subsamplingYResult.error)
            {
                return subsamplingYResult.error;
            }
            this->subsamplingY = subsamplingYResult.value;

            Result<uint8_t> reservedZeroResult = br.readU8(1);
            if (reservedZeroResult.error)
            {
                return reservedZeroResult.error;
            }
            if (reservedZeroResult.value != 0)
            {
                return { ERROR_FAIL };
            }
        }
        else
        {
            this->subsamplingX = true;
            this->subsamplingY = true;
        }
    }

    return OK;
}

ERROR VP9Parser::frameSize(BitReader &br)
{
    Result<uint16_t> frameWidthMinus1Result = br.readU16(16);
    if (frameWidthMinus1Result.error)
    {
        return frameWidthMinus1Result.error;
    }
    Result<uint16_t> frameHeightMinus1Result = br.readU16(16);
    if (frameHeightMinus1Result.error)
    {
        return frameHeightMinus1Result.error;
    }

    this->width = frameWidthMinus1Result.value + 1;
    this->height = frameHeightMinus1Result.value + 1;

    computeImageSize();

    return OK;
}

ERROR VP9Parser::renderSize(BitReader &br)
{
    Result<bool> renderAndFrameSizeDifferentResult = br.readBool();
    if (renderAndFrameSizeDifferentResult.error)
    {
        return renderAndFrameSizeDifferentResult.error;
    }
    if (renderAndFrameSizeDifferentResult.value)
    {
        Result<uint16_t> renderWidthMinus1Result = br.readU16(16);
        if (renderWidthMinus1Result.error)
        {
            return renderWidthMinus1Result.error;
        }
        Result<uint16_t> renderHeightMinus1Result = br.readU16(16);
        if (renderHeightMinus1Result.error)
        {
            return renderHeightMinus1Result.error;
        }

        this->renderWidth = renderWidthMinus1Result.value + 1;
        this->renderHeight = renderHeightMinus1Result.value + 1;
    }
    else
    {
        this->renderWidth = this->width;
        this->renderHeight = this->height;
    }

    return OK;
}

ERROR VP9Parser::frameSizeWithRefs(BitReader &br)
{
    bool foundRef = false;
    for (uint8_t i = 0; i < 3; ++i)
    {
        Result<bool> foundRefResult = br.readBool();
        if (foundRefResult.error)
        {
            return foundRefResult.error;
        }
        foundRef = foundRefResult.value;

        if (foundRef)
        {
            uint8_t refFrameIndex = this->refFrameIndices[i];
            if (refFrameIndex >= 8)
            {
                return ERROR_INVALID_REF_FRAME_INDEX;
            }
            std::pair<uint32_t, uint32_t> sizes = this->refFrameSizes[refFrameIndex];

            this->width = sizes.first;
            this->height = sizes.second;
            break;
        }
    }

    if (!foundRef)
    {
        ERROR frameSizeError = frameSize(br);
        if (frameSizeError)
        {
            return frameSizeError;
        }
    }
    else
    {
        computeImageSize();
    }

    ERROR renderSizeError = renderSize(br);
    if (renderSizeError)
    {
        return renderSizeError;
    }

    return OK;
}

void VP9Parser::computeImageSize()
{
    this->miCols = (this->width + 7) >> 3;
    this->miRows = (this->height + 7) >> 3;
}

ERROR VP9Parser::readInterpolationFilter(BitReader &br)
{
    Result<bool> isFilterSwitchableResult = br.readBool();
    if (isFilterSwitchableResult.error)
    {
        return isFilterSwitchableResult.error;
    }

    if (isFilterSwitchableResult.value)
    {
        this->interpolationFilter = INTERPOLATION_FILTER_SWITCHABLE;
    }
    else
    {
        Result<uint8_t> rawInterpolationFilterResult = br.readU8(2);
        if (rawInterpolationFilterResult.error)
        {
            return rawInterpolationFilterResult.error;
        }
        switch (rawInterpolationFilterResult.value)
        {
            case 0:
                this->interpolationFilter = INTERPOLATION_FILTER_EIGHTTAP_SMOOTH;
                break;
            case 1:
                this->interpolationFilter = INTERPOLATION_FILTER_EIGHTTAP;
                break;
            case 2:
                this->interpolationFilter = INTERPOLATION_FILTER_EIGHTTAP_SHARP;
                break;
            case 3:
                this->interpolationFilter = INTERPOLATION_FILTER_BILINEAR;
                break;
            default:
                this->interpolationFilter = INTERPOLATION_FILTER_UNKNOWN;
                break;
        }
    }

    return OK;
}

ERROR VP9Parser::loopFilterParams(BitReader &br)
{
    Result<uint8_t> loopFilterLevelResult = br.readU8(6);
    if (loopFilterLevelResult.error)
    {
        return loopFilterLevelResult.error;
    }
    this->loopFilterLevel = loopFilterLevelResult.value;

    Result<uint8_t> loopFilterSharpnessResult = br.readU8(3);
    if (loopFilterSharpnessResult.error)
    {
        return loopFilterSharpnessResult.error;
    }
    this->loopFilterSharpness = loopFilterSharpnessResult.value;

    Result<bool> loopFilterDeltaEnabledResult = br.readBool();
    if (loopFilterDeltaEnabledResult.error)
    {
        return loopFilterDeltaEnabledResult.error;
    }
    this->loopFilterDeltaEnabled = loopFilterDeltaEnabledResult.value;

    this->loopFilterUpdateRefDelta = false;
    this->loopFilterUpdateModeDelta = false;

    if (this->loopFilterDeltaEnabled)
    {
        Result<bool> loopFilterDeltaUpdateResult = br.readBool();
        if (loopFilterDeltaUpdateResult.error)
        {
            return loopFilterDeltaUpdateResult.error;
        }

        if (loopFilterDeltaUpdateResult.value)
        {
            for (uint8_t i = 0; i < 4; ++i)
            {
                Result<bool> updateRefDeltaResult = br.readBool();
                if (updateRefDeltaResult.error)
                {
                    return updateRefDeltaResult.error;
                }

                this->loopFilterUpdateRefDelta |= updateRefDeltaResult.value;

                if (updateRefDeltaResult.value)
                {
                    Result<int8_t> loopFilterRefDeltaResult = br.readInverseI8(6);
                    if (loopFilterRefDeltaResult.error)
                    {
                        return loopFilterRefDeltaResult.error;
                    }
                    loopFilterRefDeltas[i] = loopFilterRefDeltaResult.value;
                }
            }

            for (uint8_t i = 0; i < 2; ++i)
            {
                Result<bool> updateModeDeltaResult = br.readBool();
                if (updateModeDeltaResult.error)
                {
                    return updateModeDeltaResult.error;
                }

                this->loopFilterUpdateModeDelta |= updateModeDeltaResult.value;

                if (updateModeDeltaResult.value)
                {
                    Result<int8_t> loopFilterModeDeltaResult = br.readInverseI8(6);
                    if (loopFilterModeDeltaResult.error)
                    {
                        return loopFilterModeDeltaResult.error;
                    }
                    loopFilterModeDeltas[i] = loopFilterModeDeltaResult.value;
                }
            }
        }
    }

    return OK;
}

ERROR VP9Parser::quantizationParams(BitReader &br)
{
    Result<uint8_t> baseQIdxResult = br.readU8(8);
    if (baseQIdxResult.error)
    {
        return baseQIdxResult.error;
    }
    this->baseQIdx = baseQIdxResult.value;

    Result<int32_t> deltaQYDcResult = readDeltaQ(br);
    if (deltaQYDcResult.error)
    {
        return deltaQYDcResult.error;
    }
    this->deltaQYDc = deltaQYDcResult.value;

    Result<int32_t> deltaQUvDcResult = readDeltaQ(br);
    if (deltaQUvDcResult.error)
    {
        return deltaQUvDcResult.error;
    }
    this->deltaQUvDc = deltaQUvDcResult.value;

    Result<int32_t> deltaQUvAcResult = readDeltaQ(br);
    if (deltaQUvAcResult.error)
    {
        return deltaQUvAcResult.error;
    }
    this->deltaQUvAc = deltaQUvAcResult.value;

    this->lossless = (this->baseQIdx == 0)
        && (this->deltaQYDc == 0)
        && (this->deltaQUvDc == 0)
        && (this->deltaQUvAc == 0);

    return OK;
}

Result<int32_t> VP9Parser::readDeltaQ(BitReader &br)
{
    Result<bool> deltaCodedResult = br.readBool();
    if (deltaCodedResult.error)
    {
        return { deltaCodedResult.error };
    }

    if (deltaCodedResult.value)
    {
        Result<int8_t> deltaQResult = br.readInverseI8(4);
        if (deltaQResult.error)
        {
            return { deltaQResult.error };
        }

        return { OK, deltaQResult.value };
    }
    else
    {
        return { OK, 0 };
    }
}

ERROR VP9Parser::segmentationParams(BitReader &br)
{
    Result<bool> segmentationEnabledResult = br.readBool();
    if (segmentationEnabledResult.error)
    {
        return segmentationEnabledResult.error;
    }
    this->segmentationEnabled = segmentationEnabledResult.value;

    if (this->segmentationEnabled)
    {
        Result<bool> segmentationUpdateMapResult = br.readBool();
        if (segmentationUpdateMapResult.error)
        {
            return segmentationUpdateMapResult.error;
        }
        this->segmentationUpdateMap = segmentationUpdateMapResult.value;

        if (this->segmentationUpdateMap)
        {
            for (uint8_t i = 0; i < 7; ++i)
            {
                Result<uint8_t> segmentTreeProbResult = readProb(br);
                if (segmentTreeProbResult.error)
                {
                    return segmentTreeProbResult.error;
                }

                this->segmentTreeProbs[i] = segmentTreeProbResult.value;
            }

            Result<bool> segmentationTemporalUpdateResult = br.readBool();
            if (segmentationTemporalUpdateResult.error)
            {
                return segmentationTemporalUpdateResult.error;
            }
            this->segmentationTemporalUpdate = segmentationTemporalUpdateResult.value;

            for (uint8_t i = 0; i < 3; ++i)
            {
                if (this->segmentationTemporalUpdate)
                {
                    Result<uint8_t> segmentPredProbResult = readProb(br);
                    if (segmentPredProbResult.error)
                    {
                        return segmentPredProbResult.error;
                    }

                    this->segmentPredProbs[i] = segmentPredProbResult.value;
                }
                else
                {
                    this->segmentPredProbs[i] = 255;
                }
            }
        }

        Result<bool> segmentationUpdateDataResult = br.readBool();
        if (segmentationUpdateDataResult.error)
        {
            return segmentationUpdateDataResult.error;
        }
        this->segmentationUpdateData = segmentationUpdateDataResult.value;

        if (this->segmentationUpdateData)
        {
            Result<bool> segmentationAbsOrDeltaUpdateResult = br.readBool();
            if (segmentationAbsOrDeltaUpdateResult.error)
            {
                return segmentationAbsOrDeltaUpdateResult.error;
            }
            this->segmentationAbsOrDeltaUpdate = segmentationAbsOrDeltaUpdateResult.value;

            for (uint8_t i = 0; i < MAX_SEGMENTS; ++i)
            {
                Result<bool> segmentFeatureEnabledAltQResult = br.readBool();
                if (segmentFeatureEnabledAltQResult.error)
                {
                    return segmentFeatureEnabledAltQResult.error;
                }
                this->segmentFeatureEnabled[i][SEG_LVL_ALT_Q] = segmentFeatureEnabledAltQResult.value;

                if (this->segmentFeatureEnabled[i][SEG_LVL_ALT_Q])
                {
                    Result<int16_t> segmentFeatureDataAltQResult = br.readInverseI16(8);
                    if (segmentFeatureDataAltQResult.error)
                    {
                        return segmentFeatureDataAltQResult.error;
                    }
                    this->segmentFeatureData[i][SEG_LVL_ALT_Q] = segmentFeatureDataAltQResult.value;
                }

                Result<bool> segmentFeatureEnabledAltLResult = br.readBool();
                if (segmentFeatureEnabledAltLResult.error)
                {
                    return segmentFeatureEnabledAltLResult.error;
                }
                this->segmentFeatureEnabled[i][SEG_LVL_ALT_L] = segmentFeatureEnabledAltLResult.value;

                if (this->segmentFeatureEnabled[i][SEG_LVL_ALT_L])
                {
                    Result<int16_t> segmentFeatureDataAltLResult = br.readInverseI16(6);
                    if (segmentFeatureDataAltLResult.error)
                    {
                        return segmentFeatureDataAltLResult.error;
                    }
                    this->segmentFeatureData[i][SEG_LVL_ALT_L] = segmentFeatureDataAltLResult.value;
                }

                Result<bool> segmentFeatureEnabledRefFrameResult = br.readBool();
                if (segmentFeatureEnabledRefFrameResult.error)
                {
                    return segmentFeatureEnabledRefFrameResult.error;
                }
                this->segmentFeatureEnabled[i][SEG_LVL_REF_FRAME] = segmentFeatureEnabledRefFrameResult.value;

                if (this->segmentFeatureEnabled[i][SEG_LVL_REF_FRAME])
                {
                    Result<int16_t> segmentFeatureDataRefFrameResult = br.readInverseI16(6);
                    if (segmentFeatureDataRefFrameResult.error)
                    {
                        return segmentFeatureDataRefFrameResult.error;
                    }
                    this->segmentFeatureData[i][SEG_LVL_REF_FRAME] = segmentFeatureDataRefFrameResult.value;
                }

                Result<bool> segmentFeatureEnabledSkipResult = br.readBool();
                if (segmentFeatureEnabledSkipResult.error)
                {
                    return segmentFeatureEnabledSkipResult.error;
                }
                this->segmentFeatureEnabled[i][SEG_LVL_SKIP] = segmentFeatureEnabledSkipResult.value;

                this->segmentFeatureData[i][SEG_LVL_SKIP] = 0;
            }
        }
    }

    return OK;
}

Result<uint8_t> VP9Parser::readProb(BitReader &br)
{
    Result<bool> probCodedResult = br.readBool();
    if (probCodedResult.error)
    {
        return { probCodedResult.error };
    }

    if (probCodedResult.value)
    {
        Result<uint8_t> probResult = br.readU8(8);
        if (probResult.error)
        {
            return { probResult.error };
        }

        return { OK, probResult.value };
    }
    else
    {
        return { OK, 255 };
    }
}

ERROR VP9Parser::tileInfo(BitReader &br)
{
    Result<uint8_t> minLog2TileColsResult = calcMinLog2TileCols();
    if (minLog2TileColsResult.error)
    {
        return minLog2TileColsResult.error;
    }
    Result<uint8_t> maxLog2TileColsResult = calcMaxLog2TileCols();
    if (maxLog2TileColsResult.error)
    {
        return maxLog2TileColsResult.error;
    }

    this->tileColsLog2 = minLog2TileColsResult.value;
    while (this->tileColsLog2 < maxLog2TileColsResult.value)
    {
        Result<bool> incrementTileColsLog2Result = br.readBool();
        if (incrementTileColsLog2Result.error)
        {
            return incrementTileColsLog2Result.error;
        }

        if (incrementTileColsLog2Result.value)
        {
            this->tileColsLog2 += 1;
        }
        else
        {
            break;
        }
    }

    Result<uint8_t> tileRowsLog2Result = br.readU8(1);
    if (tileRowsLog2Result.error)
    {
        return tileRowsLog2Result.error;
    }

    this->tileRowsLog2 = tileRowsLog2Result.value;
    if (this->tileRowsLog2 == 1)
    {
        Result<uint8_t> incrementTileRowsLog2Result = br.readU8(1);
        if (incrementTileRowsLog2Result.error)
        {
            return incrementTileRowsLog2Result.error;
        }

        this->tileRowsLog2 += incrementTileRowsLog2Result.value;
    }

    return OK;
}

Result<uint8_t> VP9Parser::calcMinLog2TileCols()
{
    uint16_t sb64ColsU16 = (this->miCols + 7) >> 3;
    if (sb64ColsU16 > UINT8_MAX)
    {
        return { ERROR_FAIL };
    }
    uint8_t sb64Cols = static_cast<uint8_t>(sb64ColsU16);

    uint8_t minLog2 = 0;
    while ((MAX_TILE_WIDTH_B64 << minLog2) < sb64Cols)
    {
        minLog2 += 1;
    }

    return { OK, minLog2 };
}

Result<uint8_t> VP9Parser::calcMaxLog2TileCols()
{
    uint16_t sb64ColsU16 = (this->miCols + 7) >> 3;
    if (sb64ColsU16 > UINT8_MAX)
    {
        return { ERROR_FAIL };
    }
    uint8_t sb64Cols = static_cast<uint8_t>(sb64ColsU16);

    uint8_t maxLog2 = 1;
    while ((sb64Cols >> maxLog2) >= MIN_TILE_WIDTH_B64)
    {
        maxLog2 += 1;
    }
    return { OK, static_cast<uint8_t>(maxLog2 - 1) };
}

ERROR VP9Parser::trailingBits(BitReader &br)
{
    while (!br.isAligned(1))
    {
        Result<bool> zeroBitResult = br.readBool();
        if (zeroBitResult.error)
        {
            return zeroBitResult.error;
        }

        if (zeroBitResult.value)
        {
            return ERROR_INVALID_PADDING;
        }
    }

    return OK;
}

constexpr uint32_t VP9Parser::u32FromLeBytes(const uint8_t *bytes)
{
    return static_cast<uint32_t>(bytes[0])
        | (static_cast<uint32_t>(bytes[1]) << 8)
        | (static_cast<uint32_t>(bytes[2]) << 16)
        | (static_cast<uint32_t>(bytes[3]) << 24);
}

constexpr uint16_t VP9Parser::u16FromLeBytes(const uint8_t *bytes)
{
    return static_cast<uint16_t>(bytes[0])
        | (static_cast<uint16_t>(bytes[1]) << 8);
}

constexpr uint8_t VP9Parser::u8FromLeBytes(const uint8_t *bytes)
{
    return bytes[0];
}

ERROR VP9Parser::truncateVectorU8(std::vector<uint8_t> &vector, size_t newSize)
{
    if (newSize > vector.size())
    {
        return ERROR_FAIL;
    }

    vector.resize(newSize);
    return OK;
}
