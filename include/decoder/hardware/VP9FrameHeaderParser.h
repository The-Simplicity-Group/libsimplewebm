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
 *    This file was ported from vp9-parser (Rust) by Nils Hasenbanck.
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
#include <map>
#include <vector>

class VP9FrameHeaderParser
{
    /// Contains all the possible errors in VP9FrameHeaderParser.
    enum ERROR : uint8_t
    {
        OK,
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
    template<typename T>
    struct Option
    {
        bool has_value = false;
        T value = {};
    };

    /// Equivalent for Rust's slice type.
    template<typename T>
    struct Slice
    {
        const T *ptr = nullptr;
        size_t len = 0;
    };

    /// Equivalent for Rust's mutable slice type.
    template<typename T>
    struct MutableSlice
    {
        T *ptr = nullptr;
        size_t len = 0;
    };

    /// Equivalent for Rust's BitReader type.
    struct BitReader
    {
        const uint8_t *data = nullptr;
        size_t size_in_bits = 0;
        size_t position_in_bits = 0;

        BitReader(Slice<uint8_t> slice)
        {
            data = slice.ptr;
            size_in_bits = slice.len * 8;
        }

        template <typename T>
        ERROR read_unsigned(uint8_t count, T &result)
        {
            if (count > (sizeof(T) * 8) || count > get_remaining_bits())
            {
                return ERROR_BIT_READER;
            }

            Result<uint64_t> result = read_u64(count);
            if (result.error)
            {
                return result.error;
            }

            result = static_cast<T>(result.value);
            return OK;
        }

        ERROR read_u64(uint8_t count, uint64_t &result)
        {
            return read_unsigned(count, result);
        }

        ERROR read_u32(uint8_t count, uint32_t &result)
        {
            return read_unsigned(count, result);
        }

        ERROR read_u16(uint8_t count, uint16_t &result)
        {
            return read_unsigned(count, result);
        }

        ERROR read_u8(uint8_t count, uint8_t &result)
        {
            return read_unsigned(count, result);
        }

        ERROR read_bool(bool &result)
        {
            return read_unsigned(1, result);
        }

        size_t get_remaining_bits() const
        {
            return size_in_bits - position_in_bits;
        }
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

    static PROFILE int_to_profile(uint8_t value)
    {
        switch (value)
        {
            case 0: return PROFILE_0;
            case 1: return PROFILE_1;
            case 2: return PROFILE_2;
            case 3: return PROFILE_3;
            default: return PROFILE_UNKNOWN;
        }
    }
    static uint8_t profile_to_int(PROFILE value)
    {
        switch (value)
        {
            case PROFILE_0: return 0;
            case PROFILE_1: return 1;
            case PROFILE_2: return 2;
            case PROFILE_3: return 3;
            default: return UINT8_MAX;
        }
    }

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

    static METADATA_SUBSAMPLING int_to_metadata_subsampling(uint8_t value)
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

    static COLOR_SPACE int_to_color_space(uint8_t value)
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

    static COLOR_DEPTH int_to_color_depth(uint8_t value)
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

    /// Specifies the black level and range of the luma and chroma signals as specified in
    /// Rec. ITU-R BT.709-6 and Rec. ITU-R BT.2020-2.
    enum COLOR_RANGE : uint8_t
    {
        /// Put restriction on Y, U, V values.
        COLOR_RANGE_STUDIO_SWING,
        /// No restriction on Y, U, V values.
        COLOR_RANGE_FULL_SWING,
    };

    static COLOR_RANGE int_to_color_range(bool value)
    {
        switch (value)
        {
            case false:
                return COLOR_RANGE_STUDIO_SWING;
            case true:
                return COLOR_RANGE_FULL_SWING;
        }
    }

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

    static FRAME_TYPE int_to_frame_type(bool value)
    {
        switch (value)
        {
            case false:
                return FRAME_TYPE_KEY_FRAME;
            case true:
                return FRAME_TYPE_NON_KEY_FRAME;
        }
    }

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

    static RESET_FRAME_CONTEXT int_to_reset_frame_context(uint8_t value)
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

    static LEVEL int_to_level(uint8_t value)
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

    /// VP9 Codec Feature Metadata saved inside the `CodecPrivate` field of containers.
    struct Metadata {
        PROFILE profile;
        LEVEL level;
        COLOR_DEPTH color_depth;
        METADATA_SUBSAMPLING chroma_subsampling;

