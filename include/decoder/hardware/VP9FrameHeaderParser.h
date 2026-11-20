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

#ifndef VP9_FRAME_HEADER_PARSER_H
#define VP9_FRAME_HEADER_PARSER_H

#include <stdint.h>
#include <vector>

namespace VP9FrameHeaderParser
{
    /// Contains all the possible errors in VP9FrameHeaderParser.
    enum ERROR : uint8_t
    {
        OK,
        ERROR_FAIL,
        ERROR_BIT_READER,
        ERROR_IO,
        ERROR_TRY_FROM_SLICE,
        ERROR_TRY_FROM_INT,
        ERROR_INVALID_FRAME_MARKER,
        ERROR_INVALID_PADDING,
        ERROR_INVALID_SYNC_BYTE,
        ERROR_INVALID_REF_FRAME_INDEX,
        ERROR_INVALID_METADATA,
        ERROR_INVALID_FRAME_SIZE_BYTE_SIZE,
    };

    /// Equivalent for Rust's Option type.
    template <typename T>
    struct Option
    {
        bool hasValue = false;
        T value = {};
    };

    /// Equivalent for Rust's slice type.
    template <typename T>
    struct Slice
    {
        const T *ptr = nullptr;
        size_t len = 0;
    };

    /// Equivalent for Rust's mutable slice type.
    template <typename T>
    struct MutableSlice
    {
        T *ptr = nullptr;
        size_t len = 0;
    };

    /// Equivalent for Rust's Result type.
    template <typename T>
    struct Result
    {
        ERROR error = OK;
        T value = {};
    };

    /// Equivalent for Rust's BitReader type.
    struct BitReader
    {
        const uint8_t *data = nullptr;
        size_t sizeInBits = 0;
        size_t positionInBits = 0;

        BitReader(Slice<uint8_t> slice);
        Result<uint64_t> readU64(uint8_t count);
        Result<uint32_t> readU32(uint8_t count);
        Result<uint16_t> readU16(uint8_t count);
        Result<uint8_t> readU8(uint8_t count);
        Result<bool> readBool();
        Result<int8_t> readInverseI8(uint8_t count);
        Result<int16_t> readInverseI16(uint8_t count);
        size_t getRemainingBits() const;
        bool isAligned(uint32_t alignmentBytes) const;

    private:
        template <typename T>
        Result<T> readUnsigned(uint8_t count);
        template <typename T, typename U>
        Result<T> readInverseSigned(uint8_t count);
    };

    /// The number of segments allowed in segmentation map.
    static constexpr uint8_t MAX_SEGMENTS = 8;
    /// The minimum width of a tile in units of super blocks.
    static constexpr uint8_t MIN_TILE_WIDTH_B64 = 4;
    /// The maximum width of a tile in units of super blocks.
    static constexpr uint8_t MAX_TILE_WIDTH_B64 = 64;

    /// The index of the "intra" ref frame.
    static constexpr uint8_t INTRA_FRAME = 0;
    /// The index of the "last" ref frame.
    static constexpr uint8_t LAST_FRAME = 1;
    /// The index of the "golden" ref frame.
    static constexpr uint8_t GOLDEN_FRAME = 2;
    /// The index of the "altref" ref frame.
    static constexpr uint8_t ALTREF_FRAME = 3;

    /// The index of the "quantizer" segment feature.
    static constexpr uint8_t SEG_LVL_ALT_Q = 0;
    /// The index of the "loop filter" segment feature.
    static constexpr uint8_t SEG_LVL_ALT_L = 1;
    /// The index of the "reference frame" segment feature.
    static constexpr uint8_t SEG_LVL_REF_FRAME = 2;
    /// The index of the "skip" segment feature.
    static constexpr uint8_t SEG_LVL_SKIP = 3;

    /// The VP9 profiles.
    enum PROFILE : uint8_t
    {
        /// Unknown.
        PROFILE_UNKNOWN,
        /// Color depth: 8 bit/sample, chroma subsampling: 4:2:0
        PROFILE_0,
        /// Color depth: 8 bit, chroma subsampling: 4:2:2, 4:4:0, 4:4:4
        PROFILE_1,
        /// Color depth: 10–12 bit, chroma subsampling: 4:2:0
        PROFILE_2,
        /// Color depth: 10–12 bit, chroma subsampling: 4:2:2, 4:4:0, 4:4:4
        PROFILE_3,
    };

