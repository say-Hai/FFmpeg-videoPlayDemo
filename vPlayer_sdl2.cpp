#include <stdio.h>
#include <iostream>

#include "output_log.h"
using namespace std;
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#define SDL_MAIN_HANDLED
#include <SDL.h>
}
//FFMPEG相关结构体
typedef struct FFmpeg_V_Param_T
{
	AVFormatContext* pFormatCtx;
	AVCodecContext* pCodecCtx;
	SwsContext* pSwsCtx;
	int video_index;
}FFmpeg_V_Param;

//SDL2相关结构体
typedef struct SDL_Param_T
{
	SDL_Window* p_sdl_window;
	SDL_Renderer* p_sdl_renderer;
	SDL_Texture* p_sdl_texture;
	SDL_Rect sdl_rect;
	SDL_Thread* p_sdl_thread;
}SDL_Param;

static int g_frame_rate = 1;
static int g_sfp_refresh_thread_exit = 0;
static int g_sfp_refresh_thread_pause = 0;
#define SFM_REFRESH_EVENT (SDL_USEREVENT+1)
#define SFM_BREAK_EVENT (SDL_USEREVENT+2)

///初始化ffmpeg相关结构体
int init_ffmpeg(FFmpeg_V_Param* p_ffmpeg_param, const char* filePath)
{
	//AVFormatContext初始化
	p_ffmpeg_param->pFormatCtx = avformat_alloc_context();
	//解码器
	const AVCodec* pcodec = NULL;
	//1.初始化网络
	//av_register_all()已被弃用
	avformat_network_init();
	//2.open输入流
	int ret = avformat_open_input(&(p_ffmpeg_param->pFormatCtx), filePath, NULL, NULL);
	if (ret < 0)
	{
		output_log(LOG_ERROR, "avformat_open_input error");
		return -1;
	}
	//3.读取媒体的数据包以获取具体的流信息，如媒体存入的编码格式。
	ret = avformat_find_stream_info(p_ffmpeg_param->pFormatCtx, NULL);
	if (ret < 0)
	{
		output_log(LOG_ERROR, "avformat_find_stream_info error");
		return -1;
	}
	//4.遍历 FFmpeg 中 AVFormatContext 的所有媒体流
	//get video pCodecParms, codec and frame rate
	//nb_streams 表示多媒体文件或流中包含的媒体流的数量
	for (int i = 0; i < p_ffmpeg_param->pFormatCtx->nb_streams; i++)
	{
		//4.1AVStream：存储音频流或视频流的结构体
		AVStream* pStream = p_ffmpeg_param->pFormatCtx->streams[i];
		if (pStream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			//4.2查找匹配解码器ID的已注的解码器
			pcodec = avcodec_find_decoder(pStream->codecpar->codec_id);
			//4.3 分配并初始化 AVCodecContext 结构体(参数为编解码器)
			p_ffmpeg_param->pCodecCtx = avcodec_alloc_context3(pcodec);
			avcodec_parameters_to_context(p_ffmpeg_param->pCodecCtx, pStream->codecpar);
			//4.4计算视频的帧率
			g_frame_rate = pStream->avg_frame_rate.num / pStream->avg_frame_rate.den;
			//流的索引
			p_ffmpeg_param->video_index = i;
		}
	}
	if (!p_ffmpeg_param->pCodecCtx)
	{
		output_log(LOG_ERROR, "could not find video codecCtx");
		return -1;
	}
	//5 通过给定的AVCodec来初始化一个视音频编解码器的 AVCodecContext
	ret = avcodec_open2(p_ffmpeg_param->pCodecCtx, pcodec, NULL);
	if (ret < 0)
	{
		output_log(LOG_ERROR, "avcodec_open2 error");
		return -1;
	}
	//6初始化一个缩放上下文 (SwsContext)，以便进行视频像素格式的转换或尺寸缩放。（这里是转成YUV420P）
	p_ffmpeg_param->pSwsCtx = sws_getContext(
		p_ffmpeg_param->pCodecCtx->width, p_ffmpeg_param->pCodecCtx->height, p_ffmpeg_param->pCodecCtx->pix_fmt,
		p_ffmpeg_param->pCodecCtx->width, p_ffmpeg_param->pCodecCtx->height, AV_PIX_FMT_YUV420P,
		SWS_BICUBIC, NULL, NULL, NULL);

	/*
	 *打印将媒体文件的格式和流信息
	av_dump_format(p_ffmpeg_param->pFormatCtx, p_ffmpeg_param->video_index, filePath, 0);
	*/
	return 0;
	//ffmpeg初始化全部完成
}

