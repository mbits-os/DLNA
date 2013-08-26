/*
 * Copyright (C) 2013 midnightBITS
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "pch.h"
#include "dlna_media_internal.hpp"
#include <boost/filesystem/fstream.hpp>

namespace fs = boost::filesystem;

namespace net
{
	namespace dlna
	{
		namespace video
		{
			namespace mpeg4
			{
				namespace part2
				{
					static Profile mp4_sp_aac                  { "MPEG4_P2_MP4_SP_AAC",                mime::VIDEO_MPEG_4,  labels::VIDEO_CIF30,   Class::Video };
					static Profile mp4_sp_heaac                { "MPEG4_P2_MP4_SP_HEAAC",              mime::VIDEO_MPEG_4,  labels::VIDEO_CIF30,   Class::Video };
					static Profile mp4_sp_atrac3plus           { "MPEG4_P2_MP4_SP_ATRAC3plus",         mime::VIDEO_MPEG_4,  labels::VIDEO_CIF30,   Class::Video };
					static Profile mp4_sp_aac_ltp              { "MPEG4_P2_MP4_SP_AAC_LTP",            mime::VIDEO_MPEG_4,  labels::VIDEO_CIF30,   Class::Video };
					static Profile mp4_sp_l2_aac               { "MPEG4_P2_MP4_SP_L2_AAC",             mime::VIDEO_MPEG_4,  labels::VIDEO_CIF15,   Class::Video };
					static Profile mp4_sp_l2_amr               { "MPEG4_P2_MP4_SP_L2_AMR",             mime::VIDEO_MPEG_4,  labels::VIDEO_CIF15,   Class::Video };
					static Profile mp4_sp_vga_aac              { "MPEG4_P2_MP4_SP_VGA_AAC",            mime::VIDEO_MPEG_4,  labels::VIDEO_SD,         Class::Video };
					static Profile mp4_sp_vga_heaac            { "MPEG4_P2_MP4_SP_VGA_HEAAC",          mime::VIDEO_MPEG_4,  labels::VIDEO_SD,         Class::Video };
					static Profile mp4_asp_aac                 { "MPEG4_P2_MP4_ASP_AAC",               mime::VIDEO_MPEG_4,  labels::VIDEO_SD,         Class::Video };
					static Profile mp4_asp_heaac               { "MPEG4_P2_MP4_ASP_HEAAC",             mime::VIDEO_MPEG_4,  labels::VIDEO_SD,         Class::Video };
					static Profile mp4_asp_heaac_mult5         { "MPEG4_P2_MP4_ASP_HEAAC_MULT5",       mime::VIDEO_MPEG_4,  labels::VIDEO_SD,         Class::Video };
					static Profile mp4_asp_actrac3plus         { "MPEG4_P2_MP4_ASP_ATRAC3plus",        mime::VIDEO_MPEG_4,  labels::VIDEO_SD,         Class::Video };
					static Profile mp4_asp_l5_so_aac           { "MPEG4_P2_MP4_ASP_L5_SO_AAC",         mime::VIDEO_MPEG_4,  labels::VIDEO_SD,         Class::Video };
					static Profile mp4_asp_l5_so_heaac         { "MPEG4_P2_MP4_ASP_L5_SO_HEAAC",       mime::VIDEO_MPEG_4,  labels::VIDEO_SD,         Class::Video };
					static Profile mp4_asp_l5_so_heaac_mult5   { "MPEG4_P2_MP4_ASP_L5_SO_HEAAC_MULT5", mime::VIDEO_MPEG_4,  labels::VIDEO_SD,         Class::Video };
					static Profile mp4_asp_l4_so_aac           { "MPEG4_P2_MP4_ASP_L4_SO_AAC",         mime::VIDEO_MPEG_4,  labels::VIDEO_CIF30,   Class::Video };
					static Profile mp4_asp_l4_so_heaac         { "MPEG4_P2_MP4_ASP_L4_SO_HEAAC",       mime::VIDEO_MPEG_4,  labels::VIDEO_CIF30,   Class::Video };
					static Profile mp4_asp_l4_so_heaac_mult5   { "MPEG4_P2_MP4_ASP_L4_SO_HEAAC_MULT5", mime::VIDEO_MPEG_4,  labels::VIDEO_CIF30,   Class::Video };
					static Profile h263_mp4_p0_l10_aac         { "MPEG4_H263_MP4_P0_L10_AAC",          mime::VIDEO_3GP,     labels::VIDEO_QCIF15, Class::Video };
					static Profile h263_mp4_p0_l10_aac_ltp     { "MPEG4_H263_MP4_P0_L10_AAC_LTP",      mime::VIDEO_3GP,     labels::VIDEO_QCIF15, Class::Video };
					static Profile ts_sp_aac                   { "MPEG4_P2_TS_SP_AAC",                 mime::VIDEO_MPEG_TS, labels::VIDEO_CIF30,   Class::Video };
					static Profile ts_sp_aac_t                 { "MPEG4_P2_TS_SP_AAC_T",               mime::VIDEO_MPEG_TS, labels::VIDEO_CIF30,   Class::Video };
					static Profile ts_sp_aac_iso               { "MPEG4_P2_TS_SP_AAC_ISO",             mime::VIDEO_MPEG,    labels::VIDEO_CIF30,   Class::Video };
					static Profile ts_sp_mpeg1_l3              { "MPEG4_P2_TS_SP_MPEG1_L3",            mime::VIDEO_MPEG_TS, labels::VIDEO_CIF30,   Class::Video };
					static Profile ts_sp_mpeg1_l3_t            { "MPEG4_P2_TS_SP_MPEG1_L3_T",          mime::VIDEO_MPEG_TS, labels::VIDEO_CIF30,   Class::Video };
					static Profile ts_sp_mpeg1_l3_iso          { "MPEG4_P2_TS_SP_MPEG1_L3_ISO",        mime::VIDEO_MPEG,    labels::VIDEO_CIF30,   Class::Video };
					static Profile ts_sp_ac3                   { "MPEG4_P2_TS_SP_AC3_L3",              mime::VIDEO_MPEG_TS, labels::VIDEO_CIF30,   Class::Video };
					static Profile ts_sp_ac3_t                 { "MPEG4_P2_TS_SP_AC3_T",               mime::VIDEO_MPEG_TS, labels::VIDEO_CIF30,   Class::Video };
					static Profile ts_sp_ac3_iso               { "MPEG4_P2_TS_SP_AC3_ISO",             mime::VIDEO_MPEG,    labels::VIDEO_CIF30,   Class::Video };
					static Profile ts_sp_mpeg2_l2              { "MPEG4_P2_TS_SP_MPEG2_L2",            mime::VIDEO_MPEG_TS, labels::VIDEO_CIF30,   Class::Video };
					static Profile ts_sp_mpeg2_l2_t            { "MPEG4_P2_TS_SP_MPEG2_L2_T",          mime::VIDEO_MPEG_TS, labels::VIDEO_CIF30,   Class::Video };
					static Profile ts_sp_mpeg2_l2_iso          { "MPEG4_P2_TS_SP_MPEG2_L2_ISO",        mime::VIDEO_MPEG,    labels::VIDEO_CIF30,   Class::Video };
					static Profile ts_asp_aac                  { "MPEG4_P2_TS_ASP_AAC",                mime::VIDEO_MPEG_TS, labels::VIDEO_SD,         Class::Video };
					static Profile ts_asp_aac_t                { "MPEG4_P2_TS_ASP_AAC_T",              mime::VIDEO_MPEG_TS, labels::VIDEO_CIF30,   Class::Video };
					static Profile ts_asp_aac_iso              { "MPEG4_P2_TS_ASP_AAC_ISO",            mime::VIDEO_MPEG,    labels::VIDEO_SD,         Class::Video };
					static Profile ts_asp_mpeg1_l3             { "MPEG4_P2_TS_ASP_MPEG1_L3",           mime::VIDEO_MPEG_TS, labels::VIDEO_SD,         Class::Video };
					static Profile ts_asp_mpeg1_l3_t           { "MPEG4_P2_TS_ASP_MPEG1_L3_T",         mime::VIDEO_MPEG_TS, labels::VIDEO_SD,         Class::Video };
					static Profile ts_asp_mpeg1_l3_iso         { "MPEG4_P2_TS_ASP_MPEG1_L3_ISO",       mime::VIDEO_MPEG,    labels::VIDEO_SD,         Class::Video };
					static Profile ts_asp_ac3                  { "MPEG4_P2_TS_ASP_AC3_L3",             mime::VIDEO_MPEG_TS, labels::VIDEO_SD,         Class::Video };
					static Profile ts_asp_ac3_t                { "MPEG4_P2_TS_ASP_AC3_T",              mime::VIDEO_MPEG_TS, labels::VIDEO_SD,         Class::Video };
					static Profile ts_asp_ac3_iso              { "MPEG4_P2_TS_ASP_AC3_ISO",            mime::VIDEO_MPEG,    labels::VIDEO_SD,         Class::Video };
					static Profile ts_co_ac3                   { "MPEG4_P2_TS_CO_AC3",                 mime::VIDEO_MPEG_TS, labels::VIDEO_CIF30,   Class::Video };
					static Profile ts_co_ac3_t                 { "MPEG4_P2_TS_CO_AC3_T",               mime::VIDEO_MPEG_TS, labels::VIDEO_CIF30,   Class::Video };
					static Profile ts_co_ac3_iso               { "MPEG4_P2_TS_CO_AC3_ISO",             mime::VIDEO_MPEG,    labels::VIDEO_CIF30,   Class::Video };
					static Profile ts_co_mpeg2_l2              { "MPEG4_P2_TS_CO_MPEG2_L2",            mime::VIDEO_MPEG_TS, labels::VIDEO_CIF30,   Class::Video };
					static Profile ts_co_mpeg2_l2_t            { "MPEG4_P2_TS_CO_MPEG2_L2_T",          mime::VIDEO_MPEG_TS, labels::VIDEO_CIF30,   Class::Video };
					static Profile ts_co_mpeg2_l2_iso          { "MPEG4_P2_TS_CO_MPEG2_L2_ISO",        mime::VIDEO_MPEG,    labels::VIDEO_CIF30,   Class::Video };
					static Profile asf_sp_g726                 { "MPEG4_P2_ASF_SP_G726",               mime::VIDEO_ASF,     labels::VIDEO_CIF30,   Class::Video };
					static Profile asf_asp_l5_so_g726          { "MPEG4_P2_ASF_ASP_L5_SO_G726",        mime::VIDEO_ASF,     labels::VIDEO_SD,         Class::Video };
					static Profile asf_asp_l4_so_g726          { "MPEG4_P2_ASF_ASP_L4_SO_G726",        mime::VIDEO_ASF,     labels::VIDEO_CIF30,   Class::Video };
					static Profile h263_3gpp_p0_l10_amr_wbplus { "MPEG4_H263_3GPP_P0_L10_AMR_WBplus",  mime::VIDEO_3GP,     labels::VIDEO_QCIF15, Class::Video };
					static Profile _3gpp_sp_l0b_aac            { "MPEG4_P2_3GPP_SP_L0B_AAC",           mime::VIDEO_3GP,     labels::VIDEO_QCIF15, Class::Video };
					static Profile _3gpp_sp_l0b_amr            { "MPEG4_P2_3GPP_SP_L0B_AMR",           mime::VIDEO_3GP,     labels::VIDEO_QCIF15, Class::Video };
					static Profile h263_3gpp_p3_l10_amr        { "MPEG4_H263_3GPP_P3_L10_AMR",         mime::VIDEO_3GP,     labels::VIDEO_QCIF15, Class::Video };

					enum class codecodec
					{
						INVALID,
						H263,
						P2,
					};

					struct codecodec_mapping
					{
						int codec_id;
						codecodec type;
					};
					static const codecodec_mapping codecodec_mappings [] =
					{
						{ AV_CODEC_ID_H263,      codecodec::H263 },
						{ AV_CODEC_ID_H263I,     codecodec::H263 },
						{ AV_CODEC_ID_H263P,     codecodec::H263 },
						{ AV_CODEC_ID_MPEG4,     codecodec::P2 },
						{ AV_CODEC_ID_MSMPEG4V1, codecodec::P2 },
						{ AV_CODEC_ID_MSMPEG4V2, codecodec::P2 },
						{ AV_CODEC_ID_MSMPEG4V3, codecodec::P2 }
					};

					static codecodec get_codecodec(AVCodecContext *codec)
					{
						if (!codec)
							return codecodec::INVALID;

						for (auto&& map: codecodec_mappings)
							if (codec->codec_id == map.codec_id)
								return map.type;

						return codecodec::INVALID;
					}

					enum class profile
					{
						INVALID,
						H263,
						P2_SP_L0B,
						P2_SP_L2,
						P2_SP_L3,
						P2_SP_L3_VGA,
						P2_ASP_L4,
						P2_ASP_L5
					};

					struct video_properties {
						int width;
						int height;
						int fps_num;
						int fps_den;
					};

					/* H263 Resolutions (all <= 15 fps) */
					static video_properties profile_h263_res [] =
					{
						{ 176, 144, 15, 1 }, /* QCIF */
						{ 128,  96, 15, 1 }  /* SQCIF */
					};

					/* MPEG-4 SP L0B Resolutions (all <= 15 fps) */
					static video_properties profile_p2_sp_l0b_res [] =
					{
						{ 176, 144, 15, 1 }, /* QCIF */
						{ 128,  96, 15, 1 }  /* SQCIF */
					};

					/* MPEG-4 SP L2 Resolutions */
					static video_properties profile_p2_sp_l2_res [] =
					{
						{ 352, 288, 15, 1 }, /* CIF */
						{ 320, 240, 15, 1 }, /* QVGA 4:3 */
						{ 320, 180, 15, 1 }, /* QVGA 16:9 */
						{ 176, 144, 30, 1 }, /* QCIF */
						{ 128,  96, 30, 1 }  /* SQCIF */
					};

					/* MPEG-4 SP L3_VGA Resolutions */
					static video_properties profile_p2_sp_l3_vga_res [] =
					{
						{ 640, 480, 30, 1 }, /* VGA */
						{ 640, 360, 30, 1 }  /* VGA 16:9 */
					};

					/* MPEG-4 L3 / CO Resolutions (all <= 30 fps) */
					static video_properties profile_p2_sp_l3_co_res [] =
					{
						{ 352, 288, 30, 1 }, /* CIF, 625SIF */
						{ 352, 240, 30, 1 }, /* 525SIF */
						{ 320, 240, 30, 1 }, /* QVGA 4:3 */
						{ 320, 180, 30, 1 }, /* QVGA 16:9 */
						{ 240, 180, 30, 1 }, /* 1/7 VGA 4:3 */
						{ 208, 160, 30, 1 }, /* 1/9 VGA 4:3 */
						{ 176, 144, 30, 1 }, /* QCIF,625QCIF */
						{ 176, 120, 30, 1 }, /* 525QCIF */
						{ 160, 120, 30, 1 }, /* SQVGA 4:3 */
						{ 160, 112, 30, 1 }, /* 1/16 VGA 4:3 */
						{ 160,  90, 30, 1 }, /* SQVGA 16:9 */
						{ 128,  96, 30, 1 }  /* SQCIF */
					};

					/* MPEG-4 ASP L4 SO Resolutions (all <= 30 fps) */
					static video_properties profile_p2_asp_l4_res [] =
					{
						{ 352, 576, 30, 1 }, /* 625 1/2 D1 */
						{ 352, 480, 30, 1 }, /* 525 1/2 D1 */
						{ 352, 288, 30, 1 }, /* CIF, 625SIF */
						{ 352, 240, 30, 1 }, /* 525SIF */
						{ 320, 240, 30, 1 }, /* QVGA 4:3 */
						{ 320, 180, 30, 1 }, /* QVGA 16:9 */
						{ 240, 180, 30, 1 }, /* 1/7 VGA 4:3 */
						{ 208, 160, 30, 1 }, /* 1/9 VGA 4:3 */
						{ 176, 144, 30, 1 }, /* QCIF,625QCIF */
						{ 176, 120, 30, 1 }, /* 525QCIF */
						{ 160, 120, 30, 1 }, /* SQVGA 4:3 */
						{ 160, 112, 30, 1 }, /* 1/16 VGA 4:3 */
						{ 160,  90, 30, 1 }, /* SQVGA 16:9 */
						{ 128,  96, 30, 1 }  /* SQCIF */
					};

					/* MPEG-4 ASP L5 Resolutions (all <= 30 fps) */
					static video_properties profile_p2_asp_l5_res [] =
					{
						{ 720, 576, 30, 1 }, /* 625 D1 */
						{ 720, 480, 30, 1 }, /* 525 D1 */
						{ 704, 576, 30, 1 }, /* 625 4SIF */
						{ 704, 480, 30, 1 }, /* 525 4SIF */
						{ 640, 480, 30, 1 }, /* VGA */
						{ 640, 360, 30, 1 }, /* VGA 16:9 */
						{ 544, 576, 30, 1 }, /* 625 3/4 D1 */
						{ 544, 480, 30, 1 }, /* 525 3/4 D1 */
						{ 480, 576, 30, 1 }, /* 625 2/3 D1 */
						{ 480, 480, 30, 1 }, /* 525 2/3 D1 */
						{ 480, 360, 30, 1 }, /* 9/16 VGA 4:3 */
						{ 480, 270, 30, 1 }, /* 9/16 VGA 16:9 */
						{ 352, 576, 30, 1 }, /* 625 1/2 D1 */
						{ 352, 480, 30, 1 }, /* 525 1/2 D1 */
						{ 352, 288, 30, 1 }, /* CIF, 625SIF */
						{ 352, 240, 30, 1 }, /* 525SIF */
						{ 320, 240, 30, 1 }, /* QVGA 4:3 */
						{ 320, 180, 30, 1 }, /* QVGA 16:9 */
						{ 240, 180, 30, 1 }, /* 1/7 VGA 4:3 */
						{ 208, 160, 30, 1 }, /* 1/9 VGA 4:3 */
						{ 176, 144, 30, 1 }, /* QCIF,625QCIF */
						{ 176, 120, 30, 1 }, /* 525QCIF */
						{ 160, 120, 30, 1 }, /* SQVGA 4:3 */
						{ 160, 112, 30, 1 }, /* 1/16 VGA 4:3 */
						{ 160,  90, 30, 1 }, /* SQVGA 16:9 */
						{ 128,  96, 30, 1 }  /* SQCIF */
					};

					struct mpeg4_profiles
					{
						const Profile *profile;
						container::container_type container;
						part2::profile video_profile;
						audio::profile audio_profile;
					};

					static const mpeg4_profiles mpeg4_profiles_mapping [] =
					{
						/* MPEG-4 Container */
						{ &mp4_sp_aac, container::MP4, profile::P2_SP_L3, audio::profile::AAC },
						{ &mp4_sp_heaac, container::MP4, profile::P2_SP_L3, audio::profile::AAC_HE_L2 },
						{ &mp4_sp_atrac3plus, container::MP4, profile::P2_SP_L3, audio::profile::ATRAC },
						{ &mp4_sp_aac_ltp, container::MP4, profile::P2_SP_L3, audio::profile::AAC_LTP },

						{ &mp4_sp_l2_aac, container::MP4, profile::P2_SP_L2, audio::profile::AAC },
						{ &mp4_sp_l2_amr, container::MP4, profile::P2_SP_L2, audio::profile::AMR },

						{ &mp4_sp_vga_aac, container::MP4, profile::P2_SP_L3_VGA, audio::profile::AAC },
						{ &mp4_sp_vga_heaac, container::MP4, profile::P2_SP_L3_VGA, audio::profile::AAC_HE_L2 },

						{ &mp4_asp_aac, container::MP4, profile::P2_ASP_L5, audio::profile::AAC },
						{ &mp4_asp_heaac, container::MP4, profile::P2_ASP_L5, audio::profile::AAC_HE_L2 },
						{ &mp4_asp_heaac_mult5, container::MP4, profile::P2_ASP_L5, audio::profile::AAC_HE_MULT5 },
						{ &mp4_asp_actrac3plus, container::MP4, profile::P2_ASP_L5, audio::profile::ATRAC },

						{ &mp4_asp_l5_so_aac, container::MP4, profile::P2_ASP_L5, audio::profile::AAC },
						{ &mp4_asp_l5_so_heaac, container::MP4, profile::P2_ASP_L5, audio::profile::AAC_HE_L2 },
						{ &mp4_asp_l5_so_heaac_mult5, container::MP4, profile::P2_ASP_L5, audio::profile::AAC_HE_MULT5 },

						{ &mp4_asp_l4_so_aac, container::MP4, profile::P2_ASP_L4, audio::profile::AAC },
						{ &mp4_asp_l4_so_heaac, container::MP4, profile::P2_ASP_L4, audio::profile::AAC_HE_L2 },
						{ &mp4_asp_l4_so_heaac_mult5, container::MP4, profile::P2_ASP_L4, audio::profile::AAC_HE_MULT5 },

						{ &h263_mp4_p0_l10_aac, container::MP4, profile::H263, audio::profile::AAC },
						{ &h263_mp4_p0_l10_aac_ltp, container::MP4, profile::H263, audio::profile::AAC_LTP },

						/* MPEG-TS Container */
						{ &ts_sp_aac, container::MPEG_TRANSPORT_STREAM_DLNA_NO_TS, profile::P2_SP_L3, audio::profile::AAC },
						{ &ts_sp_aac_t, container::MPEG_TRANSPORT_STREAM_DLNA, profile::P2_SP_L3, audio::profile::AAC },
						{ &ts_sp_aac_iso, container::MPEG_TRANSPORT_STREAM, profile::P2_SP_L3, audio::profile::AAC },

						{ &ts_sp_mpeg1_l3, container::MPEG_TRANSPORT_STREAM_DLNA_NO_TS, profile::P2_SP_L3, audio::profile::MP3 },
						{ &ts_sp_mpeg1_l3_t, container::MPEG_TRANSPORT_STREAM_DLNA, profile::P2_SP_L3, audio::profile::MP3 },
						{ &ts_sp_mpeg1_l3_iso, container::MPEG_TRANSPORT_STREAM, profile::P2_SP_L3, audio::profile::MP3 },

						{ &ts_sp_ac3, container::MPEG_TRANSPORT_STREAM_DLNA_NO_TS, profile::P2_SP_L3, audio::profile::AC3 },
						{ &ts_sp_ac3_t, container::MPEG_TRANSPORT_STREAM_DLNA, profile::P2_SP_L3, audio::profile::AC3 },
						{ &ts_sp_ac3_iso, container::MPEG_TRANSPORT_STREAM, profile::P2_SP_L3, audio::profile::AC3 },

						{ &ts_sp_mpeg2_l2, container::MPEG_TRANSPORT_STREAM_DLNA_NO_TS, profile::P2_SP_L3, audio::profile::MP2 },
						{ &ts_sp_mpeg2_l2_t, container::MPEG_TRANSPORT_STREAM_DLNA, profile::P2_SP_L3, audio::profile::MP2 },
						{ &ts_sp_mpeg2_l2_iso, container::MPEG_TRANSPORT_STREAM, profile::P2_SP_L3, audio::profile::MP2 },

						{ &ts_asp_aac, container::MPEG_TRANSPORT_STREAM_DLNA_NO_TS, profile::P2_ASP_L5, audio::profile::AAC },
						{ &ts_asp_aac_t, container::MPEG_TRANSPORT_STREAM_DLNA, profile::P2_ASP_L5, audio::profile::AAC },
						{ &ts_asp_aac_iso, container::MPEG_TRANSPORT_STREAM, profile::P2_ASP_L5, audio::profile::AAC },

						{ &ts_asp_mpeg1_l3, container::MPEG_TRANSPORT_STREAM_DLNA_NO_TS, profile::P2_ASP_L5, audio::profile::MP3 },
						{ &ts_asp_mpeg1_l3_t, container::MPEG_TRANSPORT_STREAM_DLNA, profile::P2_ASP_L5, audio::profile::MP3 },
						{ &ts_asp_mpeg1_l3_iso, container::MPEG_TRANSPORT_STREAM, profile::P2_ASP_L5, audio::profile::MP3 },

						{ &ts_asp_ac3, container::MPEG_TRANSPORT_STREAM_DLNA_NO_TS, profile::P2_ASP_L5, audio::profile::AC3 },
						{ &ts_asp_ac3_t, container::MPEG_TRANSPORT_STREAM_DLNA, profile::P2_ASP_L5, audio::profile::AC3 },
						{ &ts_asp_ac3_iso, container::MPEG_TRANSPORT_STREAM, profile::P2_ASP_L5, audio::profile::AC3 },

						{ &ts_co_ac3, container::MPEG_TRANSPORT_STREAM_DLNA_NO_TS, profile::P2_ASP_L4, audio::profile::AC3 },
						{ &ts_co_ac3_t, container::MPEG_TRANSPORT_STREAM_DLNA, profile::P2_ASP_L4, audio::profile::AC3 },
						{ &ts_co_ac3_iso, container::MPEG_TRANSPORT_STREAM, profile::P2_ASP_L4, audio::profile::AC3 },

						{ &ts_co_mpeg2_l2, container::MPEG_TRANSPORT_STREAM_DLNA_NO_TS, profile::P2_ASP_L4, audio::profile::MP2 },
						{ &ts_co_mpeg2_l2_t, container::MPEG_TRANSPORT_STREAM_DLNA, profile::P2_ASP_L4, audio::profile::MP2 },
						{ &ts_co_mpeg2_l2_iso, container::MPEG_TRANSPORT_STREAM, profile::P2_ASP_L4, audio::profile::MP2 },

						/* ASF Container */
						{ &asf_sp_g726, container::ASF, profile::P2_SP_L3, audio::profile::G726 },
						{ &asf_asp_l5_so_g726, container::ASF, profile::P2_ASP_L5, audio::profile::G726 },
						{ &asf_asp_l4_so_g726, container::ASF, profile::P2_ASP_L4, audio::profile::G726 },

						/* 3GPP Container */
						{ &h263_3gpp_p0_l10_amr_wbplus, container::_3GP, profile::H263, audio::profile::AMR_WB },
						{ &_3gpp_sp_l0b_aac, container::_3GP, profile::P2_SP_L0B, audio::profile::AAC },
						{ &_3gpp_sp_l0b_amr, container::_3GP, profile::P2_SP_L0B, audio::profile::AMR },
						{ &h263_3gpp_p3_l10_amr, container::_3GP, profile::H263, audio::profile::AMR }
					};

					template <size_t size>
					static inline bool is_valid_video_profile(const video_properties (&props) [size], AVStream *stream, AVCodecContext *codec)
					{
						for (auto&& prop : props)
						{
							if (prop.width != codec->width) continue;
							if (prop.height != codec->height) continue;
							if ((stream->r_frame_rate.num * prop.fps_den) > (prop.fps_num * stream->r_frame_rate.den)) continue;
							return true;

						}

						return false;
					}
