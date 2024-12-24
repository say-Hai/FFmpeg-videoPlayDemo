#include "FFmpegvideoPlayDemo.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    FFmpegvideoPlayDemo w;
    w.show();
    return a.exec();
}
