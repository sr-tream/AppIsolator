#include "isolator.h"
#include <QProcess>
#include <QRegExp>
#include <QDebug>

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

void Isolator::createDir(QString dir)
{
    QProcess mkdir;
    mkdir.setProgram("mkdir");
    mkdir.setArguments(QStringList(dir));
    mkdir.start();
    mkdir.waitForStarted();
    mkdir.waitForFinished(-1);
}

void Isolator::copyTo(QFileInfo file, QString subDir, QString kName)
{
    if (subDir.at(subDir.length() - 1) != '/')
        subDir += '/';

    QProcess cp;
    QStringList args;

    if (kName.isEmpty())
        args << file.filePath();
    else args << kName;
    args << outDir + "/" + case_name->text() + "/" + subDir + file.fileName();
    createDir(outDir + "/" + case_name->text() + "/" + subDir);

    cp.setProgram("cp");
    cp.setArguments(args);

    cp.start();
    cp.waitForStarted();
    cp.waitForFinished(-1);

    if (subDir == "lib/" && file.suffix().toLower() != "so") // TODO: Change ti spawn process 'ln'
        copyTo(QFileInfo(file.path() + "/" + file.completeBaseName()), "lib", file.filePath());
}

QFileInfoList Isolator::getLibs(QFileInfo file)
{
    QProcess ldd;

    ldd.setProgram("ldd");
    ldd.setArguments(QStringList(file.filePath()));

    ldd.start();
    ldd.waitForStarted();
    ldd.waitForFinished(-1);

    QString out = ldd.readAllStandardOutput().toStdString().c_str();
    qDebug() << "getLibs():" << out;
    QStringList outList = out.split('\n');
    QRegExp re(R"(.*\s(\/.*)\s\(0x[\da-fA-F]+\))", Qt::CaseInsensitive);
    QFileInfoList libs;

    for(auto str : outList){
        if (re.indexIn(str) == 0){
            if (QFileInfo(re.cap(1)).isSymLink())
                libs.push_back(QFileInfo(re.cap(1)).symLinkTarget());
            else libs.push_back(QFileInfo(re.cap(1)));
        }
    }

    return libs;
}

void Isolator::createStarter(QFileInfo file)
{
    QFile starter(file.filePath() + ".sh");
    starter.open(QIODevice::WriteOnly);

    starter.write("#!/bin/bash\n");
    starter.write(QString("export LD_LIBRARY_PATH='" + outDir + "/" + case_name->text() + "/lib'\n").toStdString().c_str());
    starter.write(QString("export PATH='" + outDir + "/" + case_name->text() + "/bin':$PATH\n").toStdString().c_str());
    starter.write(QString("'" + file.filePath() + "'\n").toStdString().c_str());

    starter.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner |
                           QFileDevice::ReadGroup | QFileDevice::ExeGroup | QFileDevice::ReadUser |
                           QFileDevice::ExeUser | QFileDevice::ReadOther | QFileDevice::ExeOther);
    starter.close();
}

void Isolator::on_isolate_clicked()
{
    QStringList apps = QFileDialog::getOpenFileNames(this, "Select files", "/usr/bin/");
    for(auto app : apps)
        applications.push_back(QFileInfo(app));
    outDir = QFileDialog::getExistingDirectory(this, "Select directory for case", QDir::homePath());
    createDir(outDir + "/" + case_name->text());

    // TODO: Create desktop file
    for (auto app : applications){
        copyTo(app);
        createStarter(QFileInfo( outDir + "/" + case_name->text() + "/bin/" + app.fileName()));
        // TODO: Add starter to desktop as action
        QFileInfoList libs = getLibs(app);
        for (auto lib : libs)
            copyTo(lib, "lib");
    }
}

void Isolator::on_case_name_textChanged(const QString &arg1)
{
    if (!arg1.isEmpty())
        isolate->setEnabled(true);
    else isolate->setEnabled(false);

    case_name->setText(QString(arg1).replace('/', '\\'));
}
