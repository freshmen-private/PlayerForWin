#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    height = 0;
    width = 0;
    changeSize = false;
    // decoder = new Video_decode();
    // decoder->start();
    // connect(this, SIGNAL(sendFileName(QString)), decoder, SLOT(getFileName(QString)));
    // connect(decoder, SIGNAL(sendOneFrame(QImage)), this, SLOT(getOneFrame(QImage)));
    media = new MediaDecoder();
    media->startPlay();
    connect(this, SIGNAL(sendFileName(QString)), media, SLOT(getFileName(QString)));
    connect(media, SIGNAL(sendOneFrame(QImage)), ui->openGLWidget, SLOT(getOneFrame(QImage)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::getOneFrame(QImage tmpimg)
{
    if(tmpimg.width() != width && tmpimg.height() != height)
    {
        width = tmpimg.width();
        height = tmpimg.height();
        changeSize = true;
    }
    ui->openGLWidget->OpenFile(tmpimg);
}

void MainWindow::on_actionOpenFile_triggered()
{
    QRect uiRect = this->geometry();
    QString Filename = QFileDialog::getOpenFileName(this, "C:\\", "*.*");
    emit sendFileName(Filename);
}

