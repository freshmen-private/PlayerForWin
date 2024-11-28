#ifndef IMAGEPLAYER_H
#define IMAGEPLAYER_H

#include <QWidget>

namespace Ui {
class imagePlayer;
}

class imagePlayer : public QWidget
{
    Q_OBJECT

public:
    explicit imagePlayer(QWidget *parent = nullptr);
    ~imagePlayer();

public:
    Ui::imagePlayer *ui;
};

#endif // IMAGEPLAYER_H