    constexpr PROFILE u8ToProfile(uint8_t value);

    /// Chroma subsampling.
    enum SUBSAMPLING : uint8_t
    {
        /// 4:4:4 - No chrome subsampling.
        SUBSAMPLING_YUV_444,
        /// 4:4:0 - Subsampling along the y axis.
        SUBSAMPLING_YUV_440,
        /// 4:2:2 - Subsampling along the x axis.
        SUBSAMPLING_YUV_422,
        /// 4:2:0 - Subsampling along both x and y axis.
        SUBSAMPLING_YUV_420,
    };

    /// Chroma subsampling as defined in the Metadata
    enum METADATA_SUBSAMPLING : uint8_t
    {
        /// Unknown.
        METADATA_SUBSAMPLING_UNKNOWN,
        /// 4:2:0 - Subsampling along both x and y axis.
        METADATA_SUBSAMPLING_YUV_420,
        /// 4:2:0 - Chroma subsampling colocated with (0,0) luma.
        METADATA_SUBSAMPLING_YUV_420_COLOCATED,
        /// 4:2:2 - Subsampling along the x axis.
        METADATA_SUBSAMPLING_YUV_422,
        /// 4:4:4 - No chrome subsampling.
        METADATA_SUBSAMPLING_YUV_444,
    };

    constexpr METADATA_SUBSAMPLING u8ToMetadataSubsampling(uint8_t value);

    /// Color space.
    enum COLOR_SPACE : uint8_t
    {
        /// Unknown (in this case the color space must be signaled outside the VP9 bitstream).
        COLOR_SPACE_UNKNOWN,
        /// Rec. ITU-R BT.601-7
        COLOR_SPACE_BT601,
        /// Rec. ITU-R BT.709-6
        COLOR_SPACE_BT709,
        /// SMPTE-170
        COLOR_SPACE_SMPTE170,
        /// SMPTE-240
        COLOR_SPACE_SMPTE240,
        /// Rec. ITU-R BT.2020-2
        COLOR_SPACE_BT2020,
        /// Reserved
        COLOR_SPACE_RESERVED,
        /// sRGB (IEC 61966-2-1)
        COLOR_SPACE_RGB,
    };

    constexpr COLOR_SPACE u8ToColorSpace(uint8_t value);

    /// Color depth.
    enum COLOR_DEPTH : uint8_t
    {
        /// Unknown,
        COLOR_DEPTH_UNKNOWN,
        /// 8 bit depth.
        COLOR_DEPTH_8,
        /// 10 bit depth.
        COLOR_DEPTH_10,
        /// 12 bit depth.
        COLOR_DEPTH_12,
    };

    constexpr COLOR_DEPTH u8ToColorDepth(uint8_t value);

    /// Specifies the black level and range of the luma and chroma signals as specified in
    /// Rec. ITU-R BT.709-6 and Rec. ITU-R BT.2020-2.
    enum COLOR_RANGE : uint8_t
    {
        /// Put restriction on Y, U, V values.
        COLOR_RANGE_STUDIO_SWING,
        /// No restriction on Y, U, V values.
        COLOR_RANGE_FULL_SWING,
    };

    constexpr COLOR_RANGE boolToColorRange(bool value);

    /// Type of the interpolation filter.
    enum INTERPOLATION_FILTER : uint8_t
    {
        /// Unknown.
        INTERPOLATION_FILTER_UNKNOWN,
        /// EIGHTTAP.
        INTERPOLATION_FILTER_EIGHTTAP,
        /// EIGHTTAP_SMOOTH.
        INTERPOLATION_FILTER_EIGHTTAP_SMOOTH,
        /// EIGHTTAP_SHARP.
        INTERPOLATION_FILTER_EIGHTTAP_SHARP,
        /// BILINEAR.
        INTERPOLATION_FILTER_BILINEAR,
        /// SWITCHABLE.
        INTERPOLATION_FILTER_SWITCHABLE,
    };

