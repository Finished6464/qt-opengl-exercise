#include "mainwindow.h"

#include <QApplication>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    int ret = TextRender::InitLib();
    if (ret != 0) {
        QMessageBox::critical(nullptr, "error", QString("FreeTypeOpenGlContext::InitLib failed(%1)").arg(ret));
        return -1;
    }


    MainWindow *w = new MainWindow;
    w->show();
    ret = a.exec();
    delete w;
    TextRender::ReleaseLib();
    return ret;
}
