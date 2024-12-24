#include "videoDecode.h"
#include<QDebug>
#include<QTime>
#include<QImage>
#include<qdatetime.h>
#include<QMutex>

/// ���������ffmpeg��
extern  "C" {
#include"libavformat/avformat.h"
#include"libavcodec/avcodec.h"
#include"libavutil/avutil.h"
#include"libswscale/swscale.h"
#include"libavutil/imgutils.h"
}
#define ERROR_LEN 1024 //�쳣��Ϣ������
#define PRINT_LOG 1 //��ӡ�쳣��Ϣ��־λ��Ĭ����debug

videoDecode::videoDecode()
{
	//��ʼ���쳣��Ϣ������
	m_error = new char[ERROR_LEN];
}

videoDecode::~videoDecode()
{
	//���ùرպ���
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
	av_dict_set(&dict, "max_delay", "3", 0);//��������ӳٸ��ã���ֹ��������
	av_dict_set(&dict, "timeout", "1000000", 0);//�����׽��ֳ�ʱ

	//���������������ؽ��װ������
	int ret = avformat_open_input(&m_formatContext,//������װ������
		url.toStdString().data(),//Ҫ�򿪵���Ƶ��ַ��Ҫת��Ϊchar*����
		NULL,//�������ã��Զ�ѡ�������
		&dict);//�����ֵ���Ĳ���������
	//�ͷŲ����ֵ�
	if (dict)
	{
		av_dict_free(&dict);
	}
	//��������ʧ��
	if (ret < 0)
	{
#if PRINT_LOG
		errorHandle(ret);
#else
		//������
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
		//������
		Q_UNUSED(ret)
#endif
			free();
		return false;
	}
	m_totalTime = m_formatContext->duration / (AV_TIME_BASE / 1000);
#if PRINT_LOG
	qDebug() << QString("��Ƶ��ʱ��: %1 ms,[%2]").arg(m_totalTime).arg(QTime::fromMSecsSinceStartOfDay(int(m_totalTime)).toString("HH:mm:ss:zzz"));
#endif
	//��Ϣ����ȡ�ɹ���������Ҫ������Ƶ��ID
	//����ͨ��AVMediaTypeö�ٲ�ѯ��Ƶ��ID����ȻҲ���Ա�������
	m_videoIndex = av_find_best_stream(m_formatContext,
		AVMEDIA_TYPE_VIDEO,//ý������
		-1,//��ָ���������ţ��Զ�������ѵ���Ƶ��
		-1,//��������������ֻ������Ƶ������
		NULL,//����Ҫ�����ҵ��Ľ�����
		0//������������׼λ
	);
	//��ȡ��Ƶ��IDʧ��
	if (m_videoIndex < 0)
	{
#if PRINT_LOG
		errorHandle(m_videoIndex);
#else
		//������
		Q_UNUSED(m_videoIndex)
#endif
			//������û�д�����󣬷���ǰ����Դ���ͷ���
			free();
		return false;
	}
	//������������ȡ��Ƶ��
	AVStream* videoStream = m_formatContext->streams[m_videoIndex];
	m_size.setWidth(videoStream->codecpar->width);
	m_size.setHeight(videoStream->codecpar->height);
	m_frameRate = rationalToDouble(&videoStream->avg_frame_rate);

	//��ȡ������
	const AVCodec* codec = avcodec_find_decoder(videoStream->codecpar->codec_id);
	m_totalFrames = videoStream->nb_frames;
#if PRINT_LOG
	qDebug() << QString("�ֱ��ʣ�[w: %1,h: %2] ֡��: %3 ��֡��: %4 ������: %5")
		.arg(m_size.width()).arg(m_size.height())
		.arg(m_frameRate).arg(m_totalFrames)
		.arg(codec->name);
#endif
	m_codecContext = avcodec_alloc_context3(codec);
	if (!m_codecContext)
	{
#if PRINT_LOG
		qWarning() << "������Ƶ������������ʧ��";
#else
		//������
		Q_UNUSED(m_codecContext)
#endif
			//������û�д�����󣬷���ǰ����Դ���ͷ���
			free();
		return false;
	}
	ret = avcodec_parameters_to_context(m_codecContext, videoStream->codecpar);
	if (ret < 0)
	{
#if PRINT_LOG
		errorHandle(ret);
#else
		//������
		Q_UNUSED(ret)
#endif
			//������û�д�����󣬷���ǰ����Դ���ͷ���
			free();
		return false;
	}
	//����ʹ�ò����Ϲ淶�ļ��ټ���
	m_codecContext->flags2 |= AV_CODEC_FLAG2_FAST;
	//ʹ��8�߳̽���
	m_codecContext->thread_count = 8;

	ret = avcodec_open2(m_codecContext, NULL, NULL);
	if (ret < 0)
	{
#if PRINT_LOG
		errorHandle(ret);
#else
		//������
		Q_UNUSED(ret)
#endif
			//������û�д�����󣬷���ǰ����Դ���ͷ���
			free();
		return false;
	}
	//��ԭʼ�����ݰ�����ռ�
	m_packet = av_packet_alloc();
	if (!m_packet)
	{
#if PRINT_LOG
		qWarning() << "av_packet_alloc() error";
#else
		//������
		Q_UNUSED(m_packet)
#endif
			//������û�д�����󣬷���ǰ����Դ���ͷ���
			free();
		return false;
	}
	//�����������ݷ���ռ�
	m_frame = av_frame_alloc();
	if (!m_frame)
	{
#if PRINT_LOG
		qWarning() << "av_frame_alloc() error";
#else
		//������
		Q_UNUSED(m_frame)
#endif
			//������û�д�����󣬷���ǰ����Դ���ͷ���
			free();
		return false;
	}
	//����ͼ��ռ�
	//�����С
	int size = av_image_get_buffer_size(AV_PIX_FMT_RGBA,//ͼ���ʽΪRGBA
		m_size.width(),//ͼ����
		m_size.height(),//ͼ��ĸ߶�
		4//ÿ�����ص��ֽ���
	);
	//������ͼ��ռ�
	m_buffer = new uchar[size + 1000];
	m_end = false;
	return true;
	//���˴򿪽���������ȥ��װ��ʽ��������Ƶ�Ѿ�ȫ��ʵ���꣬����ʵ����Ƶ���ݶ�ȡ
}