    /// The type of a frame.
    enum FRAME_TYPE : uint8_t
    {
        /// Frame is a key frame.
        FRAME_TYPE_KEY_FRAME,
        /// Frame is not a key frame.
        FRAME_TYPE_NON_KEY_FRAME,
    };

    constexpr FRAME_TYPE boolToFrameType(bool value);

    /// Defines if the frame context should be reset.
    enum RESET_FRAME_CONTEXT : uint8_t
    {
        /// Unknown.
        RESET_FRAME_CONTEXT_UNKNOWN,
        /// Do not reset any frame context.
        RESET_FRAME_CONTEXT_NO_0,
        /// Do not reset any frame context.
        RESET_FRAME_CONTEXT_NO_1,
        /// Resets just the context specified in the frame header.
        RESET_FRAME_CONTEXT_SINGLE_RESET,
        /// Resets all contexts.
        RESET_FRAME_CONTEXT_FULL_RESET,
    };

    constexpr RESET_FRAME_CONTEXT u8ToResetFrameContext(uint8_t value);

    /// The codec level.
    enum LEVEL : uint8_t
    {
        /// Unknown.
        LEVEL_UNKNOWN,
        /// Level 1: 0.20 MBit/s
        LEVEL_1,
        /// Level 1.1: 0.80 MBit/s
        LEVEL_1_1,
        /// Level 2: 1.8 MBit/s
        LEVEL_2,
        /// Level 2.1: 3.6 MBit/s
        LEVEL_2_1,
        /// Level 3: 7.2 MBit/s
        LEVEL_3,
        /// Level 3.2: 12 MBit/s
        LEVEL_3_1,
        /// Level 4: 18 MBit/s
        LEVEL_4,
        /// Level 4.1: 30 MBit/s
        LEVEL_4_1,
        /// Level 5: 60 MBit/s
        LEVEL_5,
        /// Level 5.1: 120 MBit/s
        LEVEL_5_1,
        /// Level 5.2: 180 MBit/s
        LEVEL_5_2,
        /// Level 6: 180 MBit/s
        LEVEL_6,
        /// Level 6.1: 240 MBit/s
        LEVEL_6_1,
        /// Level 6.2: 480 MBit/s
        LEVEL_6_2,
    };

    constexpr LEVEL u8ToLevel(uint8_t value);

    /// VP9 Codec Feature Metadata saved inside the `CodecPrivate` field of containers.
    struct Metadata {
        PROFILE profile;
        LEVEL level;
        COLOR_DEPTH colorDepth;
        METADATA_SUBSAMPLING chromaSubsampling;

        /// Creates the Vp9Metadata from the given `CodecPrivate` data.
        static Result<Metadata> create(Slice<uint8_t> data);
    };

    struct VP9Parser;

