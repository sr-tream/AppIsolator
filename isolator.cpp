#include "isolator.h"
#include <QProcess>
#include <QRegExp>
#include <QDebug>

Isolator::Isolator(QWidget *parent) :
    QMainWindow(parent)
{
    setupUi(this);
    progressBar->setHidden(true);
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
    while (true)
        if (!ex.removeOne(""))
            break;
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

void Isolator::copyTo(QFileInfo file, QString subDir, QString newName)
{
    if (subDir.at(subDir.length() - 1) != '/')
        subDir += '/';

    QProcess cp;
    QStringList args;

    args << file.filePath();
    if (newName.isEmpty())
        args << outDir + "/" + case_name->text() + "/" + subDir + file.fileName();
    else args << outDir + "/" + case_name->text() + "/" + subDir + newName;
    createDir(outDir + "/" + case_name->text() + "/" + subDir);

    cp.setProgram("cp");
    cp.setArguments(args);

    cp.start();
    cp.waitForStarted();
    cp.waitForFinished(-1);
}

LibList Isolator::getLibs(QFileInfo file)
{
    QProcess ldd;

    ldd.setProgram("ldd");
    ldd.setArguments(QStringList(file.filePath()));

    ldd.start();
    ldd.waitForStarted();
    ldd.waitForFinished(-1);

    QString out = ldd.readAllStandardOutput().toStdString().c_str();
    QStringList outList = out.split('\n');
    QRegExp re(R"(\s*(.*)\s=>\s(\/.*)\s\(0x[\da-fA-F]+\))", Qt::CaseInsensitive);
    LibList libs;

    for(auto str : outList){
        if (re.indexIn(str) == 0){
            Lib lib;
            lib.second = re.cap(1);
            if (lib.second.indexOf('\t') == 0)
                lib.second.remove('\t');

            if (QFileInfo(re.cap(2)).isSymLink())
                lib.first = QFileInfo(re.cap(2)).symLinkTarget();
            else lib.first = QFileInfo(re.cap(2));

            libs.push_back(lib);
        }
    }

    return libs;
}

void Isolator::createStarter(QString file)
{
    QFile starter(outDir + "/" + case_name->text() + "/bin/" + file + ".sh");
    starter.open(QIODevice::WriteOnly);

    starter.write("#!/bin/bash\n");
    starter.write(QString("export LD_LIBRARY_PATH=${0%${0##*/}}'../lib'\n").toStdString().c_str());
    starter.write(QString("export PATH=${0%${0##*/}}:$PATH\n").toStdString().c_str());
    starter.write(QString("'" + file + "'\n").toStdString().c_str());

    starter.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner |
                           QFileDevice::ReadGroup | QFileDevice::ExeGroup | QFileDevice::ReadUser |
                           QFileDevice::ExeUser | QFileDevice::ReadOther | QFileDevice::ExeOther);
    starter.close();
}

void Isolator::on_isolate_clicked()
{
    QStringList apps = QFileDialog::getOpenFileNames(this, "Select files", "/usr/bin/");
    LibList libs;
    for(auto app : apps){
        applications.push_back(QFileInfo(app));
        LibList appLibs = getLibs(app);
        for (auto lib : appLibs)
            libs.push_back(lib);
    }

    progressBar->setMaximum(apps.count() + libs.count());
    progressBar->setHidden(false);

    outDir = QFileDialog::getExistingDirectory(this, "Select directory for case", QDir::homePath());
    createDir(outDir + "/" + case_name->text());
    QStringList ex = excludeLibs->toPlainText().split('\n');

    // TODO: Create desktop file
    for (auto app : applications){
        copyTo(app);
        createStarter(app.fileName());
        progressBar->setValue(progressBar->value() + 1);
        // TODO: Add starter to desktop as action
    }

    for (auto lib : libs){
        bool copy = true;
        for (auto str : ex)
            if (!str.isEmpty())
                if (lib.first.fileName().indexOf(str) != -1)
                    copy = false;
        if (copy)
            copyTo(lib.first, "lib", lib.second);
        progressBar->setValue(progressBar->value() + 1);
    }

    case_name->setText("");
}

void Isolator::on_case_name_textChanged(const QString &arg1)
{
    if (!arg1.isEmpty())
        isolate->setEnabled(true);
    else isolate->setEnabled(false);
}

void Isolator::on_case_name_textEdited(const QString &arg1)
{
    case_name->setText(QString(arg1).replace('/', '\\'));
    progressBar->setValue(0);
    progressBar->setVisible(false);
}
