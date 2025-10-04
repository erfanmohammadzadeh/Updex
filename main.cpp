#include "mainwindow.h"
#include <QFile>
#include <QApplication>

int main(int argc, char *argv[])
{
    QFile file(":/Resource/style.qss");
    file.open(QFile::ReadOnly | QFile::Text);
    QString style = file.readAll();

    QApplication a(argc, argv);
    a.setStyleSheet(style);
    a.setWindowIcon(QIcon(":/Resource/icon.ico"));
    MainWindow w;
    w.show();
    return a.exec();
}
