#pragma execution_character_set("utf-8")

#include "FFmpegvideoPlayDemo.h"
#include<QFileDialog>
#include<QDebug>
FFmpegvideoPlayDemo::FFmpegvideoPlayDemo(QWidget* parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	//���ñ���
	this->setWindowTitle(QString("VideoPlay Version 1.00"));
	//ʵ������Ƶ�����߳�
	m_readThread = new readThread();
	////�������̵߳��Զ����ź�updateImage�ź���PlayImage��,ֱ�ӵ��òۺ������ۺ�����ִ���꣬����
	connect(m_readThread, &readThread::updateImage, ui.playimage, &PlayImage::updateImage, Qt::DirectConnection);
	////�������̵߳��Զ��岥��״̬�ı���ź��봰���̵߳�on_PlayState�ۺ�����
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
	QString strName = QFileDialog::getOpenFileName(this,//ָ��������
		"ѡ�񲥷���Ƶ",//dialog����
		"/",//�Ӹ�Ŀ¼��ʼ
		"��Ƶ(*.mp4 *.m4v *.avi *.flv);;����(*)"//�������ַ���
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
	if (ui.but_open->text() == "��ʼ����")
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
	if (ui.btn_pause->text() == "��ͣ����")
	{
		m_readThread->pause(true);
		ui.btn_pause->setText("��������");
	}
	//��������
	else
	{
		m_readThread->pause(false);
		ui.btn_pause->setText("��ͣ����");
	}
}

void FFmpegvideoPlayDemo::on_playState(readThread::PlayState state)
{
	if (state == readThread::play)
	{
		this->setWindowTitle(QString("���ڲ���: %1").arg(m_readThread->url()));
		ui.but_open->setText("ֹͣ����");
	}
	else
	{
		ui.but_open->setText("��ʼ����");
		ui.btn_pause->setText("��ͣ����");
		this->setWindowTitle(QString("VideoPlay Version 1.00"));
	}
}