QImage videoDecode::read()
{
	//û���ݣ�����һ���յģ����������Ϣ
	if (!m_formatContext)
	{
#if PRINT_LOG
		qWarning() << "read fomatContext is empty";
#endif
		return QImage();
	}
	//�ж�������ȡ��һ֡����
	int readRet = av_read_frame(m_formatContext, m_packet);
	//�������ݶ�ȡ������
	if (readRet < 0)
	{
		//Ϊʲô��Ҫ���Ϳյ����ݰ����������أ���ΪҪ֪ͨ�������������һ�ν���
		//send_pack��read_frame�ķ���ֵ���ܲ�һ�£�read_frame����AVERROR_EOF��ʱ��
		//�����ļ�ĩβ������������
		//���ǲ�һ��send_pack��������е����ݣ��ٵ��������ͬ������,ȷ�����һ֡��֡�����ܳɹ�������������
		avcodec_send_packet(m_codecContext, m_packet);
	}
	else
	{
		//�����ͼ������(��Ƶ��)���ͽ���
		if (m_packet->stream_index == m_videoIndex)
		{
			//�����Ȼ�������������Ը�ǿ
		   //��ʾʱ���,֡�ڲ�����ʱ��ó��ֵ�ʱ��,תΪ����
			m_packet->pts = qRound64(m_packet->pts * (1000 * rationalToDouble(&m_formatContext->streams[m_videoIndex]->time_base)));
			//����ʱ�����֡�ڽ����ʱ�䡣
			m_packet->dts = qRound64(m_packet->dts * (1000 * rationalToDouble(&m_formatContext->streams[m_videoIndex]->time_base)));

			//����ȡ����ԭʼ����֡���������
			int ret = avcodec_send_packet(m_codecContext, m_packet);
			if (ret < 0)
			{
#if PRINT_LOG
				errorHandle(ret);
#endif
			}
		}
	}
	//Ҫ�ͷ����ݰ�
	av_packet_unref(m_packet);
	//�������������
	//�Ƚ���
	int ret = avcodec_receive_frame(m_codecContext, m_frame);
	//ʧ��
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
			//Ҳ�����������ˣ������ȡ�����
			m_end = true;
		}
		return QImage();
	}
	m_pts = m_frame->pts;
	//����ͼ��ת��������
	if (!m_swsContext)
	{
		/*
		 * ��ȡ��������ͼ��ת��������
		 * ����У������Ƿ�һ��
		 * У�鲻ͨ���ͷ���Դ
		 * ͨ�����ж��������Ƿ����
		 * ���ڣ�ֱ�Ӹ���
		 * �����ڣ������µģ���ʼ��
		*/
		m_swsContext = sws_getCachedContext(m_swsContext,
			m_frame->width,//����ͼ��Ŀ�
			m_frame->height,//����ͼ��ĸ�
			(AVPixelFormat)m_frame->format,//����ͼ������ظ�ʽ
			m_size.width(),//���ͼ��Ŀ�
			m_size.height(),//���ͼ��ĸ�
			AV_PIX_FMT_RGBA,//���ͼ������ظ�ʽ
			SWS_BILINEAR,//ѡ�������㷨
			NULL,//��������ͼ����˲�����Ϣ
			NULL,//�������ͼ����˲�����Ϣ
			NULL//�趨�����㷨��Ҫ�Ĳ���
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
	//��������ͼ���ʽת��ΪQImage
	uchar* data[] = { m_buffer };
	int lines[4];
	//ʹ�����ظ�ʽpix_fmt�Ϳ�����ͼ���ƽ��������С
	av_image_fill_linesizes(lines, AV_PIX_FMT_RGBA, m_frame->width);
	//��ԭͼ��Ĵ�С����ɫ�ռ�ת��Ϊ�����ͼ���ʽ
	ret = sws_scale(m_swsContext,//����������
		m_frame->data,//ԭͼ������
		m_frame->linesize,//����ԭͼ��ÿ��ƽ�沽��������
		0,//��ʼλ��
		m_frame->height,//����
		data,//Ŀ��ͼ������
		lines);//����Ŀ��ͼ��ÿ��ƽ��Ĳ���������
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
	QImage image(m_buffer,//ͼ�����ݵ�ָ��
		m_frame->width,//image�Ŀ��
		m_frame->height,//image�ĸ߶�
		QImage::Format_RGBA8888);//ͼ������ظ�ʽ
	av_frame_unref(m_frame);

	return image;

	//����QImage��ʽ��ͼ���Ѿ�������ϣ���Ƶ�������Ҫ�����Ѿ�ʵ����ϣ�������Ҫ�Ƕ�������Դ���ͷŹر�
}

void videoDecode::close()
{
	//������ջ���ĺ���
	clear();
	//������Դ�ͷź���
	free();

	//��λ����ת̬λ
	m_totalTime = 0;
	m_videoIndex = 0;
	m_totalFrames = 0;
	m_obtainFrames = 0;
	m_pts = 0;
	m_frameRate = 0;
	m_size = QSize(0, 0);
}
//����ͼ����ʾ֡ʱ�亯��
const qint64& videoDecode::pts()
{
	return m_pts;
}
//�����ж���Ƶ�Ƿ��ȡ��ɵĺ���
bool videoDecode::isEnd()
{
	return m_end;
}

void videoDecode::errorHandle(int err)
{
#if PRINT_LOG
	memset(m_error, 0, ERROR_LEN);
	av_strerror(err, m_error, ERROR_LEN);
	//���һ��ľ�����Ϣ
	qWarning() << "Video Docode error: " << err << m_error;
#else
	//������
	Q_UNUSED(err)
#endif
}

void videoDecode::free()
{
	//�ͷŽ��װ������
	if (m_formatContext)
	{
		//�رմ򿪵���
		avformat_close_input(&m_formatContext);
	}
	//�ͷŽ������������
	if (m_codecContext)
	{
		avcodec_free_context(&m_codecContext);
	}
	//�ͷ�ͼ������������
	if (m_swsContext)
	{
		sws_freeContext(m_swsContext);
		m_swsContext = NULL;
	}
	//�ͷŵ�δ���������
	if (m_packet)
	{
		av_packet_free(&m_packet);
	}

	//�ͷŽ���������
	if (m_frame)
	{
		av_frame_free(&m_frame);
	}

	//�ͷŻ�����
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
{//���IO�����ķǿգ�ˢ��IO��������ȷ���������ݶ���д��
	if (m_formatContext && m_formatContext->pb)
	{
		avio_flush(m_formatContext->pb);
	}

	if (m_formatContext)
	{
		avformat_flush(m_formatContext);
	}
}