    /// A VP9 frame.
    struct Frame {
        std::vector<uint8_t> data;
        /// The profile the frame is using.
        PROFILE profile;
        /// Indicates that the frame indexed by `frameToShowMapIdx` is to be displayed.
        /// The frame contains no actual frame data.
        bool showExistingFrame;
        /// Specifies the frame to be displayed. It is only available if `showExistingFrame` is true.
        Option<uint8_t> frameToShowMapIdx;
        /// The frame type of the previous frame.
        FRAME_TYPE lastFrameType;
        /// The frame type of this frame.
        FRAME_TYPE frameType;
        /// Indicates that the frame is to be displayed.
        bool showFrame;
        /// Indicates that error resilient mode is enabled.
        ///
        /// Error resilient mode allows the syntax of a frame to be decoded
        /// independently of previous frames.
        bool errorResilientMode;
        /// Indicates that a frame is an `intra-only` frame.
        ///
        /// A key frame is different to an `intra-only` frame even though both only use
        /// intra prediction. The difference is that a key frame fully resets the decoding process.
        bool intraOnly;
        /// Specifies whether the frame context should be reset to default values.
        RESET_FRAME_CONTEXT resetFrameContext;
        /// The indices of the used reference frames.
        uint8_t refFrameIndices[3];
        /// Specifies the intended direction of the motion vector in time for each reference frame.
        bool refFrameSignBias[4];
        /// Specifies the precision of the motion vectors.
        ///
        /// False = quarter precision, True = eighth precision.
        bool allowHighPrecisionMv;
        /// Specifies that the probabilities computed for this frame
        /// should be stored for reference by future frames.
        bool refreshFrameContext;
        /// Contains a bitmask that specifies which reference frame slots
        /// will be updated with the current frame after it is decoded.
        ///
        /// First bit = first frame (1). Last bit = last frame (8).
        uint8_t refreshFrameFlags;
        /// Specifies if parallel decoding mode is activated.
        bool frameParallelDecodingMode;
        /// Specifies which frame context to use.
        uint8_t frameContextIdx;
        /// The size of the uncompressed header.
        size_t uncompressedHeaderSize;
        /// The size of the uncompressed header.
        size_t compressedHeaderSize;
        /// The size of the tile data.
        size_t tileSize;
        /// The color depth of the frame.
        COLOR_DEPTH colorDepth;
        /// The color space of the frame.
        COLOR_SPACE colorSpace;
        /// The color range of the frame.
        COLOR_RANGE colorRange;
        /// Indicates if sub sampling is used along the x axis.
        bool subsamplingX;
        /// Indicates if sub sampling is used along the y axis.
        bool subsamplingY;
        /// The width of the frame.
        uint16_t width;
        /// The height of the frame.
        uint16_t height;
        /// A hint for the application for the desired width to render.
        uint16_t renderWidth;
        /// A hint for the application for the desired height to render.
        uint16_t renderHeight;
        /// A variable holding the vertical location of the block in units of 8x8 pixels.
        uint16_t miCols;
        /// A variable holding the horizontal location of the block in units of 8x8 pixels.
        uint16_t miRows;
        /// The base 2 logarithm of the height of each tile (where the height is measured in units
        /// of 8x8 blocks)
        uint8_t tileRowsLog2;
        /// The base 2 logarithm of the width of each tile (where the width is measured in units
        /// of 8x8 blocks)
        uint8_t tileColsLog2;
        /// The type of filter used in inter prediction.
        INTERPOLATION_FILTER interpolationFilter;
        /// The loop filter strength.
        uint8_t loopFilterLevel;
        /// The loop filter sharpness.
        uint8_t loopFilterSharpness;
        /// Indicates that the filter level depends on the mode and reference frame
        /// used to predict a block.
        bool loopFilterDeltaEnabled;
        /// Indicates that the the bitstream contains the syntax element loopFilterRefDelta.
        bool updateRefDelta;
        /// Contains the adjustment needed for the filter level based on the chosen reference frame.
        int8_t loopFilterRefDeltas[4];
        /// Indicates that the the bitstream contains the syntax element loopFilterModeDeltas.
        bool updateModeDelta;
        /// Contains the adjustment needed for the filter level based on the chosen mode.
        int8_t loopFilterModeDeltas[2];
        /// The base frame qindex. This is used for Y AC coefficients and as the base value
        /// for the other quantizers.
        int32_t baseQIdx;
        /// The Y DC quantizer relative to baseQIdx.
        int32_t deltaQYDc;
        /// The UV DC quantizer relative to baseQIdx.
        int32_t deltaQUvDc;
        /// The UV AC quantizer relative to baseQIdx.
        int32_t deltaQUvAc;
        /// Indicates that the frame is coded using a special 4x4 transform designed
        /// for encoding frames that are bit-identical with the original frames.
        bool lossless;
        /// Specifies that this frame makes use of the segmentation tool.
        bool segmentationEnabled;
        /// Specifies that the segmentation map should be updated during the decoding of this frame.
        bool segmentationUpdateMap;
        /// The probability values to be used when decoding segmentId.
        uint8_t segmentTreeProbs[7];
        /// The probability values to be used when decoding segIdPredicted.
        uint8_t segmentPredProbs[3];
        /// Indicates that the updates to the segmentation map are coded
        /// relative to the existing segmentation map.
        bool segmentationTemporalUpdate;
        /// Indicates that new parameters are about to be specified for each segment.
        bool segmentationUpdateData;
        /// Indicates that the segmentation parameters represent the actual values to be used,
        /// otherwise the segmentation parameters represent adjustments relative to the standard values.
        bool segmentationAbsOrDeltaUpdate;
        /// Indicates that the corresponding feature is used in a segment.
        bool segmentFeatureEnabled[8][4];
        /// Specifies the values of the active features of a segment.
        int16_t segmentFeatureData[8][4];

