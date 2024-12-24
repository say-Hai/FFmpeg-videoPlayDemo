#include "PlayImage.h"
#include<QPainter>

PlayImage::PlayImage(QWidget* parent) : QWidget(parent)
{
	//修改背景颜色
	QPalette palette(this->palette());
	palette.setColor(QPalette::Window, Qt::black);
	this->setPalette(palette);
	this->setAutoFillBackground(true);
}

void PlayImage::updateImage(const QImage& image)
{
	//由于QPixmap用于绘画事件更稳定更快速，这里不处理Image格式的图片
	//直接转换为QPixmap再调用updatePixmap
	updatePixmap(QPixmap::fromImage(image));
}

void PlayImage::updatePixmap(const QPixmap& pixmap)
{
	//因为这里在多线程访问的时候，可能会对m_pixmap造成问题，给这个变量的更新上锁
	m_mutex.lock();
	m_pixmap = pixmap;
	m_mutex.unlock();
	//调用重绘函数paintEvent函数
	update();
}
/// 重写绘图事件
void PlayImage::paintEvent(QPaintEvent* event)
{
	//有图就重绘
	if (!m_pixmap.isNull())
	{
		//实例化一个绘图对象
		QPainter painter(this);
		m_mutex.lock();
		//把图像按父窗口的大小，保持宽高比缩小,原始图片可能不适配播放器尺寸
		QPixmap pixmap = m_pixmap.scaled(this->size(), Qt::KeepAspectRatio);
		m_mutex.unlock();
		//居中绘画
		int x = (this->width() - pixmap.width()) / 2;
		int y = (this->height() - pixmap.height()) / 2;
		painter.drawPixmap(x, y, pixmap);
	}
	//调用QWidget的绘画函数，实现绘制功能
	QWidget::paintEvent(event);
}