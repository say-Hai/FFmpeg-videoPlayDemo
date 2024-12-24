#include "PlayImage.h"
#include<QPainter>

PlayImage::PlayImage(QWidget* parent) : QWidget(parent)
{
	//�޸ı�����ɫ
	QPalette palette(this->palette());
	palette.setColor(QPalette::Window, Qt::black);
	this->setPalette(palette);
	this->setAutoFillBackground(true);
}

void PlayImage::updateImage(const QImage& image)
{
	//����QPixmap���ڻ滭�¼����ȶ������٣����ﲻ����Image��ʽ��ͼƬ
	//ֱ��ת��ΪQPixmap�ٵ���updatePixmap
	updatePixmap(QPixmap::fromImage(image));
}

void PlayImage::updatePixmap(const QPixmap& pixmap)
{
	//��Ϊ�����ڶ��̷߳��ʵ�ʱ�򣬿��ܻ��m_pixmap������⣬����������ĸ�������
	m_mutex.lock();
	m_pixmap = pixmap;
	m_mutex.unlock();
	//�����ػ溯��paintEvent����
	update();
}
/// ��д��ͼ�¼�
void PlayImage::paintEvent(QPaintEvent* event)
{
	//��ͼ���ػ�
	if (!m_pixmap.isNull())
	{
		//ʵ����һ����ͼ����
		QPainter painter(this);
		m_mutex.lock();
		//��ͼ�񰴸����ڵĴ�С�����ֿ�߱���С,ԭʼͼƬ���ܲ����䲥�����ߴ�
		QPixmap pixmap = m_pixmap.scaled(this->size(), Qt::KeepAspectRatio);
		m_mutex.unlock();
		//���л滭
		int x = (this->width() - pixmap.width()) / 2;
		int y = (this->height() - pixmap.height()) / 2;
		painter.drawPixmap(x, y, pixmap);
	}
	//����QWidget�Ļ滭������ʵ�ֻ��ƹ���
	QWidget::paintEvent(event);
}