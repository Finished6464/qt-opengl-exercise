#include "mainwindow.h"
#include <QDebug>
#include <QMessageBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QElapsedTimer>
#include <QSlider>
#include <QMutex>

#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>


//#define BUFF_ALIGN 4


MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
{
    QBoxLayout *hlayout, *vlayout;
    QPushButton *btn;

    vlayout = new QVBoxLayout;

    video_widget_ = new VideoWidget(this);
    vlayout->addWidget(video_widget_);

    QSlider* slider = new QSlider(Qt::Horizontal);
    slider->setRange(1, 100);    
    vlayout->addWidget(slider);

    hlayout = new QHBoxLayout;
    btn  = new QPushButton("Open");
    QObject::connect(btn, &QPushButton::clicked, this, &MainWindow::OnOpenBtnClicked);
    hlayout->addWidget(btn);
    btn  = new QPushButton("Close");
    QObject::connect(btn, &QPushButton::clicked, this, &MainWindow::OnCloseBtnClicked);
    hlayout->addWidget(btn);
    btn  = new QPushButton("<< 5s");
    QObject::connect(btn, &QPushButton::clicked, this, [=]() {
        decode_thread_->Seek(-5);
    });
    hlayout->addWidget(btn);
    btn  = new QPushButton(">> 5s");
    QObject::connect(btn, &QPushButton::clicked, this, [=]() {
        decode_thread_->Seek(5);
    });
    hlayout->addWidget(btn);

    hlayout->addStretch(1);
    vlayout->addLayout(hlayout);
//    vlayout->setStretchFactor(video_widget_, 1);

    this->setLayout(vlayout);

    video_decoder_ = new VideoDecoder;


    decode_thread_ = new VideoDecodeThread(video_decoder_, this);
    QObject::connect(decode_thread_, &VideoDecodeThread::frameready, video_widget_, &VideoWidget::OnRedraw);
    QObject::connect(decode_thread_, &VideoDecodeThread::frameready, slider,
                     [=](const unsigned char* const *, const int *, int) {
        slider->setValue(video_decoder_->GetDuration() > 0 ? video_decoder_->GetFrameTime() * 100 / video_decoder_->GetDuration() : 0);
    });


    this->resize(600,480);
}

MainWindow::~MainWindow()
{
    delete decode_thread_;
    delete video_decoder_;
}

void MainWindow::OnOpenBtnClicked()
{
    if (video_decoder_->LoadFromFile("/home/kai/Videos/bbb_sunflower_1080p_60fps_30sec.mp4") != 0) {
        QMessageBox::critical(this, "error", "load failed");
        return;
    }

    video_widget_->BuildRender(video_decoder_->GetFrameWidth(), video_decoder_->GetFrameHeight());

    decode_thread_->start();
}

void MainWindow::OnCloseBtnClicked()
{
    decode_thread_->Stop();
    video_decoder_->Unload();
}




VideoWidget::VideoWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{
    yuv_render_ = new YUVRender;
    text_render_ = new TextRender;
}

VideoWidget::~VideoWidget()
{
    makeCurrent();
    delete text_render_;
    delete yuv_render_;
    doneCurrent();
}

void VideoWidget::BuildRender(int video_width, int video_height)
{
    makeCurrent();
    yuv_render_->Build(video_width, video_height);
    text_render_->Build("test.ttf");
}

void VideoWidget::OnRedraw(const unsigned char* const *data, const int *linesize, int, double t)
{
    makeCurrent();
    yuv_render_->SetupPixel(data, linesize);
    time_str_ = QString("%1:%2:%3").arg(
                (int)(t / 3600), 2, 10, QLatin1Char('0')).arg(
                (int)(t / 60), 2, 10, QLatin1Char('0')).arg(
                (int)t, 2, 10, QLatin1Char('0'));
//    doneCurrent();
    update();
}

void VideoWidget::initializeGL()
{
    initializeOpenGLFunctions();
}

void VideoWidget::paintGL()
{

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    yuv_render_->Draw();

//    glViewport(0, 0, this->width(), this->height());

    if (!time_str_.isEmpty())
        text_render_->Draw(time_str_, size().width() - 150, 0.0f, 32, 1.0f, QColor(Qt::red));

//    ushort charcodes[] = {0x36, 0x37, 0xe001, 0xe002};
//    text_render_->Render(charcodes, sizeof(charcodes) / sizeof(charcodes[0]), 10.0f, 200.0f, 96, 2.0f, QColor(128, 255, 0));
}

void VideoWidget::resizeGL(int w, int h)
{
    if (yuv_render_->IsBulit()) {
//        qDebug() << "yuv_render_->Resize()";
//        yuv_render_->Resize();
    }
}

VideoDecodeThread::VideoDecodeThread(VideoDecoder *decoder, QObject *parent)
    :QThread(parent)
{
    video_decoder_ = decoder;
    mutex_ = new QMutex;
    et_ = new QElapsedTimer;
    start_time_ = 0;
}

VideoDecodeThread::~VideoDecodeThread()
{
    Stop();
    delete mutex_;
    delete et_;
}

void VideoDecodeThread::Stop()
{
    requestInterruption();
    wait();
}

void VideoDecodeThread::Seek(int sec)
{
    mutex_->lock();
    double t = video_decoder_->GetFrameTime();
    t += sec;
    if (t < 0)
        t = 0;
    else if (t > video_decoder_->GetDuration())
        t = video_decoder_->GetDuration() - 1;
    video_decoder_->SeekFrame(t);
    start_time_ = video_decoder_->GetFrameTime();
    et_->start();
    mutex_->unlock();
}

void VideoDecodeThread::run()
{
    const unsigned char* const *data;
    const int *linesize;
    int num, ret, frame_num = 0;
    QElapsedTimer debug_et;

    debug_et.start();

    mutex_->lock();
    start_time_ = 0;
    et_->start();
    mutex_->unlock();
//    et.start();
//    qDebug("duration time: %f", video_decoder_->GetDuration());
    while (!this->isInterruptionRequested()) {
        if (!mutex_->tryLock())
            continue;
        ret = video_decoder_->ReadFrame(&data, &linesize, &num);
        mutex_->unlock();
        frame_num++;
//        qDebug("frame time: %f", video_decoder_->GetFrameTime());
        emit frameready(data, linesize, num, video_decoder_->GetFrameTime());

        if (ret != 0)
            break;

        while (!this->isInterruptionRequested()) {
            if (!mutex_->tryLock())
                continue;
            mutex_->unlock();
            qint64 v = et_->elapsed();
            if (start_time_ * 1000 + et_->elapsed() > (video_decoder_->GetFrameTime() - 0.3) * 1000) {
//                qDebug("%d * 1000 + %lld > (%f - 0.3) * 1000", start_sec_, et_->elapsed(), frame_time);
                break;
            }
            QThread::msleep(10);
        }
    }

    mutex_->lock();
    qDebug("frame: %d time: %lld  per:%d/sec", frame_num, debug_et.elapsed(), frame_num * 1000 / (debug_et.elapsed()));
//    qDebug("total time: %lld", et.elapsed());
    qDebug() << "VideoDecodeThread exited";
    mutex_->unlock();
}