        /// Creates the Vp9Metadata from the given `CodecPrivate` data.
        static ERROR create(Slice<uint8_t> data, Metadata &metadata)
        {
            bool has_profile = false;
            bool has_level = false;
            bool has_color_depth = false;
            bool has_chroma_subsampling = false;

            int32_t pos = 0;

            while (pos < data.len)
            {
                if (pos + 1 >= data.len)
                {
                    return ERROR_INVALID_METADATA;
                }

                uint8_t id = data.ptr[pos];
                uint8_t value = data.ptr[pos + 1];
                pos += 2;

                switch (id)
                {
                    case 1:
                        has_profile = true;
                        metadata.profile = int_to_profile(value);
                        break;
                    case 2:
                        has_level = true;
                        metadata.level = int_to_level(value);
                        break;
                    case 3:
                        has_color_depth = true;
                        metadata.color_depth = int_to_color_depth(value);
                        break;
                    case 4:
                        has_chroma_subsampling = true;
                        metadata.chroma_subsampling = int_to_metadata_subsampling(value);
                        break;
                }
            }

            if (!has_profile || !has_level || !has_color_depth || !has_chroma_subsampling)
            {
                return ERROR_INVALID_METADATA;
            }

            return OK;
        }
    };

    struct VP9Parser;

    /// A VP9 frame.
    struct Frame {
        std::vector<uint8_t> data;
        /// The profile the frame is using.
        PROFILE profile;
        /// Indicates that the frame indexed by `frame_to_show_map_idx` is to be displayed.
        /// The frame contains no actual frame data.
        bool show_existing_frame;
        /// Specifies the frame to be displayed. It is only available if `show_existing_frame` is true.
        Option<uint8_t> frame_to_show_map_idx;
        /// The frame type of the previous frame.
        FRAME_TYPE last_frame_type;
        /// The frame type of this frame.
        FRAME_TYPE frame_type;
        /// Indicates that the frame is to be displayed.
        bool show_frame;
        /// Indicates that error resilient mode is enabled.
        ///
        /// Error resilient mode allows the syntax of a frame to be decoded
        /// independently of previous frames.
        bool error_resilient_mode;
        /// Indicates that a frame is an `intra-only` frame.
        ///
        /// A key frame is different to an `intra-only` frame even though both only use
        /// intra prediction. The difference is that a key frame fully resets the decoding process.
        bool intra_only;
        /// Specifies whether the frame context should be reset to default values.
        RESET_FRAME_CONTEXT reset_frame_context;
        /// The indices of the used reference frames.
        uint8_t ref_frame_indices[3];
        /// Specifies the intended direction of the motion vector in time for each reference frame.
        bool ref_frame_sign_bias[4];
        /// Specifies the precision of the motion vectors.
        ///
        /// False = quarter precision, True = eighth precision.
        bool allow_high_precision_mv;
        /// Specifies that the probabilities computed for this frame
        /// should be stored for reference by future frames.
        bool refresh_frame_context;
        /// Contains a bitmask that specifies which reference frame slots
        /// will be updated with the current frame after it is decoded.
        ///
        /// First bit = first frame (1). Last bit = last frame (8).
        uint8_t refresh_frame_flags;
        /// Specifies if parallel decoding mode is activated.
        bool frame_parallel_decoding_mode;
        /// Specifies which frame context to use.
        uint8_t frame_context_idx;
        /// The size of the uncompressed header.
        size_t uncompressed_header_size;
        /// The size of the uncompressed header.
        size_t compressed_header_size;
        /// The size of the tile data.
        size_t tile_size;
        /// The color depth of the frame.
        COLOR_DEPTH color_depth;
        /// The color space of the frame.
        COLOR_SPACE color_space;
        /// The color range of the frame.
        COLOR_RANGE color_range;
        /// Indicates if sub sampling is used along the x axis.
        bool subsampling_x;
        /// Indicates if sub sampling is used along the y axis.
        bool subsampling_y;
        /// The width of the frame.
        uint16_t width;
        /// The height of the frame.
        uint16_t height;
        /// A hint for the application for the desired width to render.
        uint16_t render_width;
        /// A hint for the application for the desired height to render.
        uint16_t render_height;
        /// A variable holding the vertical location of the block in units of 8x8 pixels.
        uint16_t mi_cols;
        /// A variable holding the horizontal location of the block in units of 8x8 pixels.
        uint16_t mi_rows;
        /// The base 2 logarithm of the height of each tile (where the height is measured in units
        /// of 8x8 blocks)
        uint8_t tile_rows_log2;
        /// The base 2 logarithm of the width of each tile (where the width is measured in units
        /// of 8x8 blocks)
        uint8_t tile_cols_log2;
        /// The type of filter used in inter prediction.
        INTERPOLATION_FILTER interpolation_filter;
        /// The loop filter strength.
        uint8_t loop_filter_level;
        /// The loop filter sharpness.
        uint8_t loop_filter_sharpness;
        /// Indicates that the filter level depends on the mode and reference frame
        /// used to predict a block.
        bool loop_filter_delta_enabled;
        /// Indicates that the the bitstream contains the syntax element loop_filter_ref_delta.
        bool update_ref_delta;
        /// Contains the adjustment needed for the filter level based on the chosen reference frame.
        int8_t loop_filter_ref_deltas[4];
        /// Indicates that the the bitstream contains the syntax element loop_filter_mode_deltas.
        bool update_mode_delta;
        /// Contains the adjustment needed for the filter level based on the chosen mode.
        int8_t loop_filter_mode_deltas[2];
        /// The base frame qindex. This is used for Y AC coefficients and as the base value
        /// for the other quantizers.
        int32_t base_q_idx;
        /// The Y DC quantizer relative to base_q_idx.
        int32_t delta_q_y_dc;
        /// The UV DC quantizer relative to base_q_idx.
        int32_t delta_q_uv_dc;
        /// The UV AC quantizer relative to base_q_idx.
        int32_t delta_q_uv_ac;
        /// Indicates that the frame is coded using a special 4x4 transform designed
        /// for encoding frames that are bit-identical with the original frames.
        bool lossless;
        /// Specifies that this frame makes use of the segmentation tool.
        bool segmentation_enabled;
        /// Specifies that the segmentation map should be updated during the decoding of this frame.
        bool segmentation_update_map;
        /// The probability values to be used when decoding segment_id.
        uint8_t segment_tree_probs[7];
        /// The probability values to be used when decoding seg_id_predicted.
        uint8_t segment_pred_probs[3];
        /// Indicates that the updates to the segmentation map are coded
        /// relative to the existing segmentation map.
        bool segmentation_temporal_update;
        /// Indicates that new parameters are about to be specified for each segment.
        bool segmentation_update_data;
        /// Indicates that the segmentation parameters represent the actual values to be used,
        /// otherwise the segmentation parameters represent adjustments relative to the standard values.
        bool segmentation_abs_or_delta_update;
        /// Indicates that the corresponding feature is used in a segment.
        bool segment_feature_enabled[8][4];
        /// Specifies the values of the active features of a segment.
        int16_t segment_feature_data[8][4];

