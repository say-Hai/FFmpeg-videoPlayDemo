#ifndef VIDEODOCODE_H
#define VIDEODOCODE_H

#include<QString>
#include<QSize>
struct AVFormatContext;//��װ�Ľṹ��
struct AVRational;//ʱ���
struct AVCodecContext;//����������
struct AVPacket;//ԭʼ���ݰ�
struct AVFrame;//����������
struct SwsContext;//ͼ��ת��������

class QImage;//ǰ�������������ɣ�ֻ��Ҫ����ֵ����ֹ�ظ�����

class videoDecode
{
public:
	videoDecode();
	~videoDecode();

	//����һ��open����������Ƶ�ķ�װ��ʽ��ȥ
	bool open(const QString& url = QString());
	//��ȡ��Ƶ��
	QImage read();
	//�رպ�����ʵ����Դ�ͷź���������
	void close();
	//��ȡ��ʾ֡ʱ��
	const qint64& pts();
	//�Ƿ��ȡ���
	bool isEnd();
private:
	void errorHandle(int err);//����err
	void free();//��Դ�ͷ�
	qreal rationalToDouble(AVRational* rational);//ʱ��ת������С���
	void clear();//��������
private:
	AVFormatContext* m_formatContext = NULL;//���װ������
	char* m_error = NULL;//�쳣��Ϣ������
	qint64 m_totalTime = 0;//��Ƶ��ʱ��
	int m_videoIndex = 0;//��Ƶ������ֵ
	QSize m_size;//��Ƶͼ��ֱ���
	qreal m_frameRate = 0;//��Ƶ֡�ʣ�Ĭ��Ϊdouble
	qint64 m_totalFrames = 0;//��Ƶ��֡��
	qint64 m_obtainFrames = 0;//��ǰ֡��

	AVCodecContext* m_codecContext = NULL;//������������
	AVPacket* m_packet = NULL;//ԭʼ���ݰ�
	AVFrame* m_frame = NULL;//����������
	uchar* m_buffer = NULL;//ͼ�񻺳���
	bool m_end = false;//��Ƶ��ȡ��ɱ�־λ
	qint64 m_pts = 0;//ͼ��֡����ʾʱ��

	SwsContext* m_swsContext = NULL;//ͼ��ת��������
};

#endif // VIDEODOCODE_H
