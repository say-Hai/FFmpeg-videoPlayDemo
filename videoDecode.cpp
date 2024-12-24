#include "videoDecode.h"
#include<QDebug>
#include<QTime>
#include<QImage>
#include<qdatetime.h>
#include<QMutex>

/// 导入第三方ffmpeg库
extern  "C" {
#include"libavformat/avformat.h"
#include"libavcodec/avcodec.h"
#include"libavutil/avutil.h"
#include"libswscale/swscale.h"
#include"libavutil/imgutils.h"
}
#define ERROR_LEN 1024 //异常信息缓冲区
#define PRINT_LOG 1 //打印异常信息标志位，默认是debug

videoDecode::videoDecode()
{
	//初始化异常信息缓冲区
	m_error = new char[ERROR_LEN];
}

videoDecode::~videoDecode()
{
	//调用关闭函数
	close();
}

bool videoDecode::open(const QString& url)
{
	if (url.isNull())
	{
		qDebug() << "url is Empty ";
		return false;
	}
	AVDictionary* dict = NULL;
	av_dict_set(&dict, "rtsp_transport", "tcp", 0);
	av_dict_set(&dict, "max_delay", "3", 0);//设置最大延迟复用，禁止重新排序
	av_dict_set(&dict, "timeout", "1000000", 0);//设置套接字超时

	//打开输入流，并返回解封装上下文
	int ret = avformat_open_input(&m_formatContext,//保存解封装上下文
		url.toStdString().data(),//要打开的视频地址，要转换为char*类型
		NULL,//参数设置，自动选择解码器
		&dict);//参数字典里的参数传进来
	//释放参数字典
	if (dict)
	{
		av_dict_free(&dict);
	}
	//打开输入流失败
	if (ret < 0)
	{
#if PRINT_LOG
		errorHandle(ret);
#else
		//不管了
		Q_UNUSED(ret)
#endif
			free();
		return false;
	}
	ret = avformat_find_stream_info(m_formatContext, NULL);
	if (ret < 0)
	{
#if PRINT_LOG
		errorHandle(ret);
#else
		//不管了
		Q_UNUSED(ret)
#endif
			free();
		return false;
	}
	m_totalTime = m_formatContext->duration / (AV_TIME_BASE / 1000);
#if PRINT_LOG
	qDebug() << QString("视频总时长: %1 ms,[%2]").arg(m_totalTime).arg(QTime::fromMSecsSinceStartOfDay(int(m_totalTime)).toString("HH:mm:ss:zzz"));
#endif
	//信息流获取成功后，我们需要查找视频流ID
	//这里通过AVMediaType枚举查询视频流ID，当然也可以遍历查找
	m_videoIndex = av_find_best_stream(m_formatContext,
		AVMEDIA_TYPE_VIDEO,//媒体类型
		-1,//不指定流索引号，自动查找最佳的视频流
		-1,//不关联其他流，只考虑视频流本身
		NULL,//不需要返回找到的解码器
		0//不设置搜索标准位
	);
	//获取视频流ID失败
	if (m_videoIndex < 0)
	{
#if PRINT_LOG
		errorHandle(m_videoIndex);
#else
		//不管了
		Q_UNUSED(m_videoIndex)
#endif
			//不管有没有处理错误，返回前把资源先释放了
			free();
		return false;
	}
	//根据索引来获取视频流
	AVStream* videoStream = m_formatContext->streams[m_videoIndex];
	m_size.setWidth(videoStream->codecpar->width);
	m_size.setHeight(videoStream->codecpar->height);
	m_frameRate = rationalToDouble(&videoStream->avg_frame_rate);

	//获取解码器
	const AVCodec* codec = avcodec_find_decoder(videoStream->codecpar->codec_id);
	m_totalFrames = videoStream->nb_frames;
#if PRINT_LOG
	qDebug() << QString("分辨率：[w: %1,h: %2] 帧率: %3 总帧数: %4 解码器: %5")
		.arg(m_size.width()).arg(m_size.height())
		.arg(m_frameRate).arg(m_totalFrames)
		.arg(codec->name);
#endif
	m_codecContext = avcodec_alloc_context3(codec);
	if (!m_codecContext)
	{
#if PRINT_LOG
		qWarning() << "创建视频解码器上下文失败";
#else
		//不管了
		Q_UNUSED(m_codecContext)
#endif
			//不管有没有处理错误，返回前把资源先释放了
			free();
		return false;
	}
	ret = avcodec_parameters_to_context(m_codecContext, videoStream->codecpar);
	if (ret < 0)
	{
#if PRINT_LOG
		errorHandle(ret);
#else
		//不管了
		Q_UNUSED(ret)
#endif
			//不管有没有处理错误，返回前把资源先释放了
			free();
		return false;
	}
	//允许使用不符合规范的加速技巧
	m_codecContext->flags2 |= AV_CODEC_FLAG2_FAST;
	//使用8线程解码
	m_codecContext->thread_count = 8;

	ret = avcodec_open2(m_codecContext, NULL, NULL);
	if (ret < 0)
	{
#if PRINT_LOG
		errorHandle(ret);
#else
		//不管了
		Q_UNUSED(ret)
#endif
			//不管有没有处理错误，返回前把资源先释放了
			free();
		return false;
	}
	//给原始的数据包分配空间
	m_packet = av_packet_alloc();
	if (!m_packet)
	{
#if PRINT_LOG
		qWarning() << "av_packet_alloc() error";
#else
		//不管了
		Q_UNUSED(m_packet)
#endif
			//不管有没有处理错误，返回前把资源先释放了
			free();
		return false;
	}
	//给处理后的数据分配空间
	m_frame = av_frame_alloc();
	if (!m_frame)
	{
#if PRINT_LOG
		qWarning() << "av_frame_alloc() error";
#else
		//不管了
		Q_UNUSED(m_frame)
#endif
			//不管有没有处理错误，返回前把资源先释放了
			free();
		return false;
	}
	//分配图像空间
	//计算大小
	int size = av_image_get_buffer_size(AV_PIX_FMT_RGBA,//图像格式为RGBA
		m_size.width(),//图像宽度
		m_size.height(),//图像的高度
		4//每行像素的字节数
	);
	//多分配点图像空间
	m_buffer = new uchar[size + 1000];
	m_end = false;
	return true;
	//到此打开解码器，剥去封装格式，解析视频已经全部实现完，下面实现视频数据读取
}

