#include "videoplayer.h"
#include "ui_videoplayer.h"
#include <QFileDialog>

Q_DECLARE_METATYPE(MediaDecoder::PlayerState);

videoPlayer::videoPlayer(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::videoPlayer)
{
    ui->setupUi(this);
    decoder = new MediaDecoder();
    qRegisterMetaType<MediaDecoder::PlayerState>();
    connect(decoder,SIGNAL(sig_Statechanged(MediaDecoder::PlayerState)),this,SLOT(slotStateChanged(MediaDecoder::PlayerState)));

    onPlay = false;
    connect(ui->pushButton_open,SIGNAL(clicked()),this,SLOT(slotBtnClicked()));
    connect(ui->toolButton_open,SIGNAL(clicked()),this,SLOT(slotBtnClicked()));
    connect(ui->pushButton_play,SIGNAL(clicked()),this,SLOT(slotBtnClicked()));
    connect(ui->pushButton_stop,SIGNAL(clicked()),this,SLOT(slotBtnClicked()));
    ui->verticalLayout_show_video->addWidget(decoder->getVideoWidget());
    decoder->getVideoWidget()->hide();
    // ui->verticalLayout_show_video->removeWidget(decoder->getVideoWidget());
}

videoPlayer::~videoPlayer()
{
    delete ui;
}

void videoPlayer::slotBtnClicked()
{
    qDebug()<<"btnclicked";
    if (QObject::sender() == ui->pushButton_play)
    {
        if(onPlay)
        {
            decoder->pause();
            onPlay = false;
        }
        else
        {
            decoder->play();
            onPlay = true;
        }
    }
    else if (QObject::sender() == ui->pushButton_stop)
    {
        decoder->stop(true);
    }
    else if (QObject::sender() == ui->pushButton_open || QObject::sender() == ui->toolButton_open)
    {
        QString s = QFileDialog::getOpenFileName(
            this, "选择要播放的文件",
            "/",//初始目录
            "视频文件 (*.flv *.rmvb *.avi *.MP4);; 所有文件 (*.*);; ");
        if (!s.isEmpty())
        {
            s.replace("/","\\");

            decoder->stop(true); //如果在播放则先停止

            decoder->getFileName(s);
        }
    }
}

void videoPlayer::slotStateChanged(MediaDecoder::PlayerState state)
{
    qDebug()<<"state changed";
    switch(state)
    {
    case MediaDecoder::Playing:
    {
        ui->widget_showOpen->hide();
        ui->pushButton_play->setText("暂停");
        decoder->getVideoWidget()->show();
        break;
    }
    case MediaDecoder::Pause:
    {
        ui->pushButton_play->setText("播放");
        break;
    }
    case MediaDecoder::Stop:
    {
        ui->widget_showOpen->show();
        ui->pushButton_play->setText("播放");
        decoder->getVideoWidget()->hide();
        break;
    }
    default:
    {
        break;
    }
    }
}

void videoPlayer::on_pushButton_play_clicked()
{
    qDebug()<<decoder->getPlayState();
    if(decoder->getPlayState() == MediaDecoder::Playing)
    {
        decoder->pause();
        ui->pushButton_play->setText("播放");
    }
    else if(decoder->getPlayState() == MediaDecoder::Pause)
    {
        decoder->play();
        ui->pushButton_play->setText("暂停");
    }
}


void videoPlayer::on_pushButton_stop_clicked()
{
    decoder->stop();
    ui->pushButton_play->setText("播放");
}

