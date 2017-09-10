#include "isolator.h"

Isolator::Isolator(QWidget *parent) :
    QMainWindow(parent)
{
    setupUi(this);
}

void Isolator::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        retranslateUi(this);
        break;
    default:
        break;
    }
}