///释放都是调用对应的free、close函数
int release_ffmpeg(FFmpeg_V_Param* p_ffmpeg_param)
{
	if (!p_ffmpeg_param)
		return -1;
	//realse scale pixelformat context
	if (p_ffmpeg_param->pSwsCtx)
		sws_freeContext(p_ffmpeg_param->pSwsCtx);

	//close codec
	if (p_ffmpeg_param->pCodecCtx)
		avcodec_close(p_ffmpeg_param->pCodecCtx);

	//close input stream
	if (p_ffmpeg_param->pFormatCtx)
		avformat_close_input(&(p_ffmpeg_param->pFormatCtx));

	//free AVCodecContext
	if (p_ffmpeg_param->pCodecCtx)
		avcodec_free_context(&(p_ffmpeg_param->pCodecCtx));

	//free AVFormatContext
	if (p_ffmpeg_param->pFormatCtx)
		avformat_free_context(p_ffmpeg_param->pFormatCtx);

	//free FFmpeg_V_Param
	delete p_ffmpeg_param;
	p_ffmpeg_param = NULL;

	return 0;
}

///多线程刷新操作，暂时搁置；作用是通过定时推送 SDL 事件来控制视频播放
int sfp_refresh_thread(void* opaque)
{
	g_sfp_refresh_thread_exit = 0;
	g_sfp_refresh_thread_pause = 0;
	while (!g_sfp_refresh_thread_exit)
	{
		if (!g_sfp_refresh_thread_pause)
		{
			SDL_Event sdl_event;
			sdl_event.type = SFM_REFRESH_EVENT;
			SDL_PushEvent(&sdl_event);
		}
		SDL_Delay(1000 / g_frame_rate);
	}
	g_sfp_refresh_thread_exit = 0;
	g_sfp_refresh_thread_pause = 0;
	SDL_Event sdl_event;
	sdl_event.type = SFM_BREAK_EVENT;
	SDL_PushEvent(&sdl_event);
	return 0;
}

///初始化SDL2库的相关结构体
int init_sdl2(SDL_Param_T* p_sdl_param, int screen_w, int screen_h)
{
	if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_TIMER))
	{
		output_log(LOG_ERROR, "SDL_Init error");
		return -1;
	}
	p_sdl_param->p_sdl_window = SDL_CreateWindow("vPlayer_sdl", SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED, screen_w, screen_h, SDL_WINDOW_OPENGL);
	if (!p_sdl_param->p_sdl_window)
	{
		output_log(LOG_ERROR, "SDL_CreateWindow error");
		return -1;
	}
	p_sdl_param->p_sdl_renderer = SDL_CreateRenderer(p_sdl_param->p_sdl_window, -1, 0);
	p_sdl_param->p_sdl_texture = SDL_CreateTexture(p_sdl_param->p_sdl_renderer, SDL_PIXELFORMAT_IYUV,
		SDL_TEXTUREACCESS_STREAMING, screen_w, screen_h);
	p_sdl_param->sdl_rect.x = 0;
	p_sdl_param->sdl_rect.y = 0;
	p_sdl_param->sdl_rect.w = screen_w;
	p_sdl_param->sdl_rect.h = screen_h;
	p_sdl_param->p_sdl_thread = SDL_CreateThread(sfp_refresh_thread, NULL, NULL);

	return 0;
}

///释放库函数
int release_sdl2(SDL_Param_T* p_sdl_param)
{
	SDL_DestroyTexture(p_sdl_param->p_sdl_texture);
	SDL_DestroyRenderer(p_sdl_param->p_sdl_renderer);
	SDL_DestroyWindow(p_sdl_param->p_sdl_window);
	SDL_Quit();
	return 0;
}

