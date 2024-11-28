#include "imageplayer.h"
#include "ui_imageplayer.h"

imagePlayer::imagePlayer(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::imagePlayer)
{
    ui->setupUi(this);
}

imagePlayer::~imagePlayer()
{
    delete ui;
}
