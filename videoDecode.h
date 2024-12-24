#ifndef VIDEODOCODE_H
#define VIDEODOCODE_H

#include<QString>
#include<QSize>
struct AVFormatContext;//封装的结构体
struct AVRational;//时间戳
struct AVCodecContext;//解码上下文
struct AVPacket;//原始数据包
struct AVFrame;//处理后的数据
struct SwsContext;//图像转换上下文

class QImage;//前向声明出来即可，只需要返回值，防止重复包含

class videoDecode
{
public:
	videoDecode();
	~videoDecode();

	//声明一个open函数，把视频的封装格式剥去
	bool open(const QString& url = QString());
	//读取视频流
	QImage read();
	//关闭函数，实现资源释放和清理缓冲区
	void close();
	//获取显示帧时间
	const qint64& pts();
	//是否读取完成
	bool isEnd();
private:
	void errorHandle(int err);//处理err
	void free();//资源释放
	qreal rationalToDouble(AVRational* rational);//时间转换，减小误差
	void clear();//清理缓冲区
private:
	AVFormatContext* m_formatContext = NULL;//解封装上下文
	char* m_error = NULL;//异常信息缓冲区
	qint64 m_totalTime = 0;//视频总时长
	int m_videoIndex = 0;//视频流索引值
	QSize m_size;//视频图像分辨率
	qreal m_frameRate = 0;//视频帧率，默认为double
	qint64 m_totalFrames = 0;//视频总帧长
	qint64 m_obtainFrames = 0;//当前帧数

	AVCodecContext* m_codecContext = NULL;//解码器上下文
	AVPacket* m_packet = NULL;//原始数据包
	AVFrame* m_frame = NULL;//处理后的数据
	uchar* m_buffer = NULL;//图像缓冲区
	bool m_end = false;//视频读取完成标志位
	qint64 m_pts = 0;//图像帧的显示时间

	SwsContext* m_swsContext = NULL;//图像转换上下文
};

#endif // VIDEODOCODE_H
