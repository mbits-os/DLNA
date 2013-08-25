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
		namespace image
		{
			namespace jpeg
			{
				static Profile jpeg_sm      { "JPEG_SM",      mime::IMAGE_JPEG, labels::IMAGE_PICTURE, Class::Image };
				static Profile jpeg_med     { "JPEG_MED",     mime::IMAGE_JPEG, labels::IMAGE_PICTURE, Class::Image };
				static Profile jpeg_lrg     { "JPEG_LRG",     mime::IMAGE_JPEG, labels::IMAGE_PICTURE, Class::Image };
				static Profile jpeg_tn      { "JPEG_TN",      mime::IMAGE_JPEG, labels::IMAGE__ICON,   Class::Image };
				static Profile jpeg_sm_ico  { "JPEG_SM_ICO",  mime::IMAGE_JPEG, labels::IMAGE__ICON,   Class::Image };
				static Profile jpeg_lrg_ico { "JPEG_LRG_ICO", mime::IMAGE_JPEG, labels::IMAGE__ICON,   Class::Image };

				static const boundary boundaries [] =
				{
					{ &jpeg_sm_ico,    48,   48 },
					{ &jpeg_lrg_ico,  120,  120 },
					{ &jpeg_tn,       160,  160 },
					{ &jpeg_sm,       640,  480 },
					{ &jpeg_med,     1024,  768 },
					{ &jpeg_lrg,     4096, 4096 }
				};

				struct module : image_module<module, AV_CODEC_ID_MJPEG, AV_CODEC_ID_MJPEGB, AV_CODEC_ID_LJPEG, AV_CODEC_ID_JPEGLS>
				{
					static const Profile * probe(AVFormatContext *ctx, container::container_type container, const stream_codec& codecs)
					{
						return profiles(boundaries, ctx, container, codecs);
					}
				};
			}

			namespace png
			{
				static Profile png_tn       { "PNG_TN",       mime::IMAGE_PNG, labels::IMAGE__ICON,   Class::Image };
				static Profile png_sm_ico   { "PNG_SM_ICO",   mime::IMAGE_PNG, labels::IMAGE__ICON,   Class::Image };
				static Profile png_lrg_ico  { "PNG_LRG_ICO",  mime::IMAGE_PNG, labels::IMAGE__ICON,   Class::Image };
				static Profile png_lrg      { "PNG_LRG",      mime::IMAGE_PNG, labels::IMAGE_PICTURE, Class::Image };

				static const boundary boundaries [] =
				{
					{ &png_sm_ico,   48,   48 },
					{ &png_lrg_ico, 120,  120 },
					{ &png_tn,      160,  160 },
					{ &png_lrg,    4096, 4096 }
				};

				struct module : image_module<module, AV_CODEC_ID_PNG>
				{
					static const Profile * probe(AVFormatContext *ctx, container::container_type container, const stream_codec& codecs)
					{
						return profiles(boundaries, ctx, container, codecs);
					}
				};
			}

			namespace gif
			{
				static Profile gif_sm_ico  { "GIF_SM_ICO",  mime::IMAGE_GIF, labels::IMAGE__ICON,   Class::Image };
				static Profile gif_lrg_ico { "GIF_LRG_ICO", mime::IMAGE_GIF, labels::IMAGE__ICON,   Class::Image };
				static Profile gif_tn      { "GIF_TN",      mime::IMAGE_GIF, labels::IMAGE__ICON,   Class::Image };
				static Profile gif_lrg     { "GIF_LRG",     mime::IMAGE_GIF, labels::IMAGE_PICTURE, Class::Image };

				static const boundary boundaries [] =
				{
					{ &gif_sm_ico,    48,   48 },
					{ &gif_lrg_ico,  120,  120 },
					{ &gif_tn,       160,  160 },
					{ &gif_lrg,     4096, 4096 }
				};

				struct module : image_module<module, AV_CODEC_ID_GIF>
				{
					static const Profile * probe(AVFormatContext *ctx, container::container_type container, const stream_codec& codecs)
					{
						return profiles(boundaries, ctx, container, codecs);
					}
				};
			}

			namespace bmp
			{
				static Profile bmp_sm_ico  { "BMP_SM_ICO",  mime::IMAGE_BMP, labels::IMAGE__ICON,   Class::Image };
				static Profile bmp_lrg_ico { "BMP_LRG_ICO", mime::IMAGE_BMP, labels::IMAGE__ICON,   Class::Image };
				static Profile bmp_tn      { "BMP_TN",      mime::IMAGE_BMP, labels::IMAGE__ICON,   Class::Image };
				static Profile bmp_lrg     { "BMP_LRG",     mime::IMAGE_BMP, labels::IMAGE_PICTURE, Class::Image };

				static const boundary boundaries [] =
				{
					{ &bmp_sm_ico,    48,   48 },
					{ &bmp_lrg_ico,  120,  120 },
					{ &bmp_tn,       160,  160 },
					{ &bmp_lrg,     4096, 4096 }
				};

				struct module : image_module<module, AV_CODEC_ID_BMP>
				{
					static const Profile * probe(AVFormatContext *ctx, container::container_type container, const stream_codec& codecs)
					{
						return profiles(boundaries, ctx, container, codecs);
					}
				};
			}
		}

		void register_image_profiles()
		{
			image::jpeg::module::register_profiles("jpg,jpe,jpeg");
			image::png::module::register_profiles("png");
			image::gif::module::register_profiles("gif");
			image::bmp::module::register_profiles("bmp");
		}
	}
}
