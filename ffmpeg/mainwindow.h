#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QThread>

#include "../textrender.h"
#include "../yuvrender.h"
#include "videodecoder.h"

class QMutex;
class QElapsedTimer;

class VideoDecodeThread: public QThread
{
    Q_OBJECT
public:
    VideoDecodeThread(VideoDecoder *decoder, QObject *parent = nullptr);
    virtual ~VideoDecodeThread();

    void Stop();
    void Seek(int sec);

protected:
    void run() override;

signals:
    void frameready(const unsigned char* const *data, const int *linesize, int num, double t);

private:
    VideoDecoder *video_decoder_;
    QMutex* mutex_;
    QElapsedTimer* et_;
    double start_time_;
};

class VideoWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    VideoWidget(QWidget *parent = nullptr);
    ~VideoWidget();

    void BuildRender(int video_width, int video_height);

public slots:
    void OnRedraw(const unsigned char* const *data, const int *linesize, int num, double t);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

private:
    TextRender *text_render_;
    YUVRender* yuv_render_;

    QString time_str_;
};

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void OnOpenBtnClicked();
    void OnCloseBtnClicked();

private:
    VideoDecoder *video_decoder_;
    VideoWidget *video_widget_;

    VideoDecodeThread* decode_thread_;

};
#endif // MAINWINDOW_H
