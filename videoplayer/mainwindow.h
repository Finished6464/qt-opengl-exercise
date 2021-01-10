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
class QSlider;
class QLabel;
class QPushButton;


class VideoFrameReadThread: public QThread
{
    Q_OBJECT
public:
    VideoFrameReadThread(VideoDecoder *video_decoder, QObject *parent = nullptr);
    virtual ~VideoFrameReadThread();

    void Start();
    void Stop();
    void Pause(bool pause);
    bool IsPaused() const { return paused_; }
    void Seek(int sec);
    QMutex* mutex() { return mutex_; }
    uint uuid_hash() const { return uuid_hash_; }

protected:
    void run() override;

signals:
    void frameready(uint uuid_hash, const unsigned char* const *data, const int *linesize, int num);

private:
    VideoDecoder *video_decoder_;
    QMutex* mutex_;
    QElapsedTimer* et_;
    double start_time_;
    uint uuid_hash_;
    bool paused_;
};


class VideoWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    VideoWidget(QWidget *parent = nullptr);
    ~VideoWidget();

    void BuildRender(int yuv_width, int yuv_height, const char* font_filename);
    void BuildYuvRender(int yuv_width, int yuv_height);
    void BuildTextRender(const char* font_filename);

    void SetYuvData(const unsigned char* const *data, const int *linesize, int num);
    void SetTime(double t);
    void SetStateChr(ushort ch) {state_chr_ = ch; update();}

    void EnableDraw(bool enabled) { draw_enabled_ = enabled; update(); }

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

private:
    TextRender *text_render_;
    YUVRender* yuv_render_;

    QString time_str_;
    ushort state_chr_;
    bool draw_enabled_;
};

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    enum PlayStatus {
        Closed = 0,
        Playing,
        Stopped,
        Paused
    };

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void OnOpenBtnClicked();
    void OnPlayBtnClicked();

    void OnTimeout();
    void OnFrameReady(uint uuid_hash, const unsigned char* const *data, const int *linesize, int num);

private:
    VideoDecoder *video_decoder_;
    VideoWidget *video_widget_;

    VideoFrameReadThread* vfr_thread_;

    QPushButton* load_btn_;
    QPushButton* play_btn_;
    QSlider* slider_;
    QLabel* time_lbl_;

    QTimer* timer_;
    int status_;
};
#endif // MAINWINDOW_H
