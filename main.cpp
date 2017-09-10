#include "isolator.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Isolator w;
    w.show();

    return a.exec();
}