#define CHECK_VIDEO_PROFILE(props, profile_id) if (is_valid_video_profile(props, stream, codec)) return profile_id

					static profile get_profile(codecodec codectype, AVStream *stream, AVCodecContext *codec)
					{
						if (!stream || !codec)
							return profile::INVALID;

						if (codectype == codecodec::H263)
						{
							if (codec->bit_rate <= 64000) /* max bitrate is 64 kbps */
							{
								CHECK_VIDEO_PROFILE(profile_h263_res, profile::H263);
							}
						}
						else if (codectype == codecodec::P2)
						{
							if (codec->bit_rate <= 128000) /* SP_L2 and SP_L0B */
							{
								CHECK_VIDEO_PROFILE(profile_p2_sp_l0b_res, profile::P2_SP_L0B);
								CHECK_VIDEO_PROFILE(profile_p2_sp_l2_res, profile::P2_SP_L2);
							}
							else if (codec->bit_rate <= 384000) /* SP_L3 */
							{
								CHECK_VIDEO_PROFILE(profile_p2_sp_l3_co_res, profile::P2_SP_L3);
							}
							else if (codec->bit_rate <= 2000000) /* CO and ASP_L4 */
							{
								CHECK_VIDEO_PROFILE(profile_p2_sp_l3_co_res, profile::P2_ASP_L4);
								CHECK_VIDEO_PROFILE(profile_p2_asp_l4_res, profile::P2_ASP_L4);
							}
							else if (codec->bit_rate <= 3000000) /* SP_L3_VGA */
							{
								CHECK_VIDEO_PROFILE(profile_p2_sp_l3_vga_res, profile::P2_SP_L3_VGA);
							}
							else if (codec->bit_rate <= 8000000) /* ASP_L5 */
							{
								CHECK_VIDEO_PROFILE(profile_p2_asp_l5_res, profile::P2_ASP_L5);
							}
						}

						return profile::INVALID;
					}
