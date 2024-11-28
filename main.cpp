#include <QApplication>
#include "videoplayer.h"

#undef main
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    videoPlayer w;
    w.resize(800, 600);
    w.show();
    return a.exec();
}