QImage videoDecode::read()
{
	//没内容，返回一个空的，输出错误信息
	if (!m_formatContext)
	{
#if PRINT_LOG
		qWarning() << "read fomatContext is empty";
#endif
		return QImage();
	}
	//有东西，读取下一帧数据
	int readRet = av_read_frame(m_formatContext, m_packet);
	//处理数据读取完的情况
	if (readRet < 0)
	{
		//为什么还要发送空的数据包给解码器呢，因为要通知解码器进行最后一次解码
		//send_pack和read_frame的返回值可能不一致，read_frame返回AVERROR_EOF的时候，
		//读到文件末尾，不读数据了
		//但是不一定send_pack输出了所有的数据，再调用输出，同步数据,确保最后一帧或几帧数据能成功被传给解码器
		avcodec_send_packet(m_codecContext, m_packet);
	}
	else
	{
		//如果是图像数据(视频流)，就解码
		if (m_packet->stream_index == m_videoIndex)
		{
			//这个虽然有误差，但是适用性更强
		   //显示时间戳,帧在播出的时候该出现的时间,转为毫秒
			m_packet->pts = qRound64(m_packet->pts * (1000 * rationalToDouble(&m_formatContext->streams[m_videoIndex]->time_base)));
			//解码时间戳，帧在解码的时间。
			m_packet->dts = qRound64(m_packet->dts * (1000 * rationalToDouble(&m_formatContext->streams[m_videoIndex]->time_base)));

			//将读取到的原始数据帧传入解码器
			int ret = avcodec_send_packet(m_codecContext, m_packet);
			if (ret < 0)
			{
#if PRINT_LOG
				errorHandle(ret);
#endif
			}
		}
	}
	//要释放数据包
	av_packet_unref(m_packet);
	//处理解码后的数据
	//先接受
	int ret = avcodec_receive_frame(m_codecContext, m_frame);
	//失败
	if (ret < 0)
	{
#if PRINT_LOG
		errorHandle(ret);
#else
		Q_UNUSED(ret)
#endif
			av_frame_unref(m_frame);
		if (readRet < 0)
		{
			//也读不到数据了，代表读取完成了
			m_end = true;
		}
		return QImage();
	}
	m_pts = m_frame->pts;
	//处理图像转换上下文
	if (!m_swsContext)
	{
		/*
		 * 获取缓存区的图像转换上下文
		 * 首先校验参数是否一致
		 * 校验不通过释放资源
		 * 通过，判断上下文是否存在
		 * 存在，直接复用
		 * 不存在，分配新的，初始化
		*/
		m_swsContext = sws_getCachedContext(m_swsContext,
			m_frame->width,//输入图像的宽
			m_frame->height,//输入图像的高
			(AVPixelFormat)m_frame->format,//输入图像的像素格式
			m_size.width(),//输出图像的宽
			m_size.height(),//输出图像的高
			AV_PIX_FMT_RGBA,//输出图像的像素格式
			SWS_BILINEAR,//选择缩放算法
			NULL,//设置输入图像的滤波器信息
			NULL,//设置输出图像的滤波器信息
			NULL//设定缩放算法需要的参数
		);
		if (!m_swsContext)
		{
#if PRINT_LOG
			qWarning() << "sws_getCachedContext() error";
#endif
			free();
			return QImage();
		}
	}
	//将解码后的图像格式转换为QImage
	uchar* data[] = { m_buffer };
	int lines[4];
	//使用像素格式pix_fmt和宽度填充图像的平面线条大小
	av_image_fill_linesizes(lines, AV_PIX_FMT_RGBA, m_frame->width);
	//将原图像的大小和颜色空间转换为输出的图像格式
	ret = sws_scale(m_swsContext,//缩放上下文
		m_frame->data,//原图像数据
		m_frame->linesize,//包含原图像每个平面步幅的数组
		0,//开始位置
		m_frame->height,//行数
		data,//目标图像数组
		lines);//包含目标图像每个平面的步幅的数组
	if (ret < 0)
	{
#if PRINT_LOG
		errorHandle(ret);
#else
		Q_UNUSED(ret)
#endif
			free();
		return QImage();
	}
	QImage image(m_buffer,//图像数据的指针
		m_frame->width,//image的宽度
		m_frame->height,//image的高度
		QImage::Format_RGBA8888);//图像的像素格式
	av_frame_unref(m_frame);

	return image;

	//到此QImage格式的图像已经处理完毕，视频解码的主要功能已经实现完毕，下面主要是对现有资源的释放关闭
}