        /// Creates a frame from the parser state.
        static Frame create(VP9Parser &parser, size_t uncompressed_header_size, size_t compressed_header_size, size_t tile_size, std::vector<uint8_t> data)
        {
            Frame frame;

            frame.data = data;
            frame.profile = parser.profile;
            frame.show_existing_frame = parser.show_existing_frame;
            frame.frame_to_show_map_idx = parser.frame_to_show_map_idx;
            frame.last_frame_type = parser.last_frame_type;
            frame.frame_type = parser.frame_type;
            frame.show_frame = parser.show_frame;
            frame.error_resilient_mode = parser.error_resilient_mode;
            frame.intra_only = parser.intra_only;
            frame.reset_frame_context = parser.reset_frame_context;
            memcpy(frame.ref_frame_indices, parser.ref_frame_indices, sizeof(parser.ref_frame_indices));
            memcpy(frame.ref_frame_sign_bias, parser.ref_frame_sign_bias, sizeof(parser.ref_frame_sign_bias));
            frame.allow_high_precision_mv = parser.allow_high_precision_mv;
            frame.refresh_frame_context = parser.refresh_frame_context;
            frame.refresh_frame_flags = parser.refresh_frame_flags;
            frame.frame_parallel_decoding_mode = parser.frame_parallel_decoding_mode;
            frame.frame_context_idx = parser.frame_context_idx;
            frame.uncompressed_header_size = uncompressed_header_size;
            frame.compressed_header_size = compressed_header_size;
            frame.tile_size = tile_size;
            frame.color_depth = parser.color_depth;
            frame.color_space = parser.color_space;
            frame.color_range = parser.color_range;
            frame.subsampling_x = parser.subsampling_x;
            frame.subsampling_y = parser.subsampling_y;
            frame.width = parser.width;
            frame.height = parser.height;
            frame.render_width = parser.render_width;
            frame.render_height = parser.render_height;
            frame.mi_cols = parser.mi_cols;
            frame.mi_rows = parser.mi_rows;
            frame.tile_rows_log2 = parser.tile_rows_log2;
            frame.tile_cols_log2 = parser.tile_cols_log2;
            frame.interpolation_filter = parser.interpolation_filter;
            frame.loop_filter_level = parser.loop_filter_level;
            frame.loop_filter_sharpness = parser.loop_filter_sharpness;
            frame.loop_filter_delta_enabled = parser.loop_filter_delta_enabled;
            frame.update_ref_delta = parser.update_ref_delta;
            memcpy(frame.loop_filter_ref_deltas, parser.loop_filter_ref_deltas, sizeof(parser.loop_filter_ref_deltas));
            frame.update_mode_delta = parser.update_mode_delta;
            memcpy(frame.loop_filter_mode_deltas, parser.loop_filter_mode_deltas, sizeof(parser.loop_filter_mode_deltas));
            frame.base_q_idx = parser.base_q_idx;
            frame.delta_q_y_dc = parser.delta_q_y_dc;
            frame.delta_q_uv_dc = parser.delta_q_uv_dc;
            frame.delta_q_uv_ac = parser.delta_q_uv_ac;
            frame.lossless = parser.lossless;
            frame.segmentation_enabled = parser.segmentation_enabled;
            frame.segmentation_update_map = parser.segmentation_update_map;
            memcpy(frame.segment_tree_probs, parser.segment_tree_probs, sizeof(parser.segment_tree_probs));
            memcpy(frame.segment_pred_probs, parser.segment_pred_probs, sizeof(parser.segment_pred_probs));
            frame.segmentation_temporal_update = parser.segmentation_temporal_update;
            frame.segmentation_update_data = parser.segmentation_update_data;
            frame.segmentation_abs_or_delta_update = parser.segmentation_abs_or_delta_update;
            memcpy(frame.segment_feature_enabled, parser.segment_feature_enabled, sizeof(parser.segment_feature_enabled));
            memcpy(frame.segment_feature_data, parser.segment_feature_data, sizeof(parser.segment_feature_data));

            return frame;
        }

