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
		namespace audio
		{
			namespace aac
			{
				static Profile aac_adts             { "AAC_ADTS",          mime::AUDIO_ADTS,   labels::AUDIO_2CH,   Class::Audio };
				static Profile aac_adts_320         { "AAC_ADTS_320",      mime::AUDIO_ADTS,   labels::AUDIO_2CH,   Class::Audio };
				static Profile aac_iso              { "AAC_ISO",           mime::AUDIO_MPEG_4, labels::AUDIO_2CH,   Class::Audio };
				static Profile aac_iso_320          { "AAC_ISO_320",       mime::AUDIO_MPEG_4, labels::AUDIO_2CH,   Class::Audio };
				static Profile aac_ltp_iso          { "AAC_LTP_ISO",       mime::AUDIO_MPEG_4, labels::AUDIO_2CH,   Class::Audio };
				static Profile aac_ltp_mult5_iso    { "AAC_LTP_MULT5_ISO", mime::AUDIO_MPEG_4, labels::AUDIO_MULTI, Class::Audio };
				static Profile aac_ltp_mult7_iso    { "AAC_LTP_MULT7_ISO", mime::AUDIO_MPEG_4, labels::AUDIO_MULTI, Class::Audio };
				static Profile aac_mult5_adts       { "AAC_MULT5_ADTS",    mime::AUDIO_ADTS,   labels::AUDIO_MULTI, Class::Audio };
				static Profile aac_mult5_iso        { "AAC_MULT5_ISO",     mime::AUDIO_MPEG_4, labels::AUDIO_MULTI, Class::Audio };
				static Profile heaac_l2_adts        { "HEAAC_L2_ADTS",     mime::AUDIO_ADTS,   labels::AUDIO_2CH,   Class::Audio };
				static Profile heaac_l2_iso         { "HEAAC_L2_ISO",      mime::AUDIO_MPEG_4, labels::AUDIO_2CH,   Class::Audio };
				static Profile heaac_l3_adts        { "HEAAC_L3_ADTS",     mime::AUDIO_ADTS,   labels::AUDIO_2CH,   Class::Audio };
				static Profile heaac_l3_iso         { "HEAAC_L3_ISO",      mime::AUDIO_MPEG_4, labels::AUDIO_2CH,   Class::Audio };
				static Profile heaac_mult5_adts     { "HEAAC_MULT5_ADTS",  mime::AUDIO_ADTS,   labels::AUDIO_MULTI, Class::Audio };
				static Profile heaac_mult5_iso      { "HEAAC_MULT5_ISO",   mime::AUDIO_MPEG_4, labels::AUDIO_MULTI, Class::Audio };
				static Profile heaac_l2_adts_320    { "HEAAC_L2_ADTS_320", mime::AUDIO_ADTS,   labels::AUDIO_2CH,   Class::Audio };
				static Profile heaac_l2_iso_320     { "HEAAC_L2_ISO_320",  mime::AUDIO_MPEG_4, labels::AUDIO_2CH,   Class::Audio };
				static Profile bsac_iso             { "BSAC_ISO",          mime::AUDIO_MPEG_4, labels::AUDIO_2CH,   Class::Audio };
				static Profile bsac_mult5_iso       { "BSAC_MULT5_ISO",    mime::AUDIO_MPEG_4, labels::AUDIO_MULTI, Class::Audio };
				static Profile heaac_v2_l2          { "HEAACv2_L2",        mime::AUDIO_MPEG_4, labels::AUDIO_2CH,   Class::Audio };
				static Profile heaac_v2_l2_adts     { "HEAACv2_L2",        mime::AUDIO_ADTS,   labels::AUDIO_2CH,   Class::Audio };
				static Profile heaac_v2_l2_320      { "HEAACv2_L2_320",    mime::AUDIO_MPEG_4, labels::AUDIO_2CH,   Class::Audio };
				static Profile heaac_v2_l2_320_adts { "HEAACv2_L2_320",    mime::AUDIO_ADTS,   labels::AUDIO_2CH,   Class::Audio };
				static Profile heaac_v2_l3          { "HEAACv2_L3",        mime::AUDIO_MPEG_4, labels::AUDIO_2CH,   Class::Audio };
				static Profile heaac_v2_l3_adts     { "HEAACv2_L3",        mime::AUDIO_ADTS,   labels::AUDIO_2CH,   Class::Audio };
				static Profile heaac_v2_mult5       { "HEAACv2_MULT5",     mime::AUDIO_MPEG_4, labels::AUDIO_2CH,   Class::Audio };
				static Profile heaac_v2_mult5_adts  { "HEAACv2_MULT5",     mime::AUDIO_ADTS,   labels::AUDIO_2CH,   Class::Audio };

				enum container_type
				{
					MUXED,
					RAW
				};

				enum object_type
				{
					INVALID   =  0,
					MAIN      =  1,
					LC        =  2,
					SSR       =  3,
					LTP       =  4,
					HE        =  5,
					SCALE     =  6,
					TWINVQ    =  7,
					CELP      =  8,
					HVXC      =  9,
					TTSI      = 12,
					MS        = 13,
					WAVE      = 14,
					MIDI      = 15,
					FX        = 16,
					LC_ER     = 17,
					LTP_ER    = 19,
					SCALE_ER  = 20,
					TWINVQ_ER = 21,
					BSAC_ER   = 22,
					LD_ER     = 23,
					CELP_ER   = 24,
					HXVC_ER   = 25,
					HILN_ER   = 26,
					PARAM_ER  = 27,
					SSC       = 28,
					HE_L3     = 31,
				};

				struct Mapping
				{
					Profile *m_profile;
					container_type m_container;
					audio::profile m_audio_profile;
				};
				const Mapping mapping [] =
				{
					{ &aac_adts,              RAW,     profile::AAC              },
					{ &aac_adts_320,          RAW,     profile::AAC_320          },
					{ &aac_iso,               MUXED,   profile::AAC              },
					{ &aac_iso_320,           MUXED,   profile::AAC_320          },
					{ &aac_ltp_iso,           MUXED,   profile::AAC_LTP          },
					{ &aac_ltp_mult5_iso,     MUXED,   profile::AAC_LTP_MULT5    },
					{ &aac_ltp_mult7_iso,     MUXED,   profile::AAC_LTP_MULT7    },
					{ &aac_mult5_adts,        RAW,     profile::AAC_MULT5        },
					{ &aac_mult5_iso,         MUXED,   profile::AAC_MULT5        },
					{ &heaac_l2_adts,         RAW,     profile::AAC_HE_L2        },
					{ &heaac_l2_iso,          MUXED,   profile::AAC_HE_L2        },
					{ &heaac_l3_adts,         RAW,     profile::AAC_HE_L3        },
					{ &heaac_l3_iso,          MUXED,   profile::AAC_HE_L3        },
					{ &heaac_mult5_adts,      RAW,     profile::AAC_HE_MULT5     },
					{ &heaac_mult5_iso,       MUXED,   profile::AAC_HE_MULT5     },
					{ &heaac_l2_adts_320,     RAW,     profile::AAC_HE_L2_320    },
					{ &heaac_l2_iso_320,      MUXED,   profile::AAC_HE_L2_320    },
					{ &heaac_v2_l2,           MUXED,   profile::AAC_HE_V2_L2     },
					{ &heaac_v2_l2_adts,      RAW,     profile::AAC_HE_V2_L2     },
					{ &heaac_v2_l2_320,       MUXED,   profile::AAC_HE_V2_L2_320 },
					{ &heaac_v2_l2_320_adts,  RAW,     profile::AAC_HE_V2_L2_320 },
					{ &heaac_v2_l3,           MUXED,   profile::AAC_HE_V2_L3     },
					{ &heaac_v2_l3_adts,      RAW,     profile::AAC_HE_V2_L3     },
					{ &heaac_v2_mult5,        MUXED,   profile::AAC_HE_V2_MULT5  },
					{ &heaac_v2_mult5_adts,   RAW,     profile::AAC_HE_V2_MULT5  },
					{ &bsac_iso,              MUXED,   profile::AAC_BSAC         },
					{ &bsac_mult5_iso,        MUXED,   profile::AAC_BSAC_MULT5   }
				};

				static container_type get_format(AVFormatContext *ctx)
				{
					unsigned char buffer[4];
					if (!ctx)
						return MUXED;

					{
						fs::ifstream f(ctx->filename, std::ios::in | std::ios::binary);
						f.read((char*) buffer, sizeof(buffer));
						if (f.gcount() != sizeof(buffer))
							return MUXED;
					}


					if ((buffer[0] == 0xFF) && ((buffer[1] & 0xF6) == 0xF0))
					{
						return RAW;
					}

					return MUXED;
				}

				static object_type adts_object_type_get(AVFormatContext *ctx)
				{
					unsigned char buffer[4];
					if (!ctx)
						return INVALID;

					{
						fs::ifstream f(ctx->filename, std::ios::in | std::ios::binary);
						f.read((char*) buffer, sizeof(buffer));
						if (f.gcount() != sizeof(buffer))
							return INVALID;
					}

					return (object_type)((buffer[2] & 0xC0) >> 6);
				}

				static object_type object_type_get(uint8_t *data, size_t len)
				{
					if (data && len)
						return (object_type)(*data >> 3);

					return INVALID;
				}

				struct priv_profile
				{
					object_type m_type;
					int m_max_sample_rate;
					int m_max_channels;
					int m_max_bit_rate;
					audio::profile m_profile;
				};

				static const priv_profile priv_profiles [] =
				{
					{ LC,       48000, 2,  320000, audio::profile::AAC_320 },
					{ LC,       48000, 2,  576000, audio::profile::AAC },
					{ LC,       48000, 6, 1440000, audio::profile::AAC_MULT5 },
					{ LC_ER,    48000, 2,  320000, audio::profile::AAC_320 },
					{ LC_ER,    48000, 2,  576000, audio::profile::AAC },
					{ LC_ER,    48000, 6, 1440000, audio::profile::AAC_MULT5 },
					{ LTP,      48000, 2,  576000, audio::profile::AAC_LTP },
					{ LTP,      96000, 6, 2880000, audio::profile::AAC_LTP_MULT5 },
					{ LTP,      96000, 8, 4032000, audio::profile::AAC_LTP_MULT7 },
					{ LTP_ER,   48000, 2,  576000, audio::profile::AAC_LTP },
					{ LTP_ER,   96000, 6, 2880000, audio::profile::AAC_LTP_MULT5 },
					{ LTP_ER,   96000, 8, 4032000, audio::profile::AAC_LTP_MULT7 },
					{ HE,       24000, 2,  320000, audio::profile::AAC_HE_L2_320 },
					{ HE,       24000, 2,  576000, audio::profile::AAC_HE_L2 },
					{ HE,       48000, 2,  576000, audio::profile::AAC_HE_L3 },
					{ HE,       48000, 6, 1440000, audio::profile::AAC_HE_MULT5 },
					{ HE_L3,    24000, 2,  320000, audio::profile::AAC_HE_L2_320 },
					{ HE_L3,    24000, 2,  576000, audio::profile::AAC_HE_L2 },
					{ HE_L3,    48000, 2,  576000, audio::profile::AAC_HE_L3 },
					{ HE_L3,    48000, 6, 1440000, audio::profile::AAC_HE_MULT5 },
					{ PARAM_ER, 24000, 2,  320000, audio::profile::AAC_HE_V2_L2_320 },
					{ PARAM_ER, 24000, 2,  576000, audio::profile::AAC_HE_V2_L2 },
					{ PARAM_ER, 48000, 2,  576000, audio::profile::AAC_HE_V2_L3 },
					{ PARAM_ER, 48000, 6, 1440000, audio::profile::AAC_HE_V2_MULT5 },
					{ BSAC_ER,  48000, 2,  128000, audio::profile::AAC_BSAC },
					{ BSAC_ER,  48000, 6,  128000, audio::profile::AAC_BSAC_MULT5 },
				};

				static audio::profile guess_aac_priv(AVCodecContext *ac, object_type type)
				{
					if (!ac)
						return audio::profile::INVALID;

					/* check for AAC variants codec */
					if (ac->codec_id != CODEC_ID_AAC)
						return audio::profile::INVALID;

					if (type == BSAC_ER && ac->sample_rate < 16000)
						return audio::profile::INVALID;
					if (ac->sample_rate < 8000)
						return audio::profile::INVALID;

					for (auto && priv : priv_profiles)
					{
						if (type != priv.m_type) continue;
						if (ac->sample_rate > priv.m_max_sample_rate) continue;
						if (ac->channels > priv.m_max_channels) continue;
						if (ac->bit_rate > priv.m_max_bit_rate) continue;
						return priv.m_profile;
					}

					return audio::profile::INVALID;
				}

				audio::profile guess_aac(AVCodecContext *ac)
				{
					if (!ac)
						return audio::profile::INVALID;

					auto type = object_type_get(ac->extradata, ac->extradata_size);
					return guess_aac_priv(ac, type);
				}

				static const Profile * probe(AVFormatContext *ctx, container::container_type container, const stream_codec& codecs)
				{
					audio::profile profile;
					container_type type = MUXED;

					if (!stream_is_audio(ctx, container, codecs))
						return nullptr;

					/* check for ADTS */
					if (container == dlna::container::AAC)
					{
						type = get_format(ctx);
						auto object = adts_object_type_get(ctx);
						profile = guess_aac_priv(codecs.m_audio.m_codec, object);
					}
					else
						profile = guess_aac(codecs.m_audio.m_codec);

					if (profile == audio::profile::INVALID)
						return nullptr;

					for (auto && map : mapping)
					{
						if (map.m_container == type && map.m_audio_profile == profile)
							return map.m_profile;
					}

					return nullptr;
				}
			}

			namespace mp3
			{
				static Profile mp3       { "MP3",  mime::AUDIO_MPEG, labels::AUDIO_2CH, Class::Audio };
				static Profile mp3x      { "MP3X", mime::AUDIO_MPEG, labels::AUDIO_2CH, Class::Audio };
				static Profile mp3_upnp  { "MP3",  mime::AUDIO_MPEG, labels::AUDIO_2CH, Class::Audio, mp3 };
				static Profile mp3x_upnp { "MP3X", mime::AUDIO_MPEG, labels::AUDIO_2CH, Class::Audio, mp3x };

				static bool is_valid_mp3_common(AVCodecContext *ac)
				{
					if (ac->codec_id != AV_CODEC_ID_MP3) return false;
					if (ac->channels > 2) return false;
					return true;
				}

				static bool is_valid_mp3_upnp(AVCodecContext *ac)
				{
					if (!ac)
						return false;

					if (!is_valid_mp3_common(ac))
						return false;

					if (ac->sample_rate != 32000 &&
						ac->sample_rate != 44100 &&
						ac->sample_rate != 48000)
						return false;

					return true;
				}

				static bool is_valid_mp3(AVCodecContext *ac)
				{
					if (!ac)
						return false;

					if (!is_valid_mp3_upnp(ac))
						return false;

					switch (ac->bit_rate)
					{
					case 32000:
					case 40000:
					case 48000:
					case 56000:
					case 64000:
					case 80000:
					case 96000:
					case 112000:
					case 128000:
					case 160000:
					case 192000:
					case 224000:
					case 256000:
					case 320000:
						return true;
					default:
						break;
					}

					return false;
				}

				static bool is_valid_mp3x_upnp(AVCodecContext *ac)
				{
					if (!ac)
						return false;

					if (!is_valid_mp3_common(ac))
						return false;

					if (ac->sample_rate != 16000 &&
						ac->sample_rate != 22050 &&
						ac->sample_rate != 24000)
						return false;

					return true;
				}

				static bool is_valid_mp3x(AVCodecContext *ac)
				{
					if (!ac)
						return false;

					if (!is_valid_mp3x_upnp(ac))
						return false;

					switch (ac->bit_rate)
					{
					case 8000:
					case 16000:
					case 24000:
					case 32000:
					case 40000:
					case 48000:
					case 56000:
					case 64000:
					case 80000:
					case 96000:
					case 112000:
					case 128000:
					case 160000:
					case 192000:
					case 224000:
					case 256000:
					case 320000:
						return true;
					default:
						break;
					}

					return false;
				}

				profile guess_mp3(AVCodecContext *ac)
				{
					if (!ac)
						return profile::INVALID;

					if (is_valid_mp3x(ac))
						return profile::MP3_EXTENDED;

					if (is_valid_mp3(ac))
						return profile::MP3;

					if (is_valid_mp3x_upnp(ac))
						return profile::MP3_EXTENDED_UPNP;

					if (is_valid_mp3_upnp(ac))
						return profile::MP3_UPNP;

					if (is_valid_mp3_common(ac))
						return profile::MP3_UPNP;

					return profile::INVALID;
				}

				static const Profile * probe(AVFormatContext *ctx, container::container_type container, const stream_codec& codecs)
				{
					if (!stream_is_audio(ctx, container, codecs))
						return nullptr;

					if (container != dlna::container::MP3)
						return nullptr;

					switch (guess_mp3(codecs.m_audio.m_codec))
					{
					case profile::MP3:
						return &mp3;
					case profile::MP3_EXTENDED:
						return &mp3x;
					case profile::MP3_UPNP:
						return &mp3_upnp;
					case profile::MP3_EXTENDED_UPNP:
						return &mp3x_upnp;
					default:
						break;
					}

					return nullptr;
				}
			}
		}

		void register_audio_profiles()
		{
			profile_db::register_profile(audio::aac::probe, "aac,adts,3gp,mp4,mov,qt,m4a");
			profile_db::register_profile(audio::mp3::probe, "mp3");
		}
	}
}
