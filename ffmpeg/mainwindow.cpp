#include "mainwindow.h"
#include <QDebug>
#include <QMessageBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTimer>

#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>

#include <QImage>
#include <QPainter>

#ifdef __cplusplus
extern "C" {
#endif
#include "libavutil/imgutils.h"
#ifdef __cplusplus
}
#endif

#include <unistd.h>

#define BUFF_ALIGN 4

void print_ffmpeg_error(int err_num)
{
    char txt[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(err_num, txt, AV_ERROR_MAX_STRING_SIZE);
    qCritical("ffmpeg error: %s(%d)", txt, err_num);
}

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
{
    QBoxLayout *hlayout, *vlayout;
    QPushButton *btn;

    vlayout = new QVBoxLayout;

    video_widget_ = new VideoWidget(this);
    vlayout->addWidget(video_widget_);

    hlayout = new QHBoxLayout;
    btn  = new QPushButton("Open");
    QObject::connect(btn, &QPushButton::clicked, this, &MainWindow::OnOpenBtnClicked);
    hlayout->addWidget(btn);
    btn  = new QPushButton("Close");
    QObject::connect(btn, &QPushButton::clicked, this, &MainWindow::OnCloseBtnClicked);
    hlayout->addWidget(btn);
    hlayout->addStretch(1);
    vlayout->addLayout(hlayout);

    this->setLayout(vlayout);
    this->resize(600,480);

    video_decoder_ = new VideoDecoder;
    et_ = new QElapsedTimer;

    timer_ = new QTimer;
    QObject::connect(timer_, &QTimer::timeout, this, &MainWindow::OnTimerOut);

    decode_thread_ = new VideoDecodeThread(this);
    QObject::connect(decode_thread_, &VideoDecodeThread::frameready, this, &MainWindow::OnFrameReady);
}

MainWindow::~MainWindow()
{
    decode_thread_->requestInterruption();
    decode_thread_->wait();
    delete decode_thread_;

    delete timer_;
    delete video_decoder_;
    delete et_;
}

void MainWindow::OnOpenBtnClicked()
{
    if (video_decoder_->LoadFromFile("/home/kai/Videos/bbb_sunflower_1080p_60fps_30sec.mp4") != 0) {
        QMessageBox::critical(this, "error", "load failed");
        return;
    }

    video_widget_->SetFrameWidth(video_decoder_->GetFrameWidth());
    video_widget_->SetFrameHeight(video_decoder_->GetFrameHeight());
    video_widget_->BuildRender();

    decode_thread_->video_decoder_ = video_decoder_;
    decode_thread_->start();
//    et_->start();
//    timer_->start();
}

void MainWindow::OnCloseBtnClicked()
{
//    qDebug("use time: %lld %lld/sec", et_->elapsed(), video_decoder_->GetFrameCount() * 1000 / et_->elapsed());
//    timer_->stop();
    decode_thread_->requestInterruption();
    decode_thread_->wait();
    video_decoder_->Close();
//    video_widget_->Clear();
}

void MainWindow::OnTimerOut()
{
    uint8_t *buff = nullptr;
    int len, width, height;
    timer_->stop();
//    struct FrameContext* frame_cxt = decode_thread_->FrameContext();
//    frame_cxt->mutex.tryLock(10);
//    if (frame_cxt->queue.size() > 0) {
//        buff = frame_cxt->queue.head();
//        frame_cxt->queue.pop_front();
//    }
//    frame_cxt->mutex.unlock();

//    if (video_decoder_->ReadFrame(&buff, &len, &width, &height, nullptr) == 0) {
//        video_widget_->Redraw(buff, len);
//        timer_->start();
//    }
//    else {
//        OnCloseBtnClicked();
//    }
//    VideoDecodeThread::FreeFrameBuffer(buff);

}

void MainWindow::OnFrameReady(unsigned char* buff, int size, AVFrame* frame)
{
    video_widget_->Redraw(buff, size, frame);
}


VideoWidget::VideoWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{
//    text_render_ = new TextRender;
    yuv_render_ = new YUVRender;

    buff_ = nullptr;

    frame_width_ = frame_height_ = 0;

    frame_ = nullptr;
}

VideoWidget::~VideoWidget()
{
    makeCurrent();
//    delete text_render_;
    delete yuv_render_;
    doneCurrent();

//    if (buff_) {
//        free(buff_);
//    }
}

void VideoWidget::BuildRender()
{
    makeCurrent();
    yuv_render_->Build(frame_width_, frame_height_);
    doneCurrent();
}

void VideoWidget::Redraw(const unsigned char* buff, int size, AVFrame* frame)
{
    buff_ = buff;
    buff_size_ = size;
    frame_ = frame;
//    frame_width_ = frame_width;
//    frame_height_ = frame_height;
    update();
}

void VideoWidget::initializeGL()
{
    initializeOpenGLFunctions();

//    yuv_render_->Build();
//    text_render_->Build("test.ttf");
}

void VideoWidget::paintGL()
{

//    QPoint pt = this->pos();
//    QSize sz = QSize(FRAME_WIDTH, FRAME_HEIGHT);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//    if (this->width() < sz.width() || this->height() < sz.height()) {
//        sz.scale(this->width(), this->height(), Qt::KeepAspectRatio);
//    }
//    if (this->width() > sz.width()) {
//        pt.setX((this->width() - sz.width()) / 2);
//    }
//    if (this->height() > sz.height()) {
//        pt.setY((this->height() - sz.height()) / 2);
//    }
//    glViewport(pt.x(), pt.y(), sz.width(), sz.height());

    if (frame_) {
        yuv_render_->Render(buff_);
    }

//    glViewport(0, 0, this->width(), this->height());

//    text_render_->Render("12:53", 0.0f, 0.0f, 48, 1.0f, QColor(Qt::red));

//    ushort charcodes[] = {0x36, 0x37, 0xe001, 0xe002};
//    text_render_->Render(charcodes, sizeof(charcodes) / sizeof(charcodes[0]), 10.0f, 200.0f, 96, 2.0f, QColor(128, 255, 0));
}

VideoDecoder::VideoDecoder()
{
    code_ctx_  = nullptr;
    fmt_ctx_ = nullptr;
    packet_ = nullptr;
    frame_ = nullptr;
    buff_ = nullptr;
    frame_number_ = 0;

    sws_cxt_ = nullptr;

    memset(rgb_data_, 0, sizeof(rgb_data_));
    memset(rgb_linesize_, 0, sizeof(rgb_linesize_));
}

VideoDecoder::~VideoDecoder()
{
    Close();
}

int VideoDecoder::LoadFromFile(const char* filename)
{
    int err = 0;
    AVCodec *codec = nullptr;
    AVCodecParameters *origin_par = nullptr;

    err = avformat_open_input(&fmt_ctx_, filename, nullptr, nullptr);
    if (err < 0) {
        qCritical() << "avformat_open_input failed";
        goto failed;
    }

    err = avformat_find_stream_info(fmt_ctx_, nullptr);
    if (err < 0) {
        qCritical() << "avformat_find_stream_info failed";
        goto failed;
    }

    video_stream_index_ = av_find_best_stream(fmt_ctx_, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (video_stream_index_ < 0) {
        err = video_stream_index_;
        qCritical() << "av_find_best_stream failed";
        goto failed;
    }

    origin_par = fmt_ctx_->streams[video_stream_index_]->codecpar;

    codec = avcodec_find_decoder(origin_par->codec_id);
    if (!codec) {
        qCritical() << "avcodec_find_decoder failed";
        goto failed;
    }

//    if (codec->id == AV_CODEC_ID_H264) {
//        h264_mmal = avcodec_find_decoder_by_name("h264_mmal");
//        if (h264_mmal) {
//            codec = h264_mmal;
//        }
//    }

    code_ctx_ = avcodec_alloc_context3(codec);
    if (!code_ctx_) {
        qCritical() << "avcodec_find_decoder failed";
        goto failed;
    }

    err = avcodec_parameters_to_context(code_ctx_, origin_par);
    if (err < 0) {
        qCritical() << "avformat_find_stream_info failed";
        goto failed;
    }

    //enable multithread 0 or more than 1
    // 0: 由ffmpeg决定
    // >1: 用户指定
    code_ctx_->thread_count = 0;
//    code_ctx_->thread_type = FF_THREAD_SLICE;

    err = avcodec_open2(code_ctx_, codec, nullptr);
    if (err < 0) {
        qCritical() << "avcodec_open2 failed";
        goto failed;
    }

    packet_ = av_packet_alloc();
    if (!packet_) {
        qCritical() << "av_packet_alloc failed";
        goto failed;
    }

    frame_ = av_frame_alloc();
    if (!frame_) {
        qCritical() << "av_frame_alloc failed";
        goto failed;
    }

//    sws_cxt_ = sws_getContext(code_ctx_->width, code_ctx_->height, code_ctx_->pix_fmt,
//                              code_ctx_->width, code_ctx_->height, AV_PIX_FMT_RGB24,
//                              SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
//   err = av_image_alloc(rgb_data_, rgb_linesize_, code_ctx_->width, code_ctx_->height, AV_PIX_FMT_RGB24, 1);

    buff_size_ = av_image_get_buffer_size(code_ctx_->pix_fmt, code_ctx_->width, code_ctx_->height, BUFF_ALIGN);
    buff_ = (unsigned char *)av_malloc(buff_size_);
    if (!buff_) {
        qCritical() << "av_malloc failed";
        goto failed;
    } 

    return 0;

failed:
    print_ffmpeg_error(err);
    return err;
}

void VideoDecoder::Close()
{
    if (code_ctx_) {
        avcodec_free_context(&code_ctx_);
        code_ctx_ = nullptr;
    }

    if (fmt_ctx_) {
        avformat_close_input(&fmt_ctx_);
        fmt_ctx_ = nullptr;
    }

    if (frame_) {
        av_frame_free(&frame_);
        frame_ = nullptr;
    }

    if (packet_) {
        av_packet_free(&packet_);
        packet_ = nullptr;
    }

    if (buff_) {
        av_free(buff_);
        buff_ = nullptr;
    }

    if (rgb_data_[0]) {
        av_freep(&rgb_data_[0]);
        memset(rgb_data_, 0, sizeof(rgb_data_));
    }

    if (sws_cxt_) {
        sws_freeContext(sws_cxt_);
        sws_cxt_ = nullptr;
    }

    frame_number_ = 0;
}



int VideoDecoder::ReadFrame(unsigned char **buff, int *len, AVFrame** frame)
{
    int err;

    do {
        av_packet_unref(packet_);
        av_init_packet(packet_);
        err = av_read_frame(fmt_ctx_, packet_);
        if (err < 0 && err != AVERROR_EOF) {
            qCritical() << "av_read_frame failed";
            print_ffmpeg_error(err);
            return err;
        }

        if (err == AVERROR_EOF || packet_->stream_index == video_stream_index_) {
again:
            err = avcodec_send_packet(code_ctx_, err != AVERROR_EOF ? packet_ : nullptr);
            if (err < 0) {
                if (err == AVERROR(EAGAIN)) {
                    qWarning() << "avcodec_send_packet return EAGAIN";
                    goto again;
                }

                if (err != AVERROR_EOF) {
                    qCritical() << "avcodec_send_packet failed";
                    print_ffmpeg_error(err);
                }

                return err;
            }

            err = avcodec_receive_frame(code_ctx_, frame_);
            if (err < 0) {
                if (err == AVERROR(EAGAIN)) {
                    qWarning() << "avcodec_receive_frame return EAGAIN";
                    goto again;
                }

                if (err != AVERROR_EOF) {
                    qCritical() << "avcodec_receive_frame failed";
                    print_ffmpeg_error(err);
                }

                return err;
            }

            frame_number_++;
            err = av_image_copy_to_buffer(buff_, buff_size_, frame_->data, (const int*)frame_->linesize,
                                    code_ctx_->pix_fmt, code_ctx_->width, code_ctx_->height, BUFF_ALIGN);
            if (err < 0) {
                qCritical() << "av_image_copy_to_buffer failed";
                print_ffmpeg_error(err);
                return err;
            }
//            err = sws_scale(sws_cxt_, (const uint8_t * const*)frame_->data, frame_->linesize,
//                             0, frame_->height, rgb_data_, rgb_linesize_);
//            if (err < 0) {
//                qCritical() << "sws_scale failed";
//                print_ffmpeg_error(err);
//                return err;
//            }

            *buff = buff_;
            *len = err;

            *frame = frame_;
            return 0;
        }
    } while (1);
}

VideoDecodeThread::VideoDecodeThread(QObject *parent)
    :QThread(parent)
{
    video_decoder_ = nullptr;
}

VideoDecodeThread::~VideoDecodeThread()
{

}

void VideoDecodeThread::run()
{
    AVFrame* frame;
    uint8_t *buff = nullptr;
    int ret, len, width, height, frame_num = 0;
    QElapsedTimer et;
    et.start();
    while (!this->isInterruptionRequested()) {
        ret = video_decoder_->ReadFrame(&buff, &len, &frame);
        if (ret != 0)
            break;
        emit frameready(buff, len, frame);
        frame_num++;
//        QThread::msleep(10);
    }

    qDebug("frame: %d time: %lld  per:%d/sec", frame_num, et.elapsed(), frame_num * 1000 / et.elapsed());
    qDebug() << "VideoDecodeThread exited";
}
