#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "video_decode.h"
#include "mediadecoder.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    Video_decode* decoder;

    MediaDecoder* media;

signals:
    void sendFileName(QString);
public slots:
    void getOneFrame(QImage tmpimg);

private slots:
    void on_actionOpenFile_triggered();

private:
    bool changeSize;
    int width, height;
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