        /// Returns a slice into the data of the compressed header.
        Slice<uint8_t> compressed_header_data()
        {
            return {
                data.data() + uncompressed_header_size,
                compressed_header_size,
            };
        }

        /// Returns a slice into the data of the compressed header and tile data.
        Slice<uint8_t> compressed_header_and_tile_data()
        {
            return {
                data.data() + uncompressed_header_size,
                data.size() - uncompressed_header_size,
            };
        }

        /// Returns a slice into the data of the tile data.
        Slice<uint8_t> tile_data()
        {
            return {
                data.data() + uncompressed_header_size + compressed_header_size,
                data.size() - uncompressed_header_size - compressed_header_size,
            };
        }

        /// Last reference frame index.
        uint8_t last_ref_frame_index()
        {
            return ref_frame_indices[0];
        }

        /// Golden reference frame index.
        uint8_t golden_ref_frame_index()
        {
            return ref_frame_indices[1];
        }

        /// Alternate reference frame index.
        uint8_t alt_ref_frame_index()
        {
            return ref_frame_indices[2];
        }

        /// The subsampling the frame is using.
        SUBSAMPLING subsampling() {
            if (!subsampling_x && !subsampling_y)
            {
                return SUBSAMPLING_YUV_444;
            }
            else if (!subsampling_x && subsampling_y)
            {
                return SUBSAMPLING_YUV_440;
            }
            else if (subsampling_x && !subsampling_y)
            {
                return SUBSAMPLING_YUV_422;
            }
            else
            {
                return SUBSAMPLING_YUV_420;
            }
        }
    };

