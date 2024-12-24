#pragma once

#include <QtWidgets/QWidget>
#include "ui_FFmpegvideoPlayDemo.h"
#include "readThread.h"
class FFmpegvideoPlayDemo : public QWidget
{
	Q_OBJECT

public:
	FFmpegvideoPlayDemo(QWidget* parent = nullptr);
	~FFmpegvideoPlayDemo();
private slots:
	/// @brief Slot function for select the video file button
	void on_but_file_clicked();
	/// @brief Slot function of the start play button
	void on_but_open_clicked();
	/// @brief Slot function of the pause button
	void on_btn_pause_clicked();
	/// @brief Handle page playback state
	void on_playState(readThread::PlayState state);
private:
	Ui::FFmpegvideoPlayDemoClass ui;
	/// @brief sub thread to achieve image
	readThread* m_readThread = NULL;
};
