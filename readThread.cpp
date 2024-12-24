#include "readThread.h"
#include"videoDecode.h"
#include<QDebug>
#include<qimage.h>
#include<QEventLoop>
#include<QTimer>
#define LOG 1

readThread::readThread(QObject* parent) : QThread(parent)
{
	//���߳�������Ҫ��ʵ������Ƶ����Ķ���
	m_videoDecode = new videoDecode();
	//Qt ������ע���Զ����������͵ĺ������á��������Զ����������źźͲۻ����б�ʹ�ã��������ڿ��߳�ͨ��ʱ��
	//�����û���������ͣ��� PlayState������Ҫ��ʽע�����ǣ�������ȷ�����źźͲ�֮�䴫�ݡ�
	qRegisterMetaType<PlayState>("PlayState");
}

readThread::~readThread()
{
	//�ͷ���Ƶ�������
	if (m_videoDecode)
	{
		delete m_videoDecode;
	}
}
/// �����̴߳򿪺���,��Ҫ�ⲿ����,�����ò������߳�
void readThread::open(const QString& url)
{
	if (!this->isRunning())
	{
		m_url = url;
		qDebug() << "open video url: " << m_url;
		//�����̵߳�run����
		this->start();
	}
}

void readThread::close()
{
	m_pause = false;
	m_play = false;
}

void readThread::pause(bool flag)
{
	m_pause = flag;
}

//�����ȡ����Ƶ��ַ�ĺ���
const QString& readThread::url()
{
	return m_url;
}

void readThread::run()
{
	//���ȵ���open����,��ʼ��Ƶ����
	bool ret = m_videoDecode->open(m_url);
	if (ret)
	{
		//��Ƶ����ɹ�
		//���ò��ű�־λΪ��
		m_play = true;
		//�Ե�ǰ�̵߳�ʱ��Ϊ��㣬����ʱ��
		m_etime2.start();
		//�������̷߳�����Ƶ״̬��Ϊplay���ź�
		emit playState(play);
	}
	else
	{
#if LOG
		qDebug() << "read thread open err";
		qWarning() << "��ʧ��";
#else
		Q_UNUSED(ret)
#endif
	}
	while (m_play)
	{
		while (m_pause)
		{
			sleepMesc(200);
		}
		QImage image = m_videoDecode->read();
		if (!image.isNull())
		{
			sleepMesc(int(m_videoDecode->pts() - m_etime2.elapsed()));
			emit(updateImage(image));
		}
		else
		{
			if (m_videoDecode->isEnd())
			{
				qDebug() << "read Thread over";
				break;
			}
			sleepMesc(1);
		}
	}

	//ȫ��������
	qDebug() << "���Ž���";
	//�ص���Ƶ����
	m_videoDecode->close();
	//������Ƶ��������ź�
	emit playState(end);
	//�������Ƶ�����̵߳���Ҫ�߼��Ѿ�ʵ�����
}

//��������ʱʵ��
//ʵ�־���ָ��ʱ���˳������������̵߳�ִ��
void sleepMesc(int msec)
{
	//����ʱ��ֱ���˳�
	if (msec <= 0)   return;
	//����һ���µ��¼�ѭ��,����ʱ����ʱ�¼�
	QEventLoop loop;
	//����һ����ʱ�����ۺ���Ϊ�̵߳��˳�
	//��ʱ�������źţ���Ƶ�����߳��˳�
	QTimer::singleShot(msec, &loop, SLOT(quit()));
	//�¼�ѭ����ʼִ�У���Ƶ�����߳�����������
	//��ʱ���߳��˳�
	loop.exec();
}