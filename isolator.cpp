#include "isolator.h"
#include <QProcess>
#include <QRegExp>
#include <QDebug>

Isolator::Isolator(QWidget *parent) :
    QMainWindow(parent)
{
    setupUi(this);
    exclude = new QSettings("Prime-Hack", "AppIsolator", this);

    QString ex;
    for (int i = 0; i < exclude->value("Count").toInt(); ++i)
        ex += exclude->value(QString::number(i)).toString() + "\n";
    if (ex.isEmpty())
        ex = "libGL\nlibGLX\n";
    excludeLibs->setPlainText(ex);
}

void Isolator::closeEvent(QCloseEvent *)
{
    QStringList ex = excludeLibs->toPlainText().split('\n');
    exclude->setValue("Count", ex.count());

    for (int i = 0; i < ex.count(); ++i)
        exclude->setValue(QString::number(i), ex[i]);
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

//    if (subDir == "lib/" && file.suffix().toLower() != "so") // TODO: Change ti spawn process 'ln'
//        copyTo(QFileInfo(file.path() + "/" + file.completeBaseName()), "lib", file.filePath());
    if (subDir == "lib/" && file.suffix().toLower() != "so")
        CreateSymLink(outDir + "/" + case_name->text() + "/" + subDir + file.fileName(), file.completeBaseName());
}

void Isolator::CreateSymLink(QFileInfo file, QString link)
{
    if (link.at(0) != '/')
        link = file.path() + "/" + link;

    QProcess ln;
    QStringList args;

    args << file.filePath();
    args << link;

    ln.setProgram("ln");
    ln.setArguments(args);

    ln.start();
    ln.waitForStarted();
    ln.waitForFinished();

    if (file.completeSuffix().indexOf("so") == 0 && file.completeSuffix() != file.suffix())
        CreateSymLink(QFileInfo(link), file.path() + "/" + file.completeBaseName());
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

void Isolator::createStarter(QString file)
{
    QFile starter(outDir + "/" + case_name->text() + "/bin/" + file + ".sh");
    starter.open(QIODevice::WriteOnly);

    starter.write("#!/bin/bash\n");
    starter.write(QString("export LD_LIBRARY_PATH='../lib'\n").toStdString().c_str());
    starter.write(QString("export PATH='../bin':$PATH\n").toStdString().c_str());
    starter.write(QString("'" + file + "'\n").toStdString().c_str());

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
    if (excludeLibs->toPlainText().at(excludeLibs->toPlainText().length() -1) != '\n')
        excludeLibs->setPlainText(excludeLibs->toPlainText() + "\n");
    QStringList ex = excludeLibs->toPlainText().split('\n');

    // TODO: Create desktop file
    for (auto app : applications){
        copyTo(app);
        createStarter(app.fileName());
        // TODO: Add starter to desktop as action
        QFileInfoList libs = getLibs(app);
        for (auto lib : libs){
            bool copy = true;
            for (auto str : ex)
                if (!str.isEmpty())
                    if (lib.fileName().indexOf(str) != -1)
                        copy = false;
            if (copy)
                copyTo(lib, "lib");
        }
    }
}

void Isolator::on_case_name_textChanged(const QString &arg1)
{
    if (!arg1.isEmpty())
        isolate->setEnabled(true);
    else isolate->setEnabled(false);

    case_name->setText(QString(arg1).replace('/', '\\'));
}
