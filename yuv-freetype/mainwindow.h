#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include "../textrender.h"
#include "../yuvrender.h"

class QOpenGLTexture;

class MainWindow : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void initializeGL();
    void paintGL();

private:
    TextRender *text_render_;
    YUVRender* yuv_render_;

    uchar* buff_;
    int buff_size_;
};

#endif // MAINWINDOW_H
