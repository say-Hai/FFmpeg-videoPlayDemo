#pragma execution_character_set("utf-8")

#include "FFmpegvideoPlayDemo.h"
#include<QFileDialog>
#include<QDebug>
FFmpegvideoPlayDemo::FFmpegvideoPlayDemo(QWidget* parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	//设置标题
	this->setWindowTitle(QString("VideoPlay Version 1.00"));
	//实例化视频解码线程
	m_readThread = new readThread();
	////将解码线程的自定义信号updateImage信号与PlayImage绑定,直接调用槽函数，槽函数不执行完，阻塞
	connect(m_readThread, &readThread::updateImage, ui.playimage, &PlayImage::updateImage, Qt::DirectConnection);
	////将解码线程的自定义播放状态改变的信号与窗口线程的on_PlayState槽函数绑定
	connect(m_readThread, &readThread::playState, this, &FFmpegvideoPlayDemo::on_playState);
}

FFmpegvideoPlayDemo::~FFmpegvideoPlayDemo()
{
	if (m_readThread)
	{
		m_readThread->close();
		m_readThread->wait();
		delete m_readThread;
	}
}

void FFmpegvideoPlayDemo::on_but_file_clicked()
{
	QString strName = QFileDialog::getOpenFileName(this,//指定父窗口
		"选择播放视频",//dialog标题
		"/",//从根目录开始
		"视频(*.mp4 *.m4v *.avi *.flv);;其它(*)"//过滤器字符串
	);
	if (strName.isEmpty())
	{
		qDebug() << " achieve the file path is error:NULL ";
		return;
	}
	qDebug() << strName;
	ui.com_url->setCurrentText(strName);
}

void FFmpegvideoPlayDemo::on_but_open_clicked()
{
	if (ui.but_open->text() == "开始播放")
	{
		m_readThread->open(ui.com_url->currentText());
	}
	else
	{
		m_readThread->close();
	}
}

void FFmpegvideoPlayDemo::on_btn_pause_clicked()
{
	if (ui.btn_pause->text() == "暂停播放")
	{
		m_readThread->pause(true);
		ui.btn_pause->setText("继续播放");
	}
	//继续播放
	else
	{
		m_readThread->pause(false);
		ui.btn_pause->setText("暂停播放");
	}
}

void FFmpegvideoPlayDemo::on_playState(readThread::PlayState state)
{
	if (state == readThread::play)
	{
		this->setWindowTitle(QString("正在播放: %1").arg(m_readThread->url()));
		ui.but_open->setText("停止播放");
	}
	else
	{
		ui.but_open->setText("开始播放");
		ui.btn_pause->setText("暂停播放");
		this->setWindowTitle(QString("VideoPlay Version 1.00"));
	}
}