    /// Parses VP9 bitstreams.
    struct VP9Parser {
        std::pair<uint16_t, uint16_t> ref_frame_sizes[8] = {};
        PROFILE profile = PROFILE_0;
        bool show_existing_frame = false;
        Option<uint8_t> frame_to_show_map_idx = { false };
        FRAME_TYPE last_frame_type = FRAME_TYPE_NON_KEY_FRAME;
        FRAME_TYPE frame_type = FRAME_TYPE_NON_KEY_FRAME;
        bool show_frame = false;
        bool error_resilient_mode = false;
        bool intra_only = false;
        RESET_FRAME_CONTEXT reset_frame_context = RESET_FRAME_CONTEXT_NO_0;
        uint8_t ref_frame_indices[3] = {};
        bool ref_frame_sign_bias[4] = {};
        bool allow_high_precision_mv = false;
        bool refresh_frame_context = false;
        uint8_t refresh_frame_flags = 0;
        bool frame_parallel_decoding_mode = true;
        uint8_t frame_context_idx = 0;
        COLOR_DEPTH color_depth = COLOR_DEPTH_8;
        COLOR_SPACE color_space = COLOR_SPACE_UNKNOWN;
        COLOR_RANGE color_range = COLOR_RANGE_STUDIO_SWING;
        bool subsampling_x = true;
        bool subsampling_y = true;
        uint16_t width = 0;
        uint16_t height = 0;
        uint16_t render_width = 0;
        uint16_t render_height = 0;
        uint16_t mi_cols = 0;
        uint16_t mi_rows = 0;
        uint8_t tile_rows_log2 = 0;
        uint8_t tile_cols_log2 = 0;
        INTERPOLATION_FILTER interpolation_filter = INTERPOLATION_FILTER_EIGHTTAP;
        uint8_t loop_filter_level = 0;
        uint8_t loop_filter_sharpness = 0;
        bool loop_filter_delta_enabled = false;
        bool update_ref_delta = false;
        int8_t loop_filter_ref_deltas[4] = {1, 0, -1, -1};
        bool update_mode_delta = false;
        int8_t loop_filter_mode_deltas[2] = {};
        int32_t base_q_idx = 0;
        int32_t delta_q_y_dc = 0;
        int32_t delta_q_uv_dc = 0;
        int32_t delta_q_uv_ac = 0;
        bool lossless = false;
        bool segmentation_enabled = false;
        bool segmentation_update_map = false;
        uint8_t segment_tree_probs[7] = {};
        uint8_t segment_pred_probs[3] = {};
        bool segmentation_temporal_update = false;
        bool segmentation_update_data = false;
        bool segmentation_abs_or_delta_update = false;
        bool segment_feature_enabled[8][4] = {};
        int16_t segment_feature_data[8][4] = {};

        /// Parses a VP9 bitstream packet and returns the encoded frames.
        ///
        /// Packets needs to be supplied in the order they are appearing in the bitstream. The caller
        /// needs to reset the parser if the bitstream is changed or a seek happened. Not resetting the
        /// parser in such cases results in garbage data or errors.
        ERROR parse_packet(std::vector<uint8_t> packet, std::vector<Frame> &frames)
        {
            if (packet.empty())
            {
                return OK;
            }

            // Test for a super frame.
            size_t last_byte_index = packet.size() - 1;
            uint8_t last_byte = packet[last_byte_index];
            if (last_byte & 0b1110'0000 == 0b1100'0000)
            {
                uint8_t bytes_per_framesize_minus_1 = (last_byte & 0b11000) >> 3;
                uint8_t frames_in_superframe_minus_1 = last_byte & 0b111;
                size_t bytes_size = bytes_per_framesize_minus_1 + 1;
                size_t frame_count = frames_in_superframe_minus_1 + 1;
                size_t index_size = 2 + (frame_count * bytes_size);
                size_t first_byte_index = packet.size() - index_size;
                uint8_t first_byte = packet[first_byte_index];

                // Found a super frame.
                if (first_byte == last_byte)
                {
                    frames.reserve(frame_count);

                    size_t index_start = first_byte_index + 1;
                    size_t entry_size = frame_count * bytes_size;

                    std::vector<uint8_t> entry_data = {};
                    entry_data.reserve(entry_size);
                    entry_data.insert(entry_data.end(), packet.begin() + index_start, packet.begin() + index_start + entry_size);

                    switch (frame_count)
                    {
                        case 1:
                        {
                            // Odd, but valid bitstream configuration.
                            size_t frame_size;
                            ERROR frame_size_error = read_frame_size({ entry_data.data(), entry_data.size() }, bytes_size, 0, frame_size);
                            if (frame_size_error)
                            {
                                return frame_size_error;
                            }

                            packet.resize(frame_size);
                            Frame frame;
                            ERROR frame_error = parse_frame(packet, frame);
                            if (frame_error)
                            {
                                return frame_error;
                            }

                            frames.push_back(frame);
                            break;
                        }
                        case 2:
                        {
                            // Most common case. The first frame produces a frame that is not displayed but
                            // stored as a reference frame. The second frame is mostly empty and references
                            // the previously stored frame.
                            size_t first_frame_size;
                            ERROR first_frame_size_error = read_frame_size({ entry_data.data(), entry_data.size() }, bytes_size, 0, first_frame_size);
                            if (first_frame_size_error)
                            {
                                return first_frame_size_error;
                            }
                            std::vector<uint8_t> left_over = std::vector<uint8_t>(packet.begin() + first_frame_size, packet.end());
                            packet.resize(first_frame_size);
                            Frame first_frame;
                            ERROR first_frame_error = parse_frame(packet, first_frame);
                            if (first_frame_error)
                            {
                                return first_frame_error;
                            }

                            size_t second_frame_size;
                            ERROR second_frame_size_error = read_frame_size({ entry_data.data(), entry_data.size() }, bytes_size, 1, second_frame_size);
                            if (second_frame_size_error)
                            {
                                return second_frame_size_error;
                            }
                            left_over.resize(second_frame_size);
                            Frame second_frame;
                            ERROR second_frame_error = parse_frame(left_over, second_frame);
                            if (second_frame_error)
                            {
                                return second_frame_error;
                            }

                            frames.push_back(first_frame);
                            frames.push_back(second_frame);
                            break;
                        }
                        default:
                        {
                            // Odd, but also a valid bitstream configuration.
                            for (size_t frame_index = 0; frame_index < frame_count; frame_index++)
                            {
                                size_t frame_size;
                                ERROR frame_size_error = read_frame_size({ entry_data.data(), entry_data.size() }, bytes_size, frame_index, frame_size);
                                if (frame_size_error)
                                {
                                    return frame_size_error;
                                }

                                std::vector<uint8_t> left_over = std::vector<uint8_t>(packet.begin() + frame_size, packet.end());
                                packet.resize(frame_size);
                                Frame frame;
                                ERROR frame_error = parse_frame(packet, frame);
                                if (frame_error)
                                {
                                    return frame_error;
                                }
                                frames.push_back(frame);

                                packet = left_over;
                            }
                            break;
                        }
                    }

                    return OK;
                }
            }

            // Normal frame.
            Frame frame;
            ERROR frame_error = parse_frame(packet, frame);
            if (frame_error)
            {
                return frame_error;
            }
            frames.push_back(frame);
            return OK;
        }