        /// Creates a frame from the parser state.
        static Frame create(VP9Parser &parser, size_t uncompressedHeaderSize, size_t compressedHeaderSize, size_t tileSize, std::vector<uint8_t> data);

        /// Returns a slice into the data of the compressed header.
        Slice<uint8_t> compressedHeaderData();

        /// Returns a slice into the data of the compressed header and tile data.
        Slice<uint8_t> compressedHeaderAndTileData();

        /// Returns a slice into the data of the tile data.
        Slice<uint8_t> tileData();

        /// Last reference frame index.
        uint8_t lastRefFrameIndex();

        /// Golden reference frame index.
        uint8_t goldenRefFrameIndex();

        /// Alternate reference frame index.
        uint8_t altRefFrameIndex();

        /// The subsampling the frame is using.
        SUBSAMPLING subsampling();
    };

    /// Parses VP9 bitstreams.
    struct VP9Parser {
        std::pair<uint16_t, uint16_t> refFrameSizes[8] = {};
        PROFILE profile = PROFILE_0;
        bool showExistingFrame = false;
        Option<uint8_t> frameToShowMapIdx = { false };
        FRAME_TYPE lastFrameType = FRAME_TYPE_NON_KEY_FRAME;
        FRAME_TYPE frameType = FRAME_TYPE_NON_KEY_FRAME;
        bool showFrame = false;
        bool errorResilientMode = false;
        bool intraOnly = false;
        RESET_FRAME_CONTEXT resetFrameContext = RESET_FRAME_CONTEXT_NO_0;
        uint8_t refFrameIndices[3] = {};
        bool refFrameSignBias[4] = {};
        bool allowHighPrecisionMv = false;
        bool refreshFrameContext = false;
        uint8_t refreshFrameFlags = 0;
        bool frameParallelDecodingMode = true;
        uint8_t frameContextIdx = 0;
        COLOR_DEPTH colorDepth = COLOR_DEPTH_8;
        COLOR_SPACE colorSpace = COLOR_SPACE_UNKNOWN;
        COLOR_RANGE colorRange = COLOR_RANGE_STUDIO_SWING;
        bool subsamplingX = true;
        bool subsamplingY = true;
        uint16_t width = 0;
        uint16_t height = 0;
        uint16_t renderWidth = 0;
        uint16_t renderHeight = 0;
        uint16_t miCols = 0;
        uint16_t miRows = 0;
        uint8_t tileRowsLog2 = 0;
        uint8_t tileColsLog2 = 0;
        INTERPOLATION_FILTER interpolationFilter = INTERPOLATION_FILTER_EIGHTTAP;
        uint8_t loopFilterLevel = 0;
        uint8_t loopFilterSharpness = 0;
        bool loopFilterDeltaEnabled = false;
        bool updateRefDelta = false;
        int8_t loopFilterRefDeltas[4] = {1, 0, -1, -1};
        bool updateModeDelta = false;
        int8_t loopFilterModeDeltas[2] = {};
        int32_t baseQIdx = 0;
        int32_t deltaQYDc = 0;
        int32_t deltaQUvDc = 0;
        int32_t deltaQUvAc = 0;
        bool lossless = false;
        bool segmentationEnabled = false;
        bool segmentationUpdateMap = false;
        uint8_t segmentTreeProbs[7] = {};
        uint8_t segmentPredProbs[3] = {};
        bool segmentationTemporalUpdate = false;
        bool segmentationUpdateData = false;
        bool segmentationAbsOrDeltaUpdate = false;
        bool segmentFeatureEnabled[8][4] = {};
        int16_t segmentFeatureData[8][4] = {};