void videoDecode::close()
{
	//调用清空缓冲的函数
	clear();
	//调用资源释放函数
	free();

	//复位各种转态位
	m_totalTime = 0;
	m_videoIndex = 0;
	m_totalFrames = 0;
	m_obtainFrames = 0;
	m_pts = 0;
	m_frameRate = 0;
	m_size = QSize(0, 0);
}
//定义图像显示帧时间函数
const qint64& videoDecode::pts()
{
	return m_pts;
}
//定义判断视频是否读取完成的函数
bool videoDecode::isEnd()
{
	return m_end;
}

void videoDecode::errorHandle(int err)
{
#if PRINT_LOG
	memset(m_error, 0, ERROR_LEN);
	av_strerror(err, m_error, ERROR_LEN);
	//输出一般的警告信息
	qWarning() << "Video Docode error: " << err << m_error;
#else
	//不管了
	Q_UNUSED(err)
#endif
}

void videoDecode::free()
{
	//释放解封装上下文
	if (m_formatContext)
	{
		//关闭打开的流
		avformat_close_input(&m_formatContext);
	}
	//释放解编码器上下文
	if (m_codecContext)
	{
		avcodec_free_context(&m_codecContext);
	}
	//释放图像缩放上下文
	if (m_swsContext)
	{
		sws_freeContext(m_swsContext);
		m_swsContext = NULL;
	}
	//释放掉未解码的数据
	if (m_packet)
	{
		av_packet_free(&m_packet);
	}

	//释放解码后的数据
	if (m_frame)
	{
		av_frame_free(&m_frame);
	}

	//释放缓冲区
	if (m_buffer)
	{
		delete[] m_buffer;
		m_buffer = NULL;
	}
}

qreal videoDecode::rationalToDouble(AVRational* rational)
{
	qreal frameRate = (rational->den == 0) ? 0 : (qreal(rational->num) / rational->den);
	return frameRate;
}

void videoDecode::clear()
{//如果IO上下文非空，刷新IO缓冲区，确保所有数据都被写出
	if (m_formatContext && m_formatContext->pb)
	{
		avio_flush(m_formatContext->pb);
	}

	if (m_formatContext)
	{
		avformat_flush(m_formatContext);
	}
}