    private:
        ERROR read_frame_size(MutableSlice<uint8_t> entry_data, size_t bytes_size, size_t index, size_t &frame_size)
        {
            // sic! Even though the values inside the uncompressed header are saved in BE,
            // these values are saved in LE.
            switch (bytes_size)
            {
                case 1:
                {
                    frame_size = u8_from_le_bytes(entry_data.ptr + index);
                    return OK;
                }
                case 2:
                {
                    frame_size = u16_from_le_bytes(entry_data.ptr + (index * 2));
                    return OK;
                }
                case 3:
                {
                    uint8_t *three_bytes = entry_data.ptr + (index * 3);
                    uint8_t four_bytes[4] = { three_bytes[0], three_bytes[1], three_bytes[2], 0 };
                    frame_size = u32_from_le_bytes(four_bytes);
                    return OK;
                }
                case 4:
                {
                    frame_size = u32_from_le_bytes(entry_data.ptr + (index * 4));
                    return OK;
                }
                default:
                {
                    return ERROR_INVALID_FRAME_SIZE_BYTE_SIZE;
                }
            };
        }

        ERROR parse_frame(std::vector<uint8_t> data, Frame &frame)
        {
            BitReader br = BitReader({ data.data(), data.size() });

            uint8_t frame_marker;
            ERROR frame_marker_error = br.read_u8(2, frame_marker);
            if (frame_marker_error)
            {
                return frame_marker_error;
            }
            if (frame_marker != 2) {
                return ERROR_INVALID_FRAME_MARKER;
            }

            uint8_t profile_low_bit;
            ERROR profile_low_bit_error = br.read_u8(1, profile_low_bit);
            if (profile_low_bit_error)
            {
                return profile_low_bit_error;
            }
            uint8_t profile_high_bit;
            ERROR profile_high_bit_error = br.read_u8(1, profile_high_bit);
            if (profile_high_bit_error)
            {
                return profile_high_bit_error;
            }
            this->profile = int_to_profile((profile_high_bit << 1) + profile_low_bit);
            if (this->profile == PROFILE_3)
            {
                uint8_t reserved_zero;
                ERROR reserved_zero_error = br.read_u8(1, reserved_zero);
                if (reserved_zero_error)
                {
                    return reserved_zero_error;
                }
            }

            ERROR show_existing_frame_error = br.read_bool(this->show_existing_frame);
            if (show_existing_frame_error)
            {
                return show_existing_frame_error;
            }

            if (this->show_existing_frame)
            {
                uint8_t frame_to_show_map_idx_value;
                ERROR frame_to_show_map_idx_error = br.read_u8(3, frame_to_show_map_idx_value);
                if (frame_to_show_map_idx_error)
                {
                    return frame_to_show_map_idx_error;
                }
                this->frame_to_show_map_idx = { true, frame_to_show_map_idx_value };
                this->refresh_frame_flags = 0;
                this->loop_filter_level = 0;

                frame = Frame::create(*this, 0, 0, 0, {});
                return OK;
            }
            else
            {
                this->frame_to_show_map_idx = { false };
            }

            this->last_frame_type = this->frame_type;

            Result<bool> frame_type_bool = br.read_bool();
            if (frame_type_bool.error)
            {
                return { frame_type_bool.error };
            }
            this->frame_type = int_to_frame_type(frame_type_bool.value);

            Result<bool> show_frame_result = br.read_bool();
            if (show_frame_result.error)
            {
                return { show_frame_result.error };
            }
            this->show_frame = show_frame_result.value;

            Result<bool> error_resilient_mode_result = br.read_bool();
            if (error_resilient_mode_result.error)
            {
                return { error_resilient_mode_result.error };
            }
            this->error_resilient_mode = error_resilient_mode_result.value;

            if (this->frame_type == FRAME_TYPE_KEY_FRAME)
            {
                ERROR frame_sync_code_error = frame_sync_code(br);
                if (frame_sync_code_error)
                {
                    return { frame_sync_code_error };
                }
                ERROR color_config_error = color_config(br);
                if (color_config_error)
                {
                    return { color_config_error };
                }
                ERROR frame_size_error = frame_size(br);
                if (frame_size_error)
                {
                    return { frame_size_error };
                }
                ERROR render_size_error = render_size(br);
                if (render_size_error)
                {
                    return { render_size_error };
                }
                this->refresh_frame_flags = 0xFF;
            }
            else
            {
                if (!this->show_frame)
                {
                    Result<bool> intra_only_result = br.read_bool();
                    if (intra_only_result.error)
                    {
                        return { intra_only_result.error };
                    }
                    this->intra_only = intra_only_result.value;
                }
                else
                {
                    this->intra_only = false;
                };

                if (!this->error_resilient_mode)
                {
                    Result<uint8_t> reset_frame_context_result = br.read_u8(2);
                    if (reset_frame_context_result.error)
                    {
                        return { reset_frame_context_result.error };
                    }
                    this->reset_frame_context = int_to_reset_frame_context(reset_frame_context_result.value);
                }
                else
                {
                    this->reset_frame_context = RESET_FRAME_CONTEXT_NO_0;
                };

                if (this->intra_only)
                {
                    ERROR frame_sync_code_error = frame_sync_code(br);
                    if (frame_sync_code_error)
                    {
                        return { frame_sync_code_error };
                    }
                    if (this->profile > PROFILE_0) {
                        ERROR color_config_error = color_config(br);
                        if (color_config_error)
                        {
                            return { color_config_error };
                        }
                    } else {
                        this->color_depth = COLOR_DEPTH_8;
                        this->color_space = COLOR_SPACE_BT601;
                        this->subsampling_x = true;
                        this->subsampling_y = true;
                    }
                    Result<uint8_t> refresh_frame_flags_result = br.read_u8(8);
                    if (refresh_frame_flags_result.error)
                    {
                        return { refresh_frame_flags_result.error };
                    }
                    this->refresh_frame_flags = refresh_frame_flags_result.value;
                    ERROR frame_size_error = frame_size(br);
                    if (frame_size_error)
                    {
                        return { frame_size_error };
                    }
                    ERROR render_size_error = render_size(br);
                    if (render_size_error)
                    {
                        return { render_size_error };
                    }
                }
                else
                {
                    Result<uint8_t> refresh_frame_flags_result = br.read_u8(8);
                    if (refresh_frame_flags_result.error)
                    {
                        return { refresh_frame_flags_result.error };
                    }
                    this->refresh_frame_flags = refresh_frame_flags_result.value;
                    for (size_t i = 0; i < 3; i++)
                    {
                        Result<uint8_t> ref_frame_indices_result = br.read_u8(3);
                        if (ref_frame_indices_result.error)
                        {
                            return { ref_frame_indices_result.error };
                        }
                        this->ref_frame_indices[i] = ref_frame_indices_result.value;

                        Result<bool> ref_frame_sign_bias_result = br.read_bool();
                        if (ref_frame_sign_bias_result.error)
                        {
                            return { ref_frame_sign_bias_result.error };
                        }
                        this->ref_frame_sign_bias[LAST_FRAME + i] = ref_frame_sign_bias_result.value;
                    }
                    ERROR frame_size_with_refs_error = frame_size_with_refs(br);
                    if (frame_size_with_refs_error)
                    {
                        return { frame_size_with_refs_error };
                    }
                    Result<bool> allow_high_precision_mv_result = br.read_bool();
                    if (allow_high_precision_mv_result.error)
                    {
                        return { allow_high_precision_mv_result.error };
                    }
                    ERROR read_interpolation_filter_error = read_interpolation_filter(br);
                    if (read_interpolation_filter_error) {
                        return { read_interpolation_filter_error };
                    }
                }
            }

            if (!this->error_resilient_mode)
            {
                Result<bool> refresh_frame_context_result = br.read_bool();
                if (refresh_frame_context_result.error)
                {
                    return { refresh_frame_context_result.error };
                }
                this->refresh_frame_context = refresh_frame_context_result.value;

                Result<bool> frame_parallel_decoding_mode_result = br.read_bool();
                if (frame_parallel_decoding_mode_result.error)
                {
                    return { frame_parallel_decoding_mode_result.error };
                }
                this->frame_parallel_decoding_mode = frame_parallel_decoding_mode_result.value;
            }
            else
            {
                this->refresh_frame_context = false;
                this->frame_parallel_decoding_mode = false;
            };

            Result<uint8_t> frame_context_idx_result = br.read_u8(2);
            if (frame_context_idx_result.error)
            {
                return { frame_context_idx_result.error };
            }
            this->frame_context_idx = frame_context_idx_result.value;

            if (this->intra_only || this->error_resilient_mode)
            {
                this->frame_context_idx = 0;
            }

            if (this->frame_type == FRAME_TYPE_KEY_FRAME || this->error_resilient_mode || this->intra_only)
            {
                // Reset the loop filter deltas.
                this->loop_filter_ref_deltas[INTRA_FRAME] = 1;
                this->loop_filter_ref_deltas[LAST_FRAME] = 0;
                this->loop_filter_ref_deltas[GOLDEN_FRAME] = -1;
                this->loop_filter_ref_deltas[ALTREF_FRAME] = -1;
                this->loop_filter_mode_deltas[0] = 0;
                this->loop_filter_mode_deltas[1] = 0;
            }
            ERROR loop_filter_params_error = loop_filter_params(br);
            if (loop_filter_params_error)
            {
                return { loop_filter_params_error };
            }

            ERROR quantization_params_error = quantization_params(br);
            if (quantization_params_error)
            {
                return { quantization_params_error };
            }
            ERROR segmentation_params_error = segmentation_params(br);
            if (segmentation_params_error)
            {
                return { segmentation_params_error };
            }
            ERROR tile_info_error = tile_info(br);
            if (tile_info_error)
            {
                return { tile_info_error };
            }

            Result<uint16_t> compressed_header_size_result = br.read_u16(16);
            if (compressed_header_size_result.error)
            {
                return { compressed_header_size_result.error };
            }
            size_t compressed_header_size = compressed_header_size_result.value;
            ERROR trailing_bits_error = trailing_bits(br);
            if (trailing_bits_error)
            {
                return { trailing_bits_error };
            }
            size_t uncompressed_header_size = br.position_in_bits / 8;

            size_t size = data.size();
            size_t tile_size = size - (uncompressed_header_size + compressed_header_size);

            Frame frame = Frame::create(
                *this,
                uncompressed_header_size,
                compressed_header_size,
                tile_size,
                data
            );

            refresh_ref_frames();

            return { OK, frame };
        }