        /// Parses a VP9 bitstream packet and returns the encoded frames.
        ///
        /// Packets needs to be supplied in the order they are appearing in the bitstream. The caller
        /// needs to reset the parser if the bitstream is changed or a seek happened. Not resetting the
        /// parser in such cases results in garbage data or errors.
        Result<std::vector<Frame>> parsePacket(std::vector<uint8_t> packet);

    private:
        Result<size_t> readFrameSize(MutableSlice<uint8_t> entryData, size_t bytesSize, size_t index)
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

        Result<Frame> parseFrame(std::vector<uint8_t> data)
        {
            BitReader br = BitReader({ data.data(), data.size() });

            Result<uint8_t> frameMarker = br.readU8(2);
            if (frameMarker.error)
            {
                return { frameMarker.error };
            }
            if (frameMarker.value != 2)
            {
                return { ERROR_INVALID_FRAME_MARKER };
            }

            Result<uint8_t> profileLowBit = br.readU8(1);
            if (profileLowBit.error)
            {
                return { profileLowBit.error };
            }
            Result<uint8_t> profileHighBit = br.readU8(1);
            if (profileHighBit.error)
            {
                return { profileHighBit.error };
            }
            this->profile = u8ToProfile((profileHighBit.value << 1) + profileLowBit.value);
            if (this->profile == PROFILE_3)
            {
                Result<uint8_t> ReservedZero = br.readU8(1);
                if (ReservedZero.error)
                {
                    return { ReservedZero.error };
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

                Frame frame = Frame::create(*this, 0, 0, 0, {});
                return { OK, frame };
            }
            else
            {
                this->frameToShowMapIdx = { false };
            }

            this->lastFrameType = this->frameType;

            Result<bool> frameTypeBool = br.readBool();
            if (frameTypeBool.error)
            {
                return { frameTypeBool.error };
            }
            this->frameType = boolToFrameType(frameTypeBool.value);

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
                };

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
                };

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
                    } else {
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
            };

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

            size_t size = data.size();
            size_t tileSize = size - (uncompressedHeaderSize + compressedHeaderSize);

            Frame frame = Frame::create(
                *this,
                uncompressedHeaderSize,
                compressedHeaderSize,
                tileSize,
                data
            );

            refreshRefFrames();

