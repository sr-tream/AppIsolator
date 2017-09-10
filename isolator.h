#ifndef ISOLATOR_H
#define ISOLATOR_H

#include "ui_isolator.h"

class Isolator : public QMainWindow, private Ui::Isolator
{
    Q_OBJECT

public:
    explicit Isolator(QWidget *parent = 0);

protected:
    void changeEvent(QEvent *e);
};

#endif // ISOLATOR_H
