#pragma once
#include <QWidget>
#include<qmutex.h>
class PlayImage :
	public QWidget
{
	Q_OBJECT
public:
	explicit PlayImage(QWidget* parent = nullptr);

	void updateImage(const QImage& image);
	/// @brief Update QPixmap format
	/// @param pixmap Pixmap
	void updatePixmap(const QPixmap& pixmap);
protected:
	void paintEvent(QPaintEvent* event) override;
private:
	QPixmap m_pixmap;
	QMutex m_mutex;
};
