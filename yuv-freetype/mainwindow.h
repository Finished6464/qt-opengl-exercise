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

protected:
    void showEvent(QShowEvent *event) override;

private:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

private:
    TextRender *text_render_;
    YUVRender* yuv_render_;

    uchar* buff_;
    int buff_size_;

    bool first_show_;
};

#endif // MAINWINDOW_H