            return { OK, frame };
        }

        // Implements spec "8.10 Reference frame update process".
        void refreshRefFrames()
        {
            uint8_t flags = this->refreshFrameFlags;
            uint16_t newWidth = this->width;
            uint16_t newHeight = this->height;

            for (size_t index = 0; index < 8; ++index)
            {
                if (((flags >> index) & 1) == 1)
                {
                    refFrameSizes[index] = { newWidth, newHeight };
                }
            }
        }

        ERROR frameSyncCode(BitReader &br)
        {
            Result<uint8_t> frameSyncByte_0 = br.readU8(8);
            if (frameSyncByte_0.error)
            {
                return frameSyncByte_0.error;
            }
            Result<uint8_t> frameSyncByte_1 = br.readU8(8);
            if (frameSyncByte_1.error)
            {
                return frameSyncByte_1.error;
            }
            Result<uint8_t> frameSyncByte_2 = br.readU8(8);
            if (frameSyncByte_2.error)
            {
                return frameSyncByte_2.error;
            }

            if (frameSyncByte_0.value != 0x49 && frameSyncByte_1.value != 0x83 && frameSyncByte_2.value != 0x42)
            {
                return ERROR_INVALID_SYNC_BYTE;
            }

            return OK;
        }

        ERROR colorConfig(BitReader &br)
        {
            if (this->profile >= PROFILE_2)
            {
                Result<bool> tenOrTwelveBit = br.readBool();
                if (tenOrTwelveBit.error)
                {
                    return tenOrTwelveBit.error;
                }
                if (tenOrTwelveBit.value)
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
            };

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
                    Result<uint8_t> reservedZero = br.readU8(1);
                    if (reservedZero.error)
                    {
                        return reservedZero.error;
                    }
                }
            } else {
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
                    Result<uint8_t> reservedZero = br.readU8(1);
                    if (reservedZero.error)
                    {
                        return reservedZero.error;
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

        ERROR frameSize(BitReader &br)
        {
            Result<uint16_t> frameWidthMinus_1 = br.readU16(16);
            if (frameWidthMinus_1.error)
            {
                return frameWidthMinus_1.error;
            }
            Result<uint16_t> frameHeightMinus_1 = br.readU16(16);
            if (frameHeightMinus_1.error)
            {
                return frameHeightMinus_1.error;
            }
            this->width = frameWidthMinus_1.value + 1;
            this->height = frameHeightMinus_1.value + 1;

            computeImageSize();

            return OK;
        }

        ERROR renderSize(BitReader &br)
        {
            Result<bool> renderAndFrameSizeDifferent = br.readBool();
            if (renderAndFrameSizeDifferent.error)
            {
                return renderAndFrameSizeDifferent.error;
            }
            if (renderAndFrameSizeDifferent.value)
            {
                Result<uint16_t> renderWidthMinus_1 = br.readU16(16);
                if (renderWidthMinus_1.error)
                {
                    return renderWidthMinus_1.error;
                }
                Result<uint16_t> renderHeightMinus_1 = br.readU16(16);
                if (renderHeightMinus_1.error)
                {
                    return renderHeightMinus_1.error;
                }
                this->renderWidth = renderWidthMinus_1.value + 1;
                this->renderHeight = renderHeightMinus_1.value + 1;
            }
            else
            {
                this->renderWidth = this->width;
                this->renderHeight = this->height;
            }

            return OK;
        }

        ERROR frameSizeWithRefs(BitReader &br)
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
                    std::pair<uint16_t, uint16_t> sizes = this->refFrameSizes[refFrameIndex];

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

        void computeImageSize()
        {
            this->miCols = (this->width + 7) >> 3;
            this->miRows = (this->height + 7) >> 3;
        }

        ERROR readInterpolationFilter(BitReader &br)
        {
            Result<bool> isFilterSwitchable = br.readBool();
            if (isFilterSwitchable.error)
            {
                return isFilterSwitchable.error;
            }

            if (isFilterSwitchable.value)
            {
                this->interpolationFilter = INTERPOLATION_FILTER_SWITCHABLE;
            }
            else
            {
                Result<uint8_t> rawInterpolationFilter = br.readU8(2);
                if (rawInterpolationFilter.error)
                {
                    return rawInterpolationFilter.error;
                }
                switch (rawInterpolationFilter.value)
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

        ERROR loopFilterParams(BitReader &br)
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

        ERROR quantizationParams(BitReader &br)
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

        Result<int32_t> readDeltaQ(BitReader &br)
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

        ERROR segmentationParams(BitReader &br)
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
                        };

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
                        };

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
                        };

                        Result<bool> segmentFeatureEnabledSkipResult = br.readBool();
                        if (segmentFeatureEnabledSkipResult.error)
                        {
                            return segmentFeatureEnabledSkipResult.error;
                        }
                        this->segmentFeatureData[i][SEG_LVL_SKIP] = 0;
                    }
                }
            }

            return OK;
        }

        Result<uint8_t> readProb(BitReader &br)
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

        ERROR tileInfo(BitReader &br)
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

        Result<uint8_t> calcMinLog2TileCols()
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

        Result<uint8_t> calcMaxLog2TileCols()
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

        ERROR trailingBits(BitReader &br)
        {
            while (br.isAligned(1))
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

        static constexpr uint32_t u32FromLeBytes(const uint8_t *bytes)
        {
            return static_cast<uint32_t>(bytes[0])
                | (static_cast<uint32_t>(bytes[1]) << 8)
                | (static_cast<uint32_t>(bytes[2]) << 16)
                | (static_cast<uint32_t>(bytes[3]) << 24);
        }

        static constexpr uint16_t u16FromLeBytes(const uint8_t *bytes)
        {
            return static_cast<uint16_t>(bytes[0])
                | (static_cast<uint16_t>(bytes[1]) << 8);
        }

        static constexpr uint8_t u8FromLeBytes(const uint8_t *bytes)
        {
            return bytes[0];
        }
    };
};

#endif // VP9_FRAME_HEADER_PARSER_H