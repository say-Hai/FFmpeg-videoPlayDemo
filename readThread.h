#pragma once
#include<QThread>
#include<QElapsedTimer>
#include<QElapsedTimer>

class videoDecode;

class readThread :
	public QThread
{
	Q_OBJECT
public:
	enum PlayState
	{
		play, end
	};
	explicit readThread(QObject* parent = nullptr);

	~readThread() override;

	/// @brief Pass in the video address to start the video decoding thread
	/// @param url video Path
	void open(const QString& url = QString());
	/// @brief close the video
	void close();
	/// @brief pause the video
	/// @param flag the video state
	void pause(bool flag);
	/// @brief Get the opened video address
	/// @return video addr
	const QString& url();
protected:
	/// @brief override the QThread run() to execute task
	void run() override;

signals:
	/// @brief the video state is changed to send the signal
	void playState(PlayState state);
	/// @brief update the image to send the signal
	/// @param img the image data
	void updateImage(const QImage& img);
private:
	/// @brief Video decoding object
	videoDecode* m_videoDecode = NULL;
	QString m_url; /// video url
	bool m_play = false; ///Play control bit
	bool m_pause = false; ///pause flag bit

	QElapsedTimer m_etime2;
};
void sleepMesc(int msec);