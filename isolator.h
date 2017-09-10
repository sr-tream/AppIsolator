#ifndef ISOLATOR_H
#define ISOLATOR_H

#include "ui_isolator.h"
#include <QFileDialog>

class Isolator : public QMainWindow, private Ui::Isolator
{
    Q_OBJECT

public:
    explicit Isolator(QWidget *parent = 0);

protected:
    void changeEvent(QEvent *e);
    void createDir(QString dir);
    void copyTo(QFileInfo file, QString subDir = QString("bin"), QString kName = QString());
    QFileInfoList getLibs(QFileInfo file);
    void createStarter(QFileInfo file);
private slots:
    void on_isolate_clicked();

    void on_case_name_textChanged(const QString &arg1);

private:
    QFileInfoList applications;
    QString       outDir;
};

#endif // ISOLATOR_H
