#include "readThread.h"
#include"videoDecode.h"
#include<QDebug>
#include<qimage.h>
#include<QEventLoop>
#include<QTimer>
#define LOG 1

readThread::readThread(QObject* parent) : QThread(parent)
{
	//在线程类中需要先实例化视频解码的对象
	m_videoDecode = new videoDecode();
	//Qt 中用于注册自定义数据类型的函数调用。它允许自定义类型在信号和槽机制中被使用，尤其是在跨线程通信时。
	//对于用户定义的类型（如 PlayState），需要显式注册它们，才能正确地在信号和槽之间传递。
	qRegisterMetaType<PlayState>("PlayState");
}

readThread::~readThread()
{
	//释放视频解码对象
	if (m_videoDecode)
	{
		delete m_videoDecode;
	}
}
/// 定义线程打开函数,需要外部调用,不调用不开启线程
void readThread::open(const QString& url)
{
	if (!this->isRunning())
	{
		m_url = url;
		qDebug() << "open video url: " << m_url;
		//启动线程的run函数
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

//定义获取打开视频地址的函数
const QString& readThread::url()
{
	return m_url;
}

void readThread::run()
{
	//首先调用open函数,开始视频解码
	bool ret = m_videoDecode->open(m_url);
	if (ret)
	{
		//视频解码成功
		//设置播放标志位为真
		m_play = true;
		//以当前线程的时间为起点，计算时间
		m_etime2.start();
		//给窗口线程发送视频状态变为play的信号
		emit playState(play);
	}
	else
	{
#if LOG
		qDebug() << "read thread open err";
		qWarning() << "打开失败";
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

	//全部搞完了
	qDebug() << "播放结束";
	//关掉视频解码
	m_videoDecode->close();
	//发送视频播放完的信号
	emit playState(end);
	//到这里，视频解码线程的主要逻辑已经实现完毕
}

//非阻塞延时实现
//实现经过指定时间退出，不阻塞主线程的执行
void sleepMesc(int msec)
{
	//不延时，直接退出
	if (msec <= 0)   return;
	//定义一个新的事件循环,处理定时器超时事件
	QEventLoop loop;
	//创建一个定时器，槽函数为线程的退出
	//超时，发送信号，视频解码线程退出
	QTimer::singleShot(msec, &loop, SLOT(quit()));
	//事件循环开始执行，视频解码线程阻塞到这里
	//超时，线程退出
	loop.exec();
}