        void refresh_ref_frames();

        ERROR frame_sync_code(BitReader &bit_reader);

        ERROR color_config(BitReader &bit_reader);

        ERROR frame_size(BitReader &bit_reader);

        ERROR render_size(BitReader &bit_reader);

        ERROR frame_size_with_refs(BitReader &bit_reader);

        ERROR compute_image_size();

        ERROR read_interpolation_filter(BitReader &bit_reader);

        ERROR loop_filter_params(BitReader &bit_reader);

        ERROR quantization_params(BitReader &bit_reader);

        ERROR read_delta_q(BitReader &bit_reader);

        ERROR segmentation_params(BitReader &bit_reader);

        ERROR read_prob(BitReader &bit_reader);

        ERROR tile_info(BitReader &bit_reader);

        ERROR calc_min_log2_tile_cols(BitReader &bit_reader);

        ERROR calc_max_log2_tile_cols(BitReader &bit_reader);

        ERROR trailing_bits(BitReader &bit_reader);

        static constexpr uint32_t u32_from_le_bytes(const uint8_t *bytes)
        {
            return static_cast<uint32_t>(bytes[0])
                | (static_cast<uint32_t>(bytes[1]) << 8)
                | (static_cast<uint32_t>(bytes[2]) << 16)
                | (static_cast<uint32_t>(bytes[3]) << 24);
        }

        static constexpr uint16_t u16_from_le_bytes(const uint8_t *bytes)
        {
            return static_cast<uint16_t>(bytes[0])
                | (static_cast<uint16_t>(bytes[1]) << 8);
        }

        static constexpr uint8_t u8_from_le_bytes(const uint8_t *bytes)
        {
            return bytes[0];
        }
    };
};

#endif // VP9_FRAME_HEADER_PARSER_H