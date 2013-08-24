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
			namespace mp3
			{
				static Profile mp3  { "MP3",  mime::AUDIO_MPEG, labels::AUDIO_2CH, Class::Audio };
				static Profile mp3x { "MP3X", mime::AUDIO_MPEG, labels::AUDIO_2CH, Class::Audio };

				static bool is_valid_mp3_common(AVCodecContext *ac)
				{
					if (ac->codec_id != CODEC_ID_MP3) return false;
					if (ac->channels > 2) return false;
					return true;
				}

				static bool is_valid_mp3(AVCodecContext *ac)
				{
					if (!ac)
						return false;

					if (!is_valid_mp3_common(ac))
						return false;

					if (ac->sample_rate != 32000 &&
						ac->sample_rate != 44100 &&
						ac->sample_rate != 48000)
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

				static bool is_valid_mp3x(AVCodecContext *ac)
				{
					if (!ac)
						return false;

					if (!is_valid_mp3_common(ac))
						return false;

					if (ac->sample_rate != 16000 &&
						ac->sample_rate != 22050 &&
						ac->sample_rate != 24000)
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
					default:
						break;
					}

					return NULL;
				}
			}
		}

		void register_audio_profiles()
		{
			profile_db::register_profile(audio::mp3::probe, "mp3");
		}
	}
}
