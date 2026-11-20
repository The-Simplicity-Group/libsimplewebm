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

#ifndef VP9_FRAME_PARSER_H
#define VP9_FRAME_PARSER_H

#include <stdint.h>
#include <vector>

namespace VP9FrameParser
{
    /// Contains all the possible errors in VP9FrameParser.
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
    struct Metadata
    {
        PROFILE profile;
        LEVEL level;
        COLOR_DEPTH colorDepth;
        METADATA_SUBSAMPLING chromaSubsampling;

        /// Creates the Vp9Metadata from the given `CodecPrivate` data.
        static Result<Metadata> create(Slice<uint8_t> data);
    };

    struct VP9Parser;

    /// A VP9 frame.
    struct VP9Frame
    {
        /// The raw bitstream in the frame.
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
        ///
        /// The maximum value is 65,536 (`UINT16_MAX` plus one).
        uint32_t width;
        /// The height of the frame.
        ///
        /// The maximum value is 65,536 (`UINT16_MAX` plus one).
        uint32_t height;
        /// A hint for the application for the desired width to render.
        ///
        /// The maximum value is 65,536 (`UINT16_MAX` plus one).
        uint32_t renderWidth;
        /// A hint for the application for the desired height to render.
        ///
        /// The maximum value is 65,536 (`UINT16_MAX` plus one).
        uint32_t renderHeight;
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
        /// Indicates that the the bitstream contains the syntax element `loopFilterRefDeltas`.
        bool loopFilterUpdateRefDelta;
        /// Contains the adjustment needed for the filter level based on the chosen reference frame.
        int8_t loopFilterRefDeltas[4];
        /// Indicates that the the bitstream contains the syntax element `loopFilterModeDeltas`.
        bool loopFilterUpdateModeDelta;
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
        static VP9Frame create(VP9Parser &parser, size_t uncompressedHeaderSize, size_t compressedHeaderSize, size_t tileSize, std::vector<uint8_t> data);
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
    struct VP9Parser
    {
        std::pair<uint32_t, uint32_t> refFrameSizes[8] = {};
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
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t renderWidth = 0;
        uint32_t renderHeight = 0;
        uint16_t miCols = 0;
        uint16_t miRows = 0;
        uint8_t tileRowsLog2 = 0;
        uint8_t tileColsLog2 = 0;
        INTERPOLATION_FILTER interpolationFilter = INTERPOLATION_FILTER_EIGHTTAP;
        uint8_t loopFilterLevel = 0;
        uint8_t loopFilterSharpness = 0;
        bool loopFilterDeltaEnabled = false;
        bool loopFilterUpdateRefDelta = false;
        int8_t loopFilterRefDeltas[4] = { 1, 0, -1, -1 };
        bool loopFilterUpdateModeDelta = false;
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
        Result<std::vector<VP9Frame>> parsePacket(std::vector<uint8_t> packet);

    private:
        Result<uint32_t> readFrameSize(MutableSlice<uint8_t> entryData, size_t bytesSize, size_t index);
        Result<VP9Frame> parseFrame(std::vector<uint8_t> data);
        /// Implements spec "8.10 Reference frame update process".
        void refreshRefFrames();
        ERROR frameSyncCode(BitReader &br);
        ERROR colorConfig(BitReader &br);
        ERROR frameSize(BitReader &br);
        ERROR renderSize(BitReader &br);
        ERROR frameSizeWithRefs(BitReader &br);
        void computeImageSize();
        ERROR readInterpolationFilter(BitReader &br);
        ERROR loopFilterParams(BitReader &br);
        ERROR quantizationParams(BitReader &br);
        Result<int32_t> readDeltaQ(BitReader &br);
        ERROR segmentationParams(BitReader &br);
        Result<uint8_t> readProb(BitReader &br);
        ERROR tileInfo(BitReader &br);
        Result<uint8_t> calcMinLog2TileCols();
        Result<uint8_t> calcMaxLog2TileCols();
        ERROR trailingBits(BitReader &br);

        static constexpr uint32_t u32FromLeBytes(const uint8_t *bytes);
        static constexpr uint16_t u16FromLeBytes(const uint8_t *bytes);
        static constexpr uint8_t u8FromLeBytes(const uint8_t *bytes);
        static ERROR truncateVectorU8(std::vector<uint8_t> &vector, size_t newSize);
    };
};

#endif // VP9_FRAME_PARSER_H