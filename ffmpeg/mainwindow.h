#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>


#include <QThread>
//#include <QQueue>
//#include <QMutex>
#include <QElapsedTimer>
#include "../yuvrender.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#ifdef __cplusplus
}
#endif



class VideoDecoder
{
public:
    VideoDecoder();
    virtual ~VideoDecoder();

    int LoadFromFile(const char* filename);
    void Close();

    int ReadFrame(unsigned char **buff, int *len, AVFrame** frame);
    int GetFrameCount() const { return frame_number_; }
    int GetFrameWidth() const { return code_ctx_ ? code_ctx_->width : 0; }
    int GetFrameHeight() const { return code_ctx_ ? code_ctx_->height : 0; }

private:
    AVCodecContext* code_ctx_;
    AVFormatContext* fmt_ctx_;
    SwsContext *sws_cxt_;
    AVPacket* packet_;
    AVFrame *frame_;
    int video_stream_index_;
    unsigned char *buff_;
    int buff_size_;
//    int width;
//    int height;

    uint8_t *rgb_data_[4];
    int rgb_linesize_[4];

    int frame_number_;
};

class VideoDecodeThread: public QThread
{
    Q_OBJECT
public:
    VideoDecodeThread(QObject *parent = nullptr);
    virtual ~VideoDecodeThread();

    void run() override;

signals:
    void frameready(uint8_t *buff, int len, AVFrame* frame);

public:
    VideoDecoder *video_decoder_;
};

class VideoWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    VideoWidget(QWidget *parent = nullptr);
    ~VideoWidget();

    void SetFrameWidth(int width) {frame_width_ = width; }
    void SetFrameHeight(int height) {frame_height_ = height; }
    void BuildRender();
    void Redraw(const unsigned char* buff, int size, AVFrame *frame);
    void Clear() { buff_ = nullptr; }

private:
    void initializeGL();
    void paintGL();

private:
//    TextRender *text_render_;
    YUVRender* yuv_render_;

    const unsigned char* buff_;
    int buff_size_;
    int frame_width_;
    int frame_height_;

    AVFrame *frame_;
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

    void OnTimerOut();
    void OnFrameReady(unsigned char* buff, int size, AVFrame* frame);

private:
//    VideoDecodeThread* decode_thread_;

    QElapsedTimer *et_;

    QTimer *timer_;    
    VideoDecoder *video_decoder_;
    VideoWidget *video_widget_;

    VideoDecodeThread* decode_thread_;
};
#endif // MAINWINDOW_H