int main()
{
	const char* filePath = "test.flv";
	FFmpeg_V_Param* p_ffmpeg_param = NULL;
	AVPacket* packet = NULL;
	AVFrame* pFrame = NULL;
	AVFrame* pFrameYUV = NULL;
	int out_buffer_size = 0;
	unsigned char* out_buffer = 0;
	//sdl param
	SDL_Param_T* p_sdl_param = NULL;
	SDL_Event sdl_event;

	p_ffmpeg_param = new FFmpeg_V_Param();
	memset(p_ffmpeg_param, 0, sizeof(FFmpeg_V_Param));
	init_ffmpeg(p_ffmpeg_param, filePath);

	packet = av_packet_alloc();
	pFrame = av_frame_alloc();
	pFrameYUV = av_frame_alloc();

	out_buffer_size = av_image_get_buffer_size(AV_PIX_FMT_YUV420P,
		p_ffmpeg_param->pCodecCtx->width, p_ffmpeg_param->pCodecCtx->height, 1);

	out_buffer = (unsigned char*)av_malloc(out_buffer_size);

	av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer,
		p_ffmpeg_param->pCodecCtx->pix_fmt,
		p_ffmpeg_param->pCodecCtx->width, p_ffmpeg_param->pCodecCtx->height, 1);

	//init sdl2
	p_sdl_param = new SDL_Param_T();
	memset(p_sdl_param, 0, sizeof(SDL_Param_T));
	init_sdl2(p_sdl_param, p_ffmpeg_param->pCodecCtx->width, p_ffmpeg_param->pCodecCtx->height);
	//至此，FFMPEG已将视频流的封装格式解开，并初始化好相关SDL2库，
		//接下来就是通过av_read_frame读取码流中的音频、视频一帧，
		//通过avcodec_send_packet发送到解码器解码，
		//再avcodec_receive_frame得到解码后的音视频帧AVFrame，
		//最后通过sws_scale转换格式，通过SDL相关API显示

	//demuxing and show
	while (true)
	{
		int temp_ret = 0;
		SDL_WaitEvent(&sdl_event);
		if (sdl_event.type == SFM_REFRESH_EVENT)
		{
			while (true)
			{
				if (av_read_frame(p_ffmpeg_param->pFormatCtx, packet) < 0)
				{
					g_sfp_refresh_thread_exit = 1;
					break;
				}
				if (packet->stream_index == p_ffmpeg_param->video_index)
				{
					break;
				}
			}
			if (avcodec_send_packet(p_ffmpeg_param->pCodecCtx, packet))
				g_sfp_refresh_thread_exit = 1;

			do
			{
				temp_ret = avcodec_receive_frame(p_ffmpeg_param->pCodecCtx, pFrame);
				if (temp_ret == AVERROR_EOF)
				{
					g_sfp_refresh_thread_exit = 1;
					break;
				}
				if (temp_ret == 0)
				{
					sws_scale(p_ffmpeg_param->pSwsCtx, (const unsigned char* const*)pFrame->data,
						pFrame->linesize, 0, p_ffmpeg_param->pCodecCtx->height, pFrameYUV->data,
						pFrameYUV->linesize);

					SDL_UpdateTexture(p_sdl_param->p_sdl_texture, &(p_sdl_param->sdl_rect),
						pFrameYUV->data[0], pFrameYUV->linesize[0]);

					SDL_RenderClear(p_sdl_param->p_sdl_renderer);
					SDL_RenderCopy(p_sdl_param->p_sdl_renderer, p_sdl_param->p_sdl_texture,
						NULL, &(p_sdl_param->sdl_rect));
					SDL_RenderPresent(p_sdl_param->p_sdl_renderer);
				}
			} while (temp_ret != AVERROR(EAGAIN));

			//av_packet_unref(packet);
		}
		else if (sdl_event.type == SFM_BREAK_EVENT)
		{
			break;
		}
		else if (sdl_event.type == SDL_KEYDOWN)
		{
			if (sdl_event.key.keysym.sym == SDLK_SPACE)
				g_sfp_refresh_thread_pause = !g_sfp_refresh_thread_pause;
			if (sdl_event.key.keysym.sym == SDLK_q)
				g_sfp_refresh_thread_exit = 1;
		}
		else if (sdl_event.type == SDL_QUIT)
		{
			g_sfp_refresh_thread_exit = 1;
		}
	}
	release_ffmpeg(p_ffmpeg_param);
	av_packet_free(&packet);
	av_frame_free(&pFrame);
	av_frame_free(&pFrameYUV);
	release_sdl2(p_sdl_param);
	return 0;
}