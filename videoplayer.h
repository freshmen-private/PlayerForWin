#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QWidget>
#include "mediadecoder.h"

namespace Ui {
class videoPlayer;
}

class videoPlayer : public QWidget
{
    Q_OBJECT

public:
    explicit videoPlayer(QWidget *parent = nullptr);
    ~videoPlayer();

private slots:
    void slotBtnClicked();
    void slotStateChanged(MediaDecoder::PlayerState state);

    void on_pushButton_play_clicked();

    void on_pushButton_stop_clicked();

private:
    Ui::videoPlayer *ui;

    MediaDecoder* decoder;
    bool onPlay;
};

#endif // VIDEOPLAYER_H
