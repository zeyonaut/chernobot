#pragma once
#include "streamref.hpp"

#include <iostream>

extern "C"
{
#include <libavdevice/avdevice.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}


namespace advd
{
	class Videostream
	{
		bool m_is_open;

		AVFormatContext *format_ctx;
		int stream_index;

		AVFrame *raw_frame, *converted_frame;

		AVCodecContext *codec_ctx;
		SwsContext *conv_ctx;
		int frame_bytes; // Some of these three are probably unnecessary and can be made local. Do an investigation later.
		unsigned char *frame_buffer;

		Videostream (const Videostream &other) = delete;
		Videostream &operator= (Videostream other) = delete;

		void open_unchecked(const StreamRef &stream_ref)
		{

			avcodec_register_all();

			format_ctx = avformat_alloc_context();

			#if defined (__APPLE__)
			AVInputFormat *format = av_find_input_format("avfoundation"); // macOS only
			#elif defined (__LINUX__)
			AVInputFormat *format = av_find_input_format("l4v2"); // Linux
			#endif

			AVDictionary* dictionary = NULL;
			av_dict_set(&dictionary, "framerate", "25", 0); // Ad hoc for specific REDGO converter because open_input fails if wrong framerate given on macOS.
			// TODO: query device options and then give users the option to change them.

			assert(avformat_open_input(&format_ctx, stream_ref.label().c_str(), format, &dictionary) == 0);
			assert(avformat_find_stream_info(format_ctx, NULL) >= 0);

			av_dump_format(format_ctx, 0, stream_ref.label().c_str(), 0); // Debug only?

			stream_index = -1;
			for (int i = 0; i < format_ctx->nb_streams; i++)
			{	   
				if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
				{	   
					stream_index = i;
					break;
				}
			}
			assert(stream_index != -1);

			codec_ctx = avcodec_alloc_context3(NULL); // TODO free or something
			assert(avcodec_parameters_to_context(codec_ctx, format_ctx->streams[stream_index]->codecpar) >= 0);
			
			const AVCodec* codec = avcodec_find_decoder(codec_ctx->codec_id);
			assert(avcodec_open2(codec_ctx, codec, NULL) >= 0);

			raw_frame = av_frame_alloc();
			converted_frame = av_frame_alloc();

			frame_bytes = avpicture_get_size(AV_PIX_FMT_RGB24, codec_ctx->width, codec_ctx->height);
			frame_buffer = (unsigned char *) malloc(frame_bytes * sizeof(unsigned char));

			avpicture_fill((AVPicture *) converted_frame, frame_buffer, AV_PIX_FMT_RGB24, codec_ctx->width, codec_ctx->height);
			
			conv_ctx = sws_getContext(codec_ctx->width,
			codec_ctx->height, codec_ctx->pix_fmt, codec_ctx->width,
			codec_ctx->height, AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL,
			NULL, NULL);
			assert(conv_ctx != NULL);
		}

	public:
		bool is_open () {return m_is_open;}

		Videostream (): m_is_open(false) {}

		Videostream (const StreamRef &stream_ref): m_is_open(true)
		{
			open_unchecked(stream_ref);
		}

		~Videostream ()
		{
			if (m_is_open) avformat_close_input(&format_ctx);
		}

		void open (const StreamRef &stream_ref)
		{
			if (m_is_open) avformat_close_input(&format_ctx);

			open_unchecked(stream_ref);
			m_is_open = true;
		}

		void close ()
		{
			if (m_is_open) avformat_close_input(&format_ctx);
			m_is_open = false;
		}

		TextureData current_frame ()
		{
			AVPacket current_packet;
			av_init_packet(&current_packet);

			int is_frame_finished = 0;
			if (av_read_frame(format_ctx, &current_packet) == 0)
			{
				if (current_packet.stream_index == stream_index)
				{
					assert(codec_ctx != NULL);
					assert(raw_frame != NULL);
					avcodec_decode_video2(codec_ctx, raw_frame, &is_frame_finished, &current_packet);

					if(is_frame_finished)
					{
						sws_scale(conv_ctx, raw_frame->data, raw_frame->linesize, 0, codec_ctx->height, converted_frame->data, converted_frame->linesize);
						auto t = TextureData {codec_ctx->width, codec_ctx->height, (unsigned char *) converted_frame->data[0]}; //TODO - this is not good. You have to copy the data - so find out how large it is in the docs.
						av_packet_unref(&current_packet);
						return t;
					}
				}
			}
			
			av_packet_unref(&current_packet);
			close();
			assert(false); // Return none/unexpected.
		}
	};
}