#undef CHECK_VIDEO_PROFILE

					static const Profile* probe(AVFormatContext *ctx, container::container_type container, const stream_codec& codecs)
					{
						if (!stream_is_video(ctx, container, codecs))
							return nullptr;

						auto codectype = get_codecodec(codecs.m_video.m_codec);
						if (codectype == codecodec::INVALID)
							return nullptr;

						if (container != container::ASF &&
							container != container::_3GP &&
							container != container::MP4 &&
							container != container::MPEG_TRANSPORT_STREAM &&
							container != container::MPEG_TRANSPORT_STREAM_DLNA &&
							container != container::MPEG_TRANSPORT_STREAM_DLNA_NO_TS)
							return nullptr;

						/* ensure we have a valid video codec bit rate */
						if (codecs.m_video.m_codec->bit_rate == 0)
						{
							codecs.m_video.m_codec->bit_rate = 
								codecs.m_audio.m_codec->bit_rate ? ctx->bit_rate - codecs.m_audio.m_codec->bit_rate : ctx->bit_rate;
						}

						/* check for valid video profile */
						auto video_profile = get_profile(codectype, codecs.m_video.m_stream, codecs.m_video.m_codec);
						if (video_profile == part2::profile::INVALID)
							return nullptr;

						/* check for valid audio profile */
						auto audio_profile = audio::guess_profile(codecs.m_audio.m_codec);
						if (audio_profile == audio::profile::INVALID)
							return nullptr;

						/* AAC fixup: _320 profiles are audio-only profiles */
						if (audio_profile == audio::profile::AAC_320)
							audio_profile = audio::profile::AAC;
						if (audio_profile == audio::profile::AAC_HE_L2_320)
							audio_profile = audio::profile::AAC_HE_L2;

						/* find profile according to container type, video and audio profiles */
						for (auto && map : mpeg4_profiles_mapping)
						{
							if (map.container != container) continue;
							if (map.video_profile != video_profile) continue;
							if (map.audio_profile != audio_profile) continue;
							return map.profile;
						}

						return nullptr;
					}
				}

				namespace part10
				{
					static Profile avc_mp4_mp_sd_aac_mult5           { "AVC_MP4_MP_SD_AAC_MULT5",           mime::VIDEO_MPEG_4,  labels::VIDEO_SD,     Class::Video };
					static Profile avc_mp4_mp_sd_heaac_l2            { "AVC_MP4_MP_SD_HEAAC_L2",            mime::VIDEO_MPEG_4,  labels::VIDEO_SD,     Class::Video };
					static Profile avc_mp4_mp_sd_mpeg1_l3            { "AVC_MP4_MP_SD_MPEG1_L3",            mime::VIDEO_MPEG_4,  labels::VIDEO_SD,     Class::Video };
					static Profile avc_mp4_mp_sd_ac3                 { "AVC_MP4_MP_SD_AC3",                 mime::VIDEO_MPEG_4,  labels::VIDEO_SD,     Class::Video };
					static Profile avc_mp4_mp_sd_aac_ltp             { "AVC_MP4_MP_SD_AAC_LTP",             mime::VIDEO_MPEG_4,  labels::VIDEO_SD,     Class::Video };
					static Profile avc_mp4_mp_sd_aac_ltp_mult5       { "AVC_MP4_MP_SD_AAC_LTP_MULT5",       mime::VIDEO_MPEG_4,  labels::VIDEO_SD,     Class::Video };
					static Profile avc_mp4_mp_sd_aac_ltp_mult7       { "AVC_MP4_MP_SD_AAC_LTP_MULT7",       mime::VIDEO_MPEG_4,  labels::VIDEO_SD,     Class::Video };
					static Profile avc_mp4_mp_sd_atrac3plus          { "AVC_MP4_MP_SD_ATRAC3plus",          mime::VIDEO_MPEG_4,  labels::VIDEO_SD,     Class::Video };
					static Profile avc_mp4_bl_l3l_sd_aac             { "AVC_MP4_BL_L3L_SD_AAC",             mime::VIDEO_MPEG_4,  labels::VIDEO_SD,     Class::Video };
					static Profile avc_mp4_bl_l3l_sd_heaac           { "AVC_MP4_BL_L3L_SD_HEAAC",           mime::VIDEO_MPEG_4,  labels::VIDEO_SD,     Class::Video };
					static Profile avc_mp4_bl_l3_sd_aac              { "AVC_MP4_BL_L3_SD_AAC",              mime::VIDEO_MPEG_4,  labels::VIDEO_SD,     Class::Video };
					static Profile avc_mp4_mp_sd_bsac                { "AVC_MP4_MP_SD_BSAC",                mime::VIDEO_MPEG_4,  labels::VIDEO_SD,     Class::Video };
					static Profile avc_mp4_bl_cif30_aac_mult5        { "AVC_MP4_BL_CIF30_AAC_MULT5",        mime::VIDEO_MPEG_4,  labels::VIDEO_CIF30,  Class::Video };
					static Profile avc_mp4_bl_cif30_heaac_l2         { "AVC_MP4_BL_CIF30_HEAAC_L2",         mime::VIDEO_MPEG_4,  labels::VIDEO_CIF30,  Class::Video };
					static Profile avc_mp4_bl_cif30_mpeg1_l3         { "AVC_MP4_BL_CIF30_MPEG1_L3",         mime::VIDEO_MPEG_4,  labels::VIDEO_CIF30,  Class::Video };
					static Profile avc_mp4_bl_cif30_ac3              { "AVC_MP4_BL_CIF30_AC3",              mime::VIDEO_MPEG_4,  labels::VIDEO_CIF30,  Class::Video };
					static Profile avc_mp4_bl_cif30_aac_ltp          { "AVC_MP4_BL_CIF30_AAC_LTP",          mime::VIDEO_MPEG_4,  labels::VIDEO_CIF30,  Class::Video };
					static Profile avc_mp4_bl_cif30_aac_ltp_mult5    { "AVC_MP4_BL_CIF30_AAC_LTP_MULT5",    mime::VIDEO_MPEG_4,  labels::VIDEO_CIF30,  Class::Video };
					static Profile avc_mp4_bl_l2_cif30_aac           { "AVC_MP4_BL_L2_CIF30_AAC",           mime::VIDEO_MPEG_4,  labels::VIDEO_CIF30,  Class::Video };
					static Profile avc_mp4_bl_cif30_bsac             { "AVC_MP4_BL_CIF30_BSAC",             mime::VIDEO_MPEG_4,  labels::VIDEO_CIF30,  Class::Video };
					static Profile avc_mp4_bl_cif30_bsac_mult5       { "AVC_MP4_BL_CIF30_BSAC_MULT5",       mime::VIDEO_MPEG_4,  labels::VIDEO_CIF30,  Class::Video };
					static Profile avc_mp4_bl_cif15_heaac            { "AVC_MP4_BL_CIF15_HEAAC",            mime::VIDEO_MPEG_4,  labels::VIDEO_CIF15,  Class::Video };
					static Profile avc_mp4_bl_cif15_amr              { "AVC_MP4_BL_CIF15_AMR",              mime::VIDEO_MPEG_4,  labels::VIDEO_CIF15,  Class::Video };
					static Profile avc_mp4_bl_cif15_aac              { "AVC_MP4_BL_CIF15_AAC",              mime::VIDEO_MPEG_4,  labels::VIDEO_CIF15,  Class::Video };
					static Profile avc_mp4_bl_cif15_aac_520          { "AVC_MP4_BL_CIF15_AAC_520",          mime::VIDEO_MPEG_4,  labels::VIDEO_CIF15,  Class::Video };
					static Profile avc_mp4_bl_cif15_aac_ltp          { "AVC_MP4_BL_CIF15_AAC_LTP",          mime::VIDEO_MPEG_4,  labels::VIDEO_CIF15,  Class::Video };
					static Profile avc_mp4_bl_cif15_aac_ltp_520      { "AVC_MP4_BL_CIF15_AAC_LTP_520",      mime::VIDEO_MPEG_4,  labels::VIDEO_CIF15,  Class::Video };
					static Profile avc_mp4_bl_cif15_bsac             { "AVC_MP4_BL_CIF15_BSAC",             mime::VIDEO_MPEG_4,  labels::VIDEO_CIF15,  Class::Video };
					static Profile avc_mp4_bl_l12_cif15_heaac        { "AVC_MP4_BL_L12_CIF15_HEAAC",        mime::VIDEO_MPEG_4,  labels::VIDEO_CIF15,  Class::Video };
					static Profile avc_mp4_bl_l1b_qcif15_heaac       { "AVC_MP4_BL_L1B_QCIF15_HEAAC",       mime::VIDEO_MPEG_4,  labels::VIDEO_QCIF15, Class::Video };
					static Profile avc_ts_mp_sd_aac_mult5            { "AVC_TS_MP_SD_AAC_MULT5",            mime::VIDEO_MPEG_TS, labels::VIDEO_SD,     Class::Video };
					static Profile avc_ts_mp_sd_aac_mult5_t          { "AVC_TS_MP_SD_AAC_MULT5_T",          mime::VIDEO_MPEG_TS, labels::VIDEO_SD,     Class::Video };
					static Profile avc_ts_mp_sd_aac_mult5_iso        { "AVC_TS_MP_SD_AAC_MULT5_ISO",        mime::VIDEO_MPEG,    labels::VIDEO_SD,     Class::Video };
					static Profile avc_ts_mp_sd_heaac_l2             { "AVC_TS_MP_SD_HEAAC_L2",             mime::VIDEO_MPEG_TS, labels::VIDEO_SD,     Class::Video };
					static Profile avc_ts_mp_sd_heaac_l2_t           { "AVC_TS_MP_SD_HEAAC_L2_T",           mime::VIDEO_MPEG_TS, labels::VIDEO_SD,     Class::Video };
					static Profile avc_ts_mp_sd_heaac_l2_iso         { "AVC_TS_MP_SD_HEAAC_L2_ISO",         mime::VIDEO_MPEG,    labels::VIDEO_SD,     Class::Video };
					static Profile avc_ts_mp_sd_mpeg1_l3             { "AVC_TS_MP_SD_MPEG1_L3",             mime::VIDEO_MPEG_TS, labels::VIDEO_SD,     Class::Video };
					static Profile avc_ts_mp_sd_mpeg1_l3_t           { "AVC_TS_MP_SD_MPEG1_L3_T",           mime::VIDEO_MPEG_TS, labels::VIDEO_SD,     Class::Video };
					static Profile avc_ts_mp_sd_mpeg1_l3_iso         { "AVC_TS_MP_SD_MPEG1_L3_ISO",         mime::VIDEO_MPEG,    labels::VIDEO_SD,     Class::Video };
					static Profile avc_ts_mp_sd_ac3                  { "AVC_TS_MP_SD_AC3",                  mime::VIDEO_MPEG_TS, labels::VIDEO_SD,     Class::Video };
					static Profile avc_ts_mp_sd_ac3_t                { "AVC_TS_MP_SD_AC3_T",                mime::VIDEO_MPEG_TS, labels::VIDEO_SD,     Class::Video };
					static Profile avc_ts_mp_sd_ac3_iso              { "AVC_TS_MP_SD_AC3_ISO",              mime::VIDEO_MPEG,    labels::VIDEO_SD,     Class::Video };
					static Profile avc_ts_mp_sd_aac_ltp              { "AVC_TS_MP_SD_AAC_LTP",              mime::VIDEO_MPEG_TS, labels::VIDEO_SD,     Class::Video };
					static Profile avc_ts_mp_sd_aac_ltp_t            { "AVC_TS_MP_SD_AAC_LTP_T",            mime::VIDEO_MPEG_TS, labels::VIDEO_SD,     Class::Video };
					static Profile avc_ts_mp_sd_aac_ltp_iso          { "AVC_TS_MP_SD_AAC_LTP_ISO",          mime::VIDEO_MPEG,    labels::VIDEO_SD,     Class::Video };
					static Profile avc_ts_mp_sd_aac_ltp_mult5        { "AVC_TS_MP_SD_AAC_LTP_MULT5",        mime::VIDEO_MPEG_TS, labels::VIDEO_SD,     Class::Video };
					static Profile avc_ts_mp_sd_aac_ltp_mult5_t      { "AVC_TS_MP_SD_AAC_LTP_MULT5_T",      mime::VIDEO_MPEG_TS, labels::VIDEO_SD,     Class::Video };
					static Profile avc_ts_mp_sd_aac_ltp_mult5_iso    { "AVC_TS_MP_SD_AAC_LTP_MULT5_ISO",    mime::VIDEO_MPEG,    labels::VIDEO_SD,     Class::Video };
					static Profile avc_ts_mp_sd_aac_ltp_mult7        { "AVC_TS_MP_SD_AAC_LTP_MULT7",        mime::VIDEO_MPEG_TS, labels::VIDEO_SD,     Class::Video };
					static Profile avc_ts_mp_sd_aac_ltp_mult7_t      { "AVC_TS_MP_SD_AAC_LTP_MULT7_T",      mime::VIDEO_MPEG_TS, labels::VIDEO_SD,     Class::Video };
					static Profile avc_ts_mp_sd_aac_ltp_mult7_iso    { "AVC_TS_MP_SD_AAC_LTP_MULT7_ISO",    mime::VIDEO_MPEG,    labels::VIDEO_SD,     Class::Video };
					static Profile avc_ts_mp_sd_bsac                 { "AVC_TS_MP_SD_BSAC",                 mime::VIDEO_MPEG_TS, labels::VIDEO_SD,     Class::Video };
					static Profile avc_ts_mp_sd_bsac_t               { "AVC_TS_MP_SD_BSAC_T",               mime::VIDEO_MPEG_TS, labels::VIDEO_SD,     Class::Video };
					static Profile avc_ts_mp_sd_bsac_iso             { "AVC_TS_MP_SD_BSAC_ISO",             mime::VIDEO_MPEG,    labels::VIDEO_SD,     Class::Video };
					static Profile avc_ts_bl_cif30_aac_mult5         { "AVC_TS_BL_CIF30_AAC_MULT5",         mime::VIDEO_MPEG_TS, labels::VIDEO_CIF30,  Class::Video };
					static Profile avc_ts_bl_cif30_aac_mult5_t       { "AVC_TS_BL_CIF30_AAC_MULT5_T",       mime::VIDEO_MPEG_TS, labels::VIDEO_CIF30,  Class::Video };
					static Profile avc_ts_bl_cif30_aac_mult5_iso     { "AVC_TS_BL_CIF30_AAC_MULT5_ISO",     mime::VIDEO_MPEG,    labels::VIDEO_CIF30,  Class::Video };
					static Profile avc_ts_bl_cif30_heaac_l2          { "AVC_TS_BL_CIF30_HEAAC_L2",          mime::VIDEO_MPEG_TS, labels::VIDEO_CIF30,  Class::Video };
					static Profile avc_ts_bl_cif30_heaac_l2_t        { "AVC_TS_BL_CIF30_HEAAC_L2_T",        mime::VIDEO_MPEG_TS, labels::VIDEO_CIF30,  Class::Video };
					static Profile avc_ts_bl_cif30_heaac_l2_iso      { "AVC_TS_BL_CIF30_HEAAC_L2_ISO",      mime::VIDEO_MPEG,    labels::VIDEO_CIF30,  Class::Video };
					static Profile avc_ts_bl_cif30_mpeg1_l3          { "AVC_TS_BL_CIF30_MPEG1_L3",          mime::VIDEO_MPEG_TS, labels::VIDEO_CIF30,  Class::Video };
					static Profile avc_ts_bl_cif30_mpeg1_l3_t        { "AVC_TS_BL_CIF30_MPEG1_L3_T",        mime::VIDEO_MPEG_TS, labels::VIDEO_CIF30,  Class::Video };
					static Profile avc_ts_bl_cif30_mpeg1_l3_iso      { "AVC_TS_BL_CIF30_MPEG1_L3_ISO",      mime::VIDEO_MPEG,    labels::VIDEO_CIF30,  Class::Video };
					static Profile avc_ts_bl_cif30_ac3               { "AVC_TS_BL_CIF30_AC3",               mime::VIDEO_MPEG_TS, labels::VIDEO_CIF30,  Class::Video };
					static Profile avc_ts_bl_cif30_ac3_t             { "AVC_TS_BL_CIF30_AC3_T",             mime::VIDEO_MPEG_TS, labels::VIDEO_CIF30,  Class::Video };
					static Profile avc_ts_bl_cif30_ac3_iso           { "AVC_TS_BL_CIF30_AC3_ISO",           mime::VIDEO_MPEG,    labels::VIDEO_CIF30,  Class::Video };
					static Profile avc_ts_bl_cif30_aac_ltp           { "AVC_TS_BL_CIF30_AAC_LTP",           mime::VIDEO_MPEG_TS, labels::VIDEO_CIF30,  Class::Video };
					static Profile avc_ts_bl_cif30_aac_ltp_t         { "AVC_TS_BL_CIF30_AAC_LTP_T",         mime::VIDEO_MPEG_TS, labels::VIDEO_CIF30,  Class::Video };
					static Profile avc_ts_bl_cif30_aac_ltp_iso       { "AVC_TS_BL_CIF30_AAC_LTP_ISO",       mime::VIDEO_MPEG,    labels::VIDEO_CIF30,  Class::Video };
					static Profile avc_ts_bl_cif30_aac_ltp_mult5     { "AVC_TS_BL_CIF30_AAC_LTP_MULT5",     mime::VIDEO_MPEG_TS, labels::VIDEO_CIF30,  Class::Video };
					static Profile avc_ts_bl_cif30_aac_ltp_mult5_t   { "AVC_TS_BL_CIF30_AAC_LTP_MULT5_T",   mime::VIDEO_MPEG_TS, labels::VIDEO_CIF30,  Class::Video };
					static Profile avc_ts_bl_cif30_aac_ltp_mult5_iso { "AVC_TS_BL_CIF30_AAC_LTP_MULT5_ISO", mime::VIDEO_MPEG,    labels::VIDEO_CIF30,  Class::Video };
					static Profile avc_ts_bl_cif30_aac_940           { "AVC_TS_BL_CIF30_AAC_940",           mime::VIDEO_MPEG_TS, labels::VIDEO_CIF30,  Class::Video };
					static Profile avc_ts_bl_cif30_aac_940_t         { "AVC_TS_BL_CIF30_AAC_940_T",         mime::VIDEO_MPEG_TS, labels::VIDEO_CIF30,  Class::Video };
					static Profile avc_ts_bl_cif30_aac_940_iso       { "AVC_TS_BL_CIF30_AAC_940_ISO",       mime::VIDEO_MPEG,    labels::VIDEO_CIF30,  Class::Video };
					static Profile avc_ts_mp_hd_aac_mult5            { "AVC_TS_MP_HD_AAC_MULT5",            mime::VIDEO_MPEG_TS, labels::VIDEO_HD,     Class::Video };
					static Profile avc_ts_mp_hd_aac_mult5_t          { "AVC_TS_MP_HD_AAC_MULT5_T",          mime::VIDEO_MPEG_TS, labels::VIDEO_HD,     Class::Video };
					static Profile avc_ts_mp_hd_aac_mult5_iso        { "AVC_TS_MP_HD_AAC_MULT5_ISO",        mime::VIDEO_MPEG,    labels::VIDEO_HD,     Class::Video };
					static Profile avc_ts_mp_hd_heaac_l2             { "AVC_TS_MP_HD_HEAAC_L2",             mime::VIDEO_MPEG_TS, labels::VIDEO_HD,     Class::Video };
					static Profile avc_ts_mp_hd_heaac_l2_t           { "AVC_TS_MP_HD_HEAAC_L2_T",           mime::VIDEO_MPEG_TS, labels::VIDEO_HD,     Class::Video };
					static Profile avc_ts_mp_hd_heaac_l2_iso         { "AVC_TS_MP_HD_HEAAC_L2_ISO",         mime::VIDEO_MPEG,    labels::VIDEO_HD,     Class::Video };
					static Profile avc_ts_mp_hd_mpeg1_l3             { "AVC_TS_MP_HD_MPEG1_L3",             mime::VIDEO_MPEG_TS, labels::VIDEO_HD,     Class::Video };
					static Profile avc_ts_mp_hd_mpeg1_l3_t           { "AVC_TS_MP_HD_MPEG1_L3_T",           mime::VIDEO_MPEG_TS, labels::VIDEO_HD,     Class::Video };
					static Profile avc_ts_mp_hd_mpeg1_l3_iso         { "AVC_TS_MP_HD_MPEG1_L3_ISO",         mime::VIDEO_MPEG,    labels::VIDEO_HD,     Class::Video };
					static Profile avc_ts_mp_hd_ac3                  { "AVC_TS_MP_HD_AC3",                  mime::VIDEO_MPEG_TS, labels::VIDEO_HD,     Class::Video };
					static Profile avc_ts_mp_hd_ac3_t                { "AVC_TS_MP_HD_AC3_T",                mime::VIDEO_MPEG_TS, labels::VIDEO_HD,     Class::Video };
					static Profile avc_ts_mp_hd_ac3_iso              { "AVC_TS_MP_HD_AC3_ISO",              mime::VIDEO_MPEG,    labels::VIDEO_HD,     Class::Video };
					static Profile avc_ts_mp_hd_aac                  { "AVC_TS_MP_HD_AAC",                  mime::VIDEO_MPEG_TS, labels::VIDEO_HD,     Class::Video };
					static Profile avc_ts_mp_hd_aac_t                { "AVC_TS_MP_HD_AAC_T",                mime::VIDEO_MPEG_TS, labels::VIDEO_HD,     Class::Video };
					static Profile avc_ts_mp_hd_aac_iso              { "AVC_TS_MP_HD_AAC_ISO",              mime::VIDEO_MPEG,    labels::VIDEO_HD,     Class::Video };
					static Profile avc_ts_mp_hd_aac_ltp              { "AVC_TS_MP_HD_AAC_LTP",              mime::VIDEO_MPEG_TS, labels::VIDEO_HD,     Class::Video };
					static Profile avc_ts_mp_hd_aac_ltp_t            { "AVC_TS_MP_HD_AAC_LTP_T",            mime::VIDEO_MPEG_TS, labels::VIDEO_HD,     Class::Video };
					static Profile avc_ts_mp_hd_aac_ltp_iso          { "AVC_TS_MP_HD_AAC_LTP_ISO",          mime::VIDEO_MPEG,    labels::VIDEO_HD,     Class::Video };
					static Profile avc_ts_mp_hd_aac_ltp_mult5        { "AVC_TS_MP_HD_AAC_LTP_MULT5",        mime::VIDEO_MPEG_TS, labels::VIDEO_HD,     Class::Video };
					static Profile avc_ts_mp_hd_aac_ltp_mult5_t      { "AVC_TS_MP_HD_AAC_LTP_MULT5_T",      mime::VIDEO_MPEG_TS, labels::VIDEO_HD,     Class::Video };
					static Profile avc_ts_mp_hd_aac_ltp_mult5_iso    { "AVC_TS_MP_HD_AAC_LTP_MULT5_ISO",    mime::VIDEO_MPEG,    labels::VIDEO_HD,     Class::Video };
					static Profile avc_ts_mp_hd_aac_ltp_mult7        { "AVC_TS_MP_HD_AAC_LTP_MULT7",        mime::VIDEO_MPEG_TS, labels::VIDEO_HD,     Class::Video };
					static Profile avc_ts_mp_hd_aac_ltp_mult7_t      { "AVC_TS_MP_HD_AAC_LTP_MULT7_T",      mime::VIDEO_MPEG_TS, labels::VIDEO_HD,     Class::Video };
					static Profile avc_ts_mp_hd_aac_ltp_mult7_iso    { "AVC_TS_MP_HD_AAC_LTP_MULT7_ISO",    mime::VIDEO_MPEG,    labels::VIDEO_HD,     Class::Video };
					static Profile avc_ts_bl_cif15_aac               { "AVC_TS_BL_CIF15_AAC",               mime::VIDEO_MPEG_TS, labels::VIDEO_CIF15,  Class::Video };
					static Profile avc_ts_bl_cif15_aac_t             { "AVC_TS_BL_CIF15_AAC_T",             mime::VIDEO_MPEG_TS, labels::VIDEO_CIF15,  Class::Video };
					static Profile avc_ts_bl_cif15_aac_iso           { "AVC_TS_BL_CIF15_AAC_ISO",           mime::VIDEO_MPEG,    labels::VIDEO_CIF15,  Class::Video };
					static Profile avc_ts_bl_cif15_aac_540           { "AVC_TS_BL_CIF15_AAC_540",           mime::VIDEO_MPEG_TS, labels::VIDEO_CIF15,  Class::Video };
					static Profile avc_ts_bl_cif15_aac_540_t         { "AVC_TS_BL_CIF15_AAC_540_T",         mime::VIDEO_MPEG_TS, labels::VIDEO_CIF15,  Class::Video };
					static Profile avc_ts_bl_cif15_aac_540_iso       { "AVC_TS_BL_CIF15_AAC_540_ISO",       mime::VIDEO_MPEG,    labels::VIDEO_CIF15,  Class::Video };
					static Profile avc_ts_bl_cif15_aac_ltp           { "AVC_TS_BL_CIF15_AAC_LTP",           mime::VIDEO_MPEG_TS, labels::VIDEO_CIF15,  Class::Video };
					static Profile avc_ts_bl_cif15_aac_ltp_t         { "AVC_TS_BL_CIF15_AAC_LTP_T",         mime::VIDEO_MPEG_TS, labels::VIDEO_CIF15,  Class::Video };
					static Profile avc_ts_bl_cif15_aac_ltp_iso       { "AVC_TS_BL_CIF15_AAC_LTP_ISO",       mime::VIDEO_MPEG,    labels::VIDEO_CIF15,  Class::Video };
					static Profile avc_ts_bl_cif15_bsac              { "AVC_TS_BL_CIF15_BSAC",              mime::VIDEO_MPEG_TS, labels::VIDEO_CIF15,  Class::Video };
					static Profile avc_ts_bl_cif15_bsac_t            { "AVC_TS_BL_CIF15_BSAC_T",            mime::VIDEO_MPEG_TS, labels::VIDEO_CIF15,  Class::Video };
					static Profile avc_ts_bl_cif15_bsac_iso          { "AVC_TS_BL_CIF15_BSAC_ISO",          mime::VIDEO_MPEG,    labels::VIDEO_CIF15,  Class::Video };
					static Profile avc_3gpp_bl_cif30_amr_wbplus      { "AVC_3GPP_BL_CIF30_AMR_WBplus",      mime::VIDEO_3GP,     labels::VIDEO_CIF30,  Class::Video };
					static Profile avc_3gpp_bl_cif15_amr_wbplus      { "AVC_3GPP_BL_CIF15_AMR_WBplus",      mime::VIDEO_3GP,     labels::VIDEO_CIF15,  Class::Video };
					static Profile avc_3gpp_bl_qcif15_aac            { "AVC_3GPP_BL_QCIF15_AAC",            mime::VIDEO_3GP,     labels::VIDEO_QCIF15, Class::Video };
					static Profile avc_3gpp_bl_qcif15_aac_ltp        { "AVC_3GPP_BL_QCIF15_AAC_LTP",        mime::VIDEO_3GP,     labels::VIDEO_QCIF15, Class::Video };
					static Profile avc_3gpp_bl_qcif15_heaac          { "AVC_3GPP_BL_QCIF15_HEAAC",          mime::VIDEO_3GP,     labels::VIDEO_QCIF15, Class::Video };
					static Profile avc_3gpp_bl_qcif15_amr_wbplus     { "AVC_3GPP_BL_QCIF15_AMR_WBplus",     mime::VIDEO_3GP,     labels::VIDEO_QCIF15, Class::Video };
					static Profile avc_3gpp_bl_qcif15_amr            { "AVC_3GPP_BL_QCIF15_AMR",            mime::VIDEO_3GP,     labels::VIDEO_QCIF15, Class::Video };
					enum class profile
					{
						INVALID,
						BL_QCIF15,
						BL_L1B_QCIF,
						BL_L12_CIF15,
						BL_CIF15,
						BL_CIF15_520,
						BL_CIF15_540,
						BL_L2_CIF30,
						BL_CIF30,
						BL_CIF30_940,
						BL_L3L_SD,
						BL_L3_SD,
						MP_SD,
						MP_HD
					};

					struct video_properties {
						int width;
						int height;
					};

					static video_properties profile_cif_res [] =
					{
						{ 352, 288 }, /* CIF */
						{ 352, 240 }, /* 525SIF */
						{ 320, 240 }, /* QVGA 4:3 */
						{ 320, 180 }, /* QVGA 16:9 */
						{ 240, 180 }, /* 1/7 VGA 4:3 */
						{ 240, 135 }, /* 1/7 VGA 16:9 */
						{ 208, 160 }, /* 1/9 VGA 4:3 */
						{ 176, 144 }, /* QCIF,625QCIF */
						{ 176, 120 }, /* 525QCIF */
						{ 160, 120 }, /* SQVGA 4:3 */
						{ 160, 112 }, /* 1/16 VGA 4:3 */
						{ 160,  90 }, /* SQVGA 16:9 */
						{ 128,  96 }  /* SQCIF */
					};

					static video_properties profile_mp_l3_sd_res [] =
					{
						{ 720, 576 }, /* 625 D1 */
						{ 720, 480 }, /* 525 D1 */
						{ 640, 480 }, /* VGA */
						{ 640, 360 }  /* VGA 16:9 */
					};

					static video_properties profile_mp_sd_res [] =
					{
						{ 720, 576 }, /* 625 D1 */
						{ 720, 480 }, /* 525 D1 */
						{ 704, 576 }, /* 625 4SIF */
						{ 704, 480 }, /* 525 4SIF */
						{ 640, 480 }, /* VGA */
						{ 640, 360 }, /* VGA 16:9 */
						{ 544, 576 }, /* 625 3/4 D1 */
						{ 544, 480 }, /* 525 3/4 D1 */
						{ 480, 576 }, /* 625 2/3 D1 */
						{ 480, 480 }, /* 525 2/3 D1 */
						{ 480, 360 }, /* 9/16 VGA 4:3 */
						{ 480, 270 }, /* 9/16 VGA 16:9 */
						{ 352, 576 }, /* 625 1/2 D1 */
						{ 352, 480 }, /* 525 1/2 D1 */
						{ 352, 288 }, /* CIF, 625SIF */
						{ 352, 240 }, /* 525SIF */
						{ 320, 240 }, /* QVGA 4:3 */
						{ 320, 180 }, /* QVGA 16:9 */
						{ 240, 180 }, /* 1/7 VGA 4:3 */
						{ 208, 160 }, /* 1/9 VGA 4:3 */
						{ 176, 144 }, /* QCIF,625QCIF */
						{ 176, 120 }, /* 525QCIF */
						{ 160, 120 }, /* SQVGA 4:3 */
						{ 160, 112 }, /* 1/16 VGA 4:3 */
						{ 160,  90 }, /* SQVGA 16:9 */
						{ 128,  96 }  /* SQCIF */
					};

					static video_properties profile_mp_hd_res [] =
					{
						{ 1920, 1080 }, /* 1080p */
						{ 1920, 1152 },
						{ 1920,  540 }, /* 1080i */
						{ 1280,  720 }  /* 720p */
					};

					struct avc_profile
					{
						const Profile *profile;
						container::container_type container;
						part10::profile video_profile;
						audio::profile audio_profile;
					};
					
					static const avc_profile avc_profiles_mapping [] =
					{
						/* MPEG-4 Container */
						{ &avc_mp4_mp_sd_aac_mult5, container::MP4, profile::MP_SD, audio::profile::AAC_MULT5 },
						{ &avc_mp4_mp_sd_heaac_l2, container::MP4, profile::MP_SD, audio::profile::AAC_HE_L2 },
						{ &avc_mp4_mp_sd_mpeg1_l3, container::MP4, profile::MP_SD, audio::profile::MP3 },
						{ &avc_mp4_mp_sd_ac3, container::MP4, profile::MP_SD, audio::profile::AC3 },
						{ &avc_mp4_mp_sd_aac_ltp, container::MP4, profile::MP_SD, audio::profile::AAC_LTP },
						{ &avc_mp4_mp_sd_aac_ltp_mult5, container::MP4, profile::MP_SD, audio::profile::AAC_LTP_MULT5 },
						{ &avc_mp4_mp_sd_aac_ltp_mult7, container::MP4, profile::MP_SD, audio::profile::AAC_LTP_MULT7 },
						{ &avc_mp4_mp_sd_atrac3plus, container::MP4, profile::MP_SD, audio::profile::ATRAC },
						{ &avc_mp4_mp_sd_bsac, container::MP4, profile::MP_SD, audio::profile::AAC_BSAC },

						{ &avc_mp4_bl_l3l_sd_aac, container::MP4, profile::BL_L3L_SD, audio::profile::AAC },
						{ &avc_mp4_bl_l3l_sd_heaac, container::MP4, profile::BL_L3L_SD, audio::profile::AAC_HE_L2 },

						{ &avc_mp4_bl_l3_sd_aac, container::MP4, profile::BL_L3_SD, audio::profile::AAC },

						{ &avc_mp4_bl_cif30_aac_mult5, container::MP4, profile::BL_CIF30, audio::profile::AAC_MULT5 },
						{ &avc_mp4_bl_cif30_heaac_l2, container::MP4, profile::BL_CIF30, audio::profile::AAC_HE_L2 },
						{ &avc_mp4_bl_cif30_mpeg1_l3, container::MP4, profile::BL_CIF30, audio::profile::MP3 },
						{ &avc_mp4_bl_cif30_ac3, container::MP4, profile::BL_CIF30, audio::profile::AC3 },
						{ &avc_mp4_bl_cif30_aac_ltp, container::MP4, profile::BL_CIF30, audio::profile::AAC_LTP },
						{ &avc_mp4_bl_cif30_aac_ltp_mult5, container::MP4, profile::BL_CIF30, audio::profile::AAC_LTP_MULT5 },
						{ &avc_mp4_bl_cif30_bsac, container::MP4, profile::BL_CIF30, audio::profile::AAC_BSAC },
						{ &avc_mp4_bl_cif30_bsac_mult5, container::MP4, profile::BL_CIF30, audio::profile::AAC_BSAC_MULT5 },

						{ &avc_mp4_bl_l2_cif30_aac, container::MP4, profile::BL_L2_CIF30, audio::profile::AAC },

						{ &avc_mp4_bl_cif15_heaac, container::MP4, profile::BL_CIF15, audio::profile::AAC_HE_L2 },
						{ &avc_mp4_bl_cif15_amr, container::MP4, profile::BL_CIF15, audio::profile::AMR },
						{ &avc_mp4_bl_cif15_aac, container::MP4, profile::BL_CIF15, audio::profile::AAC },
						{ &avc_mp4_bl_cif15_aac_520, container::MP4, profile::BL_CIF15_520, audio::profile::AAC },
						{ &avc_mp4_bl_cif15_aac_ltp, container::MP4, profile::BL_CIF15, audio::profile::AAC_LTP },
						{ &avc_mp4_bl_cif15_aac_ltp_520, container::MP4, profile::BL_CIF15_520, audio::profile::AAC_LTP },
						{ &avc_mp4_bl_cif15_bsac, container::MP4, profile::BL_CIF15, audio::profile::AAC_BSAC },

						{ &avc_mp4_bl_l12_cif15_heaac, container::MP4, profile::BL_L12_CIF15, audio::profile::AAC_HE_L2 },

						{ &avc_mp4_bl_l1b_qcif15_heaac, container::MP4, profile::BL_L1B_QCIF, audio::profile::AAC_HE_L2 },

						/* MPEG-TS Container */
						{ &avc_ts_mp_sd_aac_mult5, container::MPEG_TRANSPORT_STREAM_DLNA_NO_TS, profile::MP_SD, audio::profile::AAC_MULT5 },
						{ &avc_ts_mp_sd_aac_mult5_t, container::MPEG_TRANSPORT_STREAM_DLNA, profile::MP_SD, audio::profile::AAC_MULT5 },
						{ &avc_ts_mp_sd_aac_mult5_iso, container::MPEG_TRANSPORT_STREAM, profile::MP_SD, audio::profile::AAC_MULT5 },

						{ &avc_ts_mp_sd_heaac_l2, container::MPEG_TRANSPORT_STREAM_DLNA_NO_TS, profile::MP_SD, audio::profile::AAC_HE_L2 },
						{ &avc_ts_mp_sd_heaac_l2_t, container::MPEG_TRANSPORT_STREAM_DLNA, profile::MP_SD, audio::profile::AAC_HE_L2 },
						{ &avc_ts_mp_sd_heaac_l2_iso, container::MPEG_TRANSPORT_STREAM, profile::MP_SD, audio::profile::AAC_HE_L2 },

						{ &avc_ts_mp_sd_mpeg1_l3, container::MPEG_TRANSPORT_STREAM_DLNA_NO_TS, profile::MP_SD, audio::profile::MP3 },
						{ &avc_ts_mp_sd_mpeg1_l3_t, container::MPEG_TRANSPORT_STREAM_DLNA, profile::MP_SD, audio::profile::MP3 },
						{ &avc_ts_mp_sd_mpeg1_l3_iso, container::MPEG_TRANSPORT_STREAM, profile::MP_SD, audio::profile::MP3 },

						{ &avc_ts_mp_sd_ac3, container::MPEG_TRANSPORT_STREAM_DLNA_NO_TS, profile::MP_SD, audio::profile::AC3 },
						{ &avc_ts_mp_sd_ac3_t, container::MPEG_TRANSPORT_STREAM_DLNA, profile::MP_SD, audio::profile::AC3 },
						{ &avc_ts_mp_sd_ac3_iso, container::MPEG_TRANSPORT_STREAM, profile::MP_SD, audio::profile::AC3 },

						{ &avc_ts_mp_sd_aac_ltp, container::MPEG_TRANSPORT_STREAM_DLNA_NO_TS, profile::MP_SD, audio::profile::AAC_LTP },
						{ &avc_ts_mp_sd_aac_ltp_t, container::MPEG_TRANSPORT_STREAM_DLNA, profile::MP_SD, audio::profile::AAC_LTP },
						{ &avc_ts_mp_sd_aac_ltp_iso, container::MPEG_TRANSPORT_STREAM, profile::MP_SD, audio::profile::AAC_LTP },

						{ &avc_ts_mp_sd_aac_ltp_mult5, container::MPEG_TRANSPORT_STREAM_DLNA_NO_TS, profile::MP_SD, audio::profile::AAC_LTP_MULT5 },
						{ &avc_ts_mp_sd_aac_ltp_mult5_t, container::MPEG_TRANSPORT_STREAM_DLNA, profile::MP_SD, audio::profile::AAC_LTP_MULT5 },
						{ &avc_ts_mp_sd_aac_ltp_mult5_iso, container::MPEG_TRANSPORT_STREAM, profile::MP_SD, audio::profile::AAC_LTP_MULT5 },

						{ &avc_ts_mp_sd_aac_ltp_mult7, container::MPEG_TRANSPORT_STREAM_DLNA_NO_TS, profile::MP_SD, audio::profile::AAC_LTP_MULT7 },
						{ &avc_ts_mp_sd_aac_ltp_mult7_t, container::MPEG_TRANSPORT_STREAM_DLNA, profile::MP_SD, audio::profile::AAC_LTP_MULT7 },
						{ &avc_ts_mp_sd_aac_ltp_mult7_iso, container::MPEG_TRANSPORT_STREAM, profile::MP_SD, audio::profile::AAC_LTP_MULT7 },

						{ &avc_ts_mp_sd_bsac, container::MPEG_TRANSPORT_STREAM_DLNA_NO_TS, profile::MP_SD, audio::profile::AAC_BSAC },
						{ &avc_ts_mp_sd_bsac_t, container::MPEG_TRANSPORT_STREAM_DLNA, profile::MP_SD, audio::profile::AAC_BSAC },
						{ &avc_ts_mp_sd_bsac_iso, container::MPEG_TRANSPORT_STREAM, profile::MP_SD, audio::profile::AAC_BSAC },

						{ &avc_ts_bl_cif30_aac_mult5, container::MPEG_TRANSPORT_STREAM_DLNA_NO_TS, profile::BL_CIF30, audio::profile::AAC_MULT5 },
						{ &avc_ts_bl_cif30_aac_mult5_t, container::MPEG_TRANSPORT_STREAM_DLNA, profile::BL_CIF30, audio::profile::AAC_MULT5 },
						{ &avc_ts_bl_cif30_aac_mult5_iso, container::MPEG_TRANSPORT_STREAM, profile::BL_CIF30, audio::profile::AAC_MULT5 },

						{ &avc_ts_bl_cif30_heaac_l2, container::MPEG_TRANSPORT_STREAM_DLNA_NO_TS, profile::BL_CIF30, audio::profile::AAC_HE_L2 },
						{ &avc_ts_bl_cif30_heaac_l2_t, container::MPEG_TRANSPORT_STREAM_DLNA, profile::BL_CIF30, audio::profile::AAC_HE_L2 },
						{ &avc_ts_bl_cif30_heaac_l2_iso, container::MPEG_TRANSPORT_STREAM, profile::BL_CIF30, audio::profile::AAC_HE_L2 },

						{ &avc_ts_bl_cif30_mpeg1_l3, container::MPEG_TRANSPORT_STREAM_DLNA_NO_TS, profile::BL_CIF30, audio::profile::MP3 },
						{ &avc_ts_bl_cif30_mpeg1_l3_t, container::MPEG_TRANSPORT_STREAM_DLNA, profile::BL_CIF30, audio::profile::MP3 },
						{ &avc_ts_bl_cif30_mpeg1_l3_iso, container::MPEG_TRANSPORT_STREAM, profile::BL_CIF30, audio::profile::MP3 },

						{ &avc_ts_bl_cif30_ac3, container::MPEG_TRANSPORT_STREAM_DLNA_NO_TS, profile::BL_CIF30, audio::profile::AC3 },
						{ &avc_ts_bl_cif30_ac3_t, container::MPEG_TRANSPORT_STREAM_DLNA, profile::BL_CIF30, audio::profile::AC3 },
						{ &avc_ts_bl_cif30_ac3_iso, container::MPEG_TRANSPORT_STREAM, profile::BL_CIF30, audio::profile::AC3 },

						{ &avc_ts_bl_cif30_aac_ltp, container::MPEG_TRANSPORT_STREAM_DLNA_NO_TS, profile::BL_CIF30, audio::profile::AAC_LTP },
						{ &avc_ts_bl_cif30_aac_ltp_t, container::MPEG_TRANSPORT_STREAM_DLNA, profile::BL_CIF30, audio::profile::AAC_LTP },
						{ &avc_ts_bl_cif30_aac_ltp_iso, container::MPEG_TRANSPORT_STREAM, profile::BL_CIF30, audio::profile::AAC_LTP },

						{ &avc_ts_bl_cif30_aac_ltp_mult5, container::MPEG_TRANSPORT_STREAM_DLNA_NO_TS, profile::BL_CIF30, audio::profile::AAC_LTP_MULT5 },
						{ &avc_ts_bl_cif30_aac_ltp_mult5_t, container::MPEG_TRANSPORT_STREAM_DLNA, profile::BL_CIF30, audio::profile::AAC_LTP_MULT5 },
						{ &avc_ts_bl_cif30_aac_ltp_mult5_iso, container::MPEG_TRANSPORT_STREAM, profile::BL_CIF30, audio::profile::AAC_LTP_MULT5 },

						{ &avc_ts_bl_cif30_aac_940, container::MPEG_TRANSPORT_STREAM_DLNA_NO_TS, profile::BL_CIF30_940, audio::profile::AAC },
						{ &avc_ts_bl_cif30_aac_940_t, container::MPEG_TRANSPORT_STREAM_DLNA, profile::BL_CIF30_940, audio::profile::AAC },
						{ &avc_ts_bl_cif30_aac_940_iso, container::MPEG_TRANSPORT_STREAM, profile::BL_CIF30_940, audio::profile::AAC },

						{ &avc_ts_mp_hd_aac_mult5, container::MPEG_TRANSPORT_STREAM_DLNA_NO_TS, profile::MP_HD, audio::profile::AAC_MULT5 },
						{ &avc_ts_mp_hd_aac_mult5_t, container::MPEG_TRANSPORT_STREAM_DLNA, profile::MP_HD, audio::profile::AAC_MULT5 },
						{ &avc_ts_mp_hd_aac_mult5_iso, container::MPEG_TRANSPORT_STREAM, profile::MP_HD, audio::profile::AAC_MULT5 },

						{ &avc_ts_mp_hd_heaac_l2, container::MPEG_TRANSPORT_STREAM_DLNA_NO_TS, profile::MP_HD, audio::profile::AAC_HE_L2 },
						{ &avc_ts_mp_hd_heaac_l2_t, container::MPEG_TRANSPORT_STREAM_DLNA, profile::MP_HD, audio::profile::AAC_HE_L2 },
						{ &avc_ts_mp_hd_heaac_l2_iso, container::MPEG_TRANSPORT_STREAM, profile::MP_HD, audio::profile::AAC_HE_L2 },

						{ &avc_ts_mp_hd_mpeg1_l3, container::MPEG_TRANSPORT_STREAM_DLNA_NO_TS, profile::MP_HD, audio::profile::MP3 },
						{ &avc_ts_mp_hd_mpeg1_l3_t, container::MPEG_TRANSPORT_STREAM_DLNA, profile::MP_HD, audio::profile::MP3 },
						{ &avc_ts_mp_hd_mpeg1_l3_iso, container::MPEG_TRANSPORT_STREAM, profile::MP_HD, audio::profile::MP3 },

						{ &avc_ts_mp_hd_ac3, container::MPEG_TRANSPORT_STREAM_DLNA_NO_TS, profile::MP_HD, audio::profile::AC3 },
						{ &avc_ts_mp_hd_ac3_t, container::MPEG_TRANSPORT_STREAM_DLNA, profile::MP_HD, audio::profile::AC3 },
						{ &avc_ts_mp_hd_ac3_iso, container::MPEG_TRANSPORT_STREAM, profile::MP_HD, audio::profile::AC3 },

						{ &avc_ts_mp_hd_aac, container::MPEG_TRANSPORT_STREAM_DLNA_NO_TS, profile::MP_HD, audio::profile::AAC },
						{ &avc_ts_mp_hd_aac_t, container::MPEG_TRANSPORT_STREAM_DLNA, profile::MP_HD, audio::profile::AAC },
						{ &avc_ts_mp_hd_aac_iso, container::MPEG_TRANSPORT_STREAM, profile::MP_HD, audio::profile::AAC },

						{ &avc_ts_mp_hd_aac_ltp, container::MPEG_TRANSPORT_STREAM_DLNA_NO_TS, profile::MP_HD, audio::profile::AAC_LTP },
						{ &avc_ts_mp_hd_aac_ltp_t, container::MPEG_TRANSPORT_STREAM_DLNA, profile::MP_HD, audio::profile::AAC_LTP },
						{ &avc_ts_mp_hd_aac_ltp_iso, container::MPEG_TRANSPORT_STREAM, profile::MP_HD, audio::profile::AAC_LTP },

						{ &avc_ts_mp_hd_aac_ltp_mult5, container::MPEG_TRANSPORT_STREAM_DLNA_NO_TS, profile::MP_HD, audio::profile::AAC_LTP_MULT5 },
						{ &avc_ts_mp_hd_aac_ltp_mult5_t, container::MPEG_TRANSPORT_STREAM_DLNA, profile::MP_HD, audio::profile::AAC_LTP_MULT5 },
						{ &avc_ts_mp_hd_aac_ltp_mult5_iso, container::MPEG_TRANSPORT_STREAM, profile::MP_HD, audio::profile::AAC_LTP_MULT5 },

						{ &avc_ts_mp_hd_aac_ltp_mult7, container::MPEG_TRANSPORT_STREAM_DLNA_NO_TS, profile::MP_HD, audio::profile::AAC_LTP_MULT7 },
						{ &avc_ts_mp_hd_aac_ltp_mult7_t, container::MPEG_TRANSPORT_STREAM_DLNA, profile::MP_HD, audio::profile::AAC_LTP_MULT7 },
						{ &avc_ts_mp_hd_aac_ltp_mult7_iso, container::MPEG_TRANSPORT_STREAM, profile::MP_HD, audio::profile::AAC_LTP_MULT7 },

						{ &avc_ts_bl_cif15_aac, container::MPEG_TRANSPORT_STREAM_DLNA_NO_TS, profile::BL_CIF15, audio::profile::AAC },
						{ &avc_ts_bl_cif15_aac_t, container::MPEG_TRANSPORT_STREAM_DLNA, profile::BL_CIF15, audio::profile::AAC },
						{ &avc_ts_bl_cif15_aac_iso, container::MPEG_TRANSPORT_STREAM, profile::BL_CIF15, audio::profile::AAC },

						{ &avc_ts_bl_cif15_aac_540, container::MPEG_TRANSPORT_STREAM_DLNA_NO_TS, profile::BL_CIF15_540, audio::profile::AAC },
						{ &avc_ts_bl_cif15_aac_540_t, container::MPEG_TRANSPORT_STREAM_DLNA, profile::BL_CIF15_540, audio::profile::AAC },
						{ &avc_ts_bl_cif15_aac_540_iso, container::MPEG_TRANSPORT_STREAM, profile::BL_CIF15_540, audio::profile::AAC },

						{ &avc_ts_bl_cif15_aac_ltp, container::MPEG_TRANSPORT_STREAM_DLNA_NO_TS, profile::BL_CIF15, audio::profile::AAC_LTP },
						{ &avc_ts_bl_cif15_aac_ltp_t, container::MPEG_TRANSPORT_STREAM_DLNA, profile::BL_CIF15, audio::profile::AAC_LTP },
						{ &avc_ts_bl_cif15_aac_ltp_iso, container::MPEG_TRANSPORT_STREAM, profile::BL_CIF15, audio::profile::AAC_LTP },

						{ &avc_ts_bl_cif15_bsac, container::MPEG_TRANSPORT_STREAM_DLNA_NO_TS, profile::BL_CIF15, audio::profile::AAC_BSAC },
						{ &avc_ts_bl_cif15_bsac_t, container::MPEG_TRANSPORT_STREAM_DLNA, profile::BL_CIF15, audio::profile::AAC_BSAC },
						{ &avc_ts_bl_cif15_bsac_iso, container::MPEG_TRANSPORT_STREAM, profile::BL_CIF15, audio::profile::AAC_BSAC },

						/* 3GPP Container */
						{ &avc_3gpp_bl_cif30_amr_wbplus, container::_3GP, profile::BL_CIF30, audio::profile::AMR_WB },

						{ &avc_3gpp_bl_cif15_amr_wbplus, container::_3GP, profile::BL_CIF15, audio::profile::AMR_WB },

						{ &avc_3gpp_bl_qcif15_aac, container::_3GP, profile::BL_QCIF15, audio::profile::AAC },
						{ &avc_3gpp_bl_qcif15_aac_ltp, container::_3GP, profile::BL_QCIF15, audio::profile::AAC_LTP },
						{ &avc_3gpp_bl_qcif15_heaac, container::_3GP, profile::BL_QCIF15, audio::profile::AAC_HE_L2 },
						{ &avc_3gpp_bl_qcif15_amr_wbplus, container::_3GP, profile::BL_QCIF15, audio::profile::AMR_WB },
						{ &avc_3gpp_bl_qcif15_amr, container::_3GP, profile::BL_QCIF15, audio::profile::AMR }
					};

					template <size_t size>
					static inline bool is_valid_video_profile(const video_properties(&props)[size], AVCodecContext *codec)
					{
						for (auto && prop : props)
						{
							if (prop.width != codec->width) continue;
							if (prop.height != codec->height) continue;
							return true;

						}

						return false;
					}

#define CHECK_VIDEO_PROFILE(props, profile_id) if (is_valid_video_profile(props, codec)) return profile_id

					static profile get_profile(AVFormatContext *ctx, AVStream *stream, AVCodecContext *codec)
					{
						if (!stream || !codec)
							return profile::INVALID;

						/* stupid exception to CIF15 */
						if (codec->bit_rate <= 384000 && ctx->bit_rate <= 600000 && codec->width == 320 && codec->height == 240)
							return profile::BL_L12_CIF15;

						/* CIF */
						if (is_valid_video_profile(profile_cif_res, codec))
						{
							/* QCIF */
							if (codec->bit_rate <= 128000 && ctx->bit_rate <= 256000)
							{
								if (stream->r_frame_rate.num == 15 && stream->r_frame_rate.num == 1)
									return profile::BL_QCIF15;
								else
									return profile::BL_L1B_QCIF;
							}

							/* CIF15 */
							if (ctx->bit_rate <= 520000) /* 520 kbps max system bitrate */
								return profile::BL_CIF15_520;
							if (ctx->bit_rate <= 540000) /* 540 kbps max system bitrate */
								return profile::BL_CIF15_540;

							/* 384 kbps max video bitrate */
							if (codec->bit_rate <= 384000 && ctx->bit_rate <= 600000)
								return profile::BL_CIF15;

							/* CIF30 */
							if (ctx->bit_rate <= 940000) /* 940 kbps max system bitrate */
								return profile::BL_CIF30_940;
							if (ctx->bit_rate <= 1300000) /* 1.3 Mbps kbps max system bitrate */
								return profile::BL_L2_CIF30;

							/* 2 Mbps max video bitrate */
							if (codec->bit_rate <= 2000000 && ctx->bit_rate <= 3000000)
								return profile::BL_CIF30;
						}

						/* SD */
						if (codec->bit_rate <= 4000000 && is_valid_video_profile(profile_mp_l3_sd_res, codec)) /* 4 Mbps max */
							return profile::BL_L3_SD;
						/* what is BL_L3L ?? */

						if (codec->bit_rate <= 10000000 && is_valid_video_profile(profile_mp_sd_res, codec)) /* 10 Mbps max */
							return profile::MP_SD;

						/* HD */
						if (codec->bit_rate <= 20000000) /* 20 Mbps max */
						{
							if (is_valid_video_profile(profile_mp_hd_res, codec))
								return profile::MP_HD;

							/* dirty hack to support some excentric 480/720/1080(i,p) files
							where only one of the size is correct */
							if (codec->width == 1920 || codec->width == 1280 || codec->width == 720)
								return profile::MP_HD;
							if (codec->height == 1080 || codec->height == 720 || codec->height == 480)
								return profile::MP_HD;
						}

						return profile::INVALID;
					}
#undef CHECK_VIDEO_PROFILE

					static const Profile* probe(AVFormatContext *ctx, container::container_type container, const stream_codec& codecs)
					{
						if (!stream_is_video(ctx, container, codecs))
							return nullptr;

						/* check for H.264/AVC codec */
						if (codecs.m_video.m_codec->codec_id != CODEC_ID_H264)
							return nullptr;

						/* check for a supported container */
						if (container != container::_3GP &&
							container != container::MP4 &&
							container != container::MPEG_TRANSPORT_STREAM &&
							container != container::MPEG_TRANSPORT_STREAM_DLNA &&
							container != container::MPEG_TRANSPORT_STREAM_DLNA_NO_TS)
							return nullptr;

						/* ensure we have a valid video codec bit rate */
						if (codecs.m_video.m_codec->bit_rate == 0)
						{
							codecs.m_video.m_codec->bit_rate =
								codecs.m_audio.m_codec->bit_rate ? ctx->bit_rate - codecs.m_audio.m_codec->bit_rate : ctx->bit_rate;
						}

						/* check for valid video profile */
						auto video_profile = get_profile(ctx, codecs.m_video.m_stream, codecs.m_video.m_codec);
						if (video_profile == part10::profile::INVALID)
							return nullptr;

						/* check for valid audio profile */
						auto audio_profile = audio::guess_profile(codecs.m_audio.m_codec);
						if (audio_profile == audio::profile::INVALID)
							return nullptr;

						/* AAC fixup: _320 profiles are audio-only profiles */
						if (audio_profile == audio::profile::AAC_320)
							audio_profile = audio::profile::AAC;
						if (audio_profile == audio::profile::AAC_HE_L2_320)
							audio_profile = audio::profile::AAC_HE_L2;

						/* find profile according to container type, video and audio profiles */
						for (auto && map : avc_profiles_mapping)
						{
							if (map.container != container) continue;
							if (map.video_profile != video_profile) continue;
							if (map.audio_profile != audio_profile) continue;
							return map.profile;
						}

						return nullptr;
					}
				}
			}
		}

		void register_video_profiles()
		{
			profile_db::register_profile(video::mpeg4::part2::probe,  "mov,hdmov,mp4,3gp,3gpp,asf,mpg,mpeg,mpe,mp2t,ts");
			profile_db::register_profile(video::mpeg4::part10::probe, "mov,hdmov,mp4,3gp,3gpp,mpg,mpeg,mpe,mp2t,ts");
		}
	}
}
