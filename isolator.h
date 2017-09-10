#ifndef ISOLATOR_H
#define ISOLATOR_H

#include "ui_isolator.h"
#include <QFileDialog>
#include <QSettings>
#include <QList>
#include <QPair>

typedef QPair<QFileInfo, QString> Lib;
typedef QList<Lib> LibList;

class Isolator : public QMainWindow, private Ui::Isolator
{
    Q_OBJECT

public:
    explicit Isolator(QWidget *parent = 0);

protected:
    void closeEvent(QCloseEvent *);
    void changeEvent(QEvent *e);
    void createDir(QString dir);
    void copyTo(QFileInfo file, QString subDir = QString("bin"), QString newName = QString());
    LibList getLibs(QFileInfo file);
    void createStarter(QString file);
private slots:
    void on_isolate_clicked();

    void on_case_name_textChanged(const QString &arg1);

    void on_case_name_textEdited(const QString &arg1);

private:
    QFileInfoList applications;
    QString       outDir;
    QSettings     *exclude;
};

#endif // ISOLATOR_H
