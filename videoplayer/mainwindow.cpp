#include "mainwindow.h"
#include <QDebug>
#include <QMessageBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QElapsedTimer>
#include <QSlider>
#include <QLabel>
#include <QMutex>
#include <QTimer>
#include <QUuid>
#include <QFileDialog>

#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>

#define TWO_DIG_TIME_ARG(t) (int)(t), 2, 10, QLatin1Char('0')

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
{
    QBoxLayout *hlayout, *vlayout;
    QPushButton *btn;

    status_ = Closed;
    video_decoder_ = new VideoDecoder;
    vfr_thread_ = new VideoFrameReadThread(video_decoder_, this);
    QObject::connect(vfr_thread_, &VideoFrameReadThread::frameready, this, &MainWindow::OnFrameReady);
    QObject::connect(vfr_thread_, &VideoFrameReadThread::finished, this, [=]() {
        status_ = video_decoder_->IsLoad() ? Stopped : Closed; });

    vlayout = new QVBoxLayout;

    video_widget_ = new VideoWidget(this);
    vlayout->addWidget(video_widget_);

    hlayout = new QHBoxLayout;
    slider_ = new QSlider(Qt::Horizontal);
    slider_->setRange(1, 100);
    hlayout->addWidget(slider_);

    time_lbl_ = new QLabel("00:00");
    hlayout->addWidget(time_lbl_);

    vlayout->addLayout(hlayout);

    hlayout = new QHBoxLayout;
    load_btn_  = new QPushButton("Load...");
    QObject::connect(load_btn_, &QPushButton::clicked, this, &MainWindow::OnOpenBtnClicked);
    hlayout->addWidget(load_btn_);
    play_btn_  = new QPushButton("Play");
    QObject::connect(play_btn_, &QPushButton::clicked, this, &MainWindow::OnPlayBtnClicked);
    hlayout->addWidget(play_btn_);
    btn  = new QPushButton("<< 5s");
    QObject::connect(btn, &QPushButton::clicked, this, [=]() { vfr_thread_->Seek(-5); });
    hlayout->addWidget(btn);
    btn  = new QPushButton(">> 5s");
    QObject::connect(btn, &QPushButton::clicked, this, [=]() { vfr_thread_->Seek(5); });
    hlayout->addWidget(btn);

    hlayout->addStretch(1);
    vlayout->addLayout(hlayout);
    vlayout->setStretchFactor(video_widget_, 1);

    this->setLayout(vlayout);

//    QObject::connect(decode_thread_, &VideoDecodeThread::frameready, slider,
//                     [=](const unsigned char* const *, const int *, int) {
//        slider->setValue(video_decoder_->GetDuration() > 0 ? video_decoder_->GetFrameTime() * 100 / video_decoder_->GetDuration() : 0);
//    });

    timer_ = new QTimer(this);
    QObject::connect(timer_, &QTimer::timeout, this, &MainWindow::OnTimeout);
    timer_->start(1000);

    this->resize(600,480);
}

MainWindow::~MainWindow()
{
    delete vfr_thread_;
    delete video_decoder_;
    delete timer_;
}

void MainWindow::OnTimeout()
{
    QString time_str;
    double frame_time = 0;
    double duration = 0;

    timer_->stop();

    if (status_ == Closed) {
        slider_->setValue(slider_->minimum());
        time_lbl_->setText("00:00");
        load_btn_->setText("Load...");
        video_widget_->EnableDraw(false);
        play_btn_->setText("Play");
    }
    else {
        vfr_thread_->mutex()->lock();
        duration = video_decoder_->GetDuration();
        frame_time = status_ != Stopped ? video_decoder_->GetFrameTime() : duration;
        vfr_thread_->mutex()->unlock();

        if (duration >= 3600) {
            time_str = QString("%1:%2:%3/%4:%5:%6").arg(
                        TWO_DIG_TIME_ARG(frame_time / 3600)).arg(
                        TWO_DIG_TIME_ARG(frame_time / 60)).arg(
                        TWO_DIG_TIME_ARG((int)frame_time % 60)).arg(
                        TWO_DIG_TIME_ARG(duration / 3600)).arg(
                        TWO_DIG_TIME_ARG(duration / 60)).arg(
                        TWO_DIG_TIME_ARG((int)duration % 60));
        }
        else {
            time_str = QString("%1:%2/%3:%4").arg(
                        TWO_DIG_TIME_ARG(frame_time / 60)).arg(
                        TWO_DIG_TIME_ARG((int)frame_time % 60)).arg(
                        TWO_DIG_TIME_ARG(duration / 60)).arg(
                        TWO_DIG_TIME_ARG((int)duration % 60));
        }

        if (status_ == Playing) {
            video_widget_->SetStateChr(0);
            play_btn_->setText("Pause");
        }
        else {
            video_widget_->SetStateChr(status_ == Stopped ? 0xe001 : 0xe002);
            play_btn_->setText("Play");
        }

        slider_->setValue(duration > 0 ? frame_time * 100 / duration : 0);
        time_lbl_->setText(time_str);
        video_widget_->SetTime(frame_time);
        video_widget_->EnableDraw(true);
        load_btn_->setText("Close");
    }
//    if (video_decoder_->IsLoad()) {
//        vfr_thread_->mutex()->lock();
//        frame_time = video_decoder_->GetFrameTime();
//        duration = video_decoder_->GetDuration();
//        vfr_thread_->mutex()->unlock();

//        if (duration >= 3600) {
//            time_str = QString("%1:%2:%3/%4:%5:%6").arg(
//                        TWO_DIG_TIME_ARG(frame_time / 3600)).arg(
//                        TWO_DIG_TIME_ARG(frame_time / 60)).arg(
//                        TWO_DIG_TIME_ARG(frame_time)).arg(
//                        TWO_DIG_TIME_ARG(duration / 3600)).arg(
//                        TWO_DIG_TIME_ARG(duration / 60)).arg(
//                        TWO_DIG_TIME_ARG(duration));
//        }
//        else {
//            time_str = QString("%1:%2/%3:%4").arg(
//                        TWO_DIG_TIME_ARG(frame_time / 60)).arg(
//                        TWO_DIG_TIME_ARG(frame_time)).arg(
//                        TWO_DIG_TIME_ARG(duration / 60)).arg(
//                        TWO_DIG_TIME_ARG(duration));
//        }

//        slider_->setValue(duration > 0 ? frame_time * 100 / duration : 0);
//        time_lbl_->setText(time_str);
//        video_widget_->SetTime(frame_time);
//    }
    timer_->start(500);
}

void MainWindow::OnOpenBtnClicked()
{
    if (vfr_thread_->isRunning()) {
        vfr_thread_->Stop();
        video_decoder_->Unload();
        status_ = Closed;
    }
    else {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Open Video"), "/home");
        if (fileName.isNull())
            return;
        if (video_decoder_->LoadFromFile(fileName.toUtf8()) != 0) {
            QMessageBox::critical(this, "error", "load failed");
            return;
        }

        video_widget_->BuildRender(video_decoder_->GetFrameWidth(), video_decoder_->GetFrameHeight(), "test.ttf");

        OnPlayBtnClicked();
    }
}

void MainWindow::OnPlayBtnClicked()
{
    if (!video_decoder_->IsLoad())
        return;
    if (!vfr_thread_->isRunning()) {
        if (video_decoder_->GetFrameTime() > 0)
            video_decoder_->SeekFrame(0);
        vfr_thread_->Start();
        status_ = Playing;
    }
    else {
        status_ = status_ == Playing ? Paused : Playing;
        vfr_thread_->Pause(status_ != Playing);
    }
}

void MainWindow::OnFrameReady(uint uuid_hash, const unsigned char* const *data, const int *linesize, int num)
{
    if (vfr_thread_->uuid_hash() != uuid_hash) {
        qDebug("uuid_hash different: %u %u", vfr_thread_->uuid_hash(), uuid_hash);
        return;
    }
    video_widget_->SetYuvData(data, linesize, num);
    video_widget_->update();
}


VideoWidget::VideoWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{
    yuv_render_ = new YUVRender;
    text_render_ = new TextRender;
    draw_enabled_ = false;
    state_chr_ = 0;
}

VideoWidget::~VideoWidget()
{
    makeCurrent();
    delete text_render_;
    delete yuv_render_;
    doneCurrent();
}

void VideoWidget::BuildRender(int yuv_width, int yuv_height, const char* font_filename)
{
    makeCurrent();
    yuv_render_->Build(yuv_width, yuv_height);
    text_render_->Build(font_filename);
}

void VideoWidget::BuildYuvRender(int yuv_width, int yuv_height)
{
    makeCurrent();
    yuv_render_->Build(yuv_width, yuv_height);
}

void VideoWidget::BuildTextRender(const char* font_filename)
{
    makeCurrent();
    text_render_->Build(font_filename);
}

void VideoWidget::SetYuvData(const unsigned char* const *data, const int *linesize, int)
{
    makeCurrent();
    yuv_render_->SetupPixel(data, linesize);
}

void VideoWidget::SetTime(double t)
{
    time_str_ = QString("%1:%2:%3").arg(
                (int)(t / 3600), 2, 10, QLatin1Char('0')).arg(
                (int)(t / 60), 2, 10, QLatin1Char('0')).arg(
                (int)t % 60, 2, 10, QLatin1Char('0'));
}

void VideoWidget::initializeGL()
{
    initializeOpenGLFunctions();
}

void VideoWidget::paintGL()
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (draw_enabled_) {
        yuv_render_->Draw();

        if (!time_str_.isEmpty()) {
            text_render_->Draw(time_str_, size().width() - 150, 0.0f, 32, 1.0f, QColor(Qt::red));
        }

        if (state_chr_ > 0) {
            text_render_->Draw(&state_chr_, 1, size().width() / 2 - 100, size().height() / 2 - 50, 96, 2.0f, QColor(255, 255, 255));
        }
    }

}

void VideoWidget::resizeGL(int, int)
{
    if (yuv_render_->IsBulit()) {
//        qDebug() << "yuv_render_->Resize()";
        yuv_render_->Resize();
    }
}

VideoFrameReadThread::VideoFrameReadThread(VideoDecoder *video_decoder, QObject *parent)
    :QThread(parent)
{
    video_decoder_ = video_decoder;
    mutex_ = new QMutex;
    et_ = new QElapsedTimer;
    start_time_ = 0;
    paused_ = false;
    uuid_hash_ = 0;
}

VideoFrameReadThread::~VideoFrameReadThread()
{
    Stop();
    delete mutex_;
    delete et_;
}

void VideoFrameReadThread::Start()
{
    Stop();

    paused_ = false;
    uuid_hash_ = qHash(QUuid::createUuid());
    start();
}

void VideoFrameReadThread::Stop()
{
    requestInterruption();
    wait();
    uuid_hash_ = 0;
}

void VideoFrameReadThread::Pause(bool pause)
{
    mutex_->lock();
    paused_ = pause;
    if (!pause) {
        start_time_ = video_decoder_->GetFrameTime();
        et_->start();
    }
    mutex_->unlock();
}

void VideoFrameReadThread::Seek(int sec)
{
    if (!isRunning())
        return;

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

void VideoFrameReadThread::run()
{
    const unsigned char* const *data;
    const int *linesize;
    int num, ret, frame_num = 0;
    double frame_time = 0;
    QElapsedTimer debug_et;
//    uint uuid_hash = qHash(QUuid::createUuid());

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

        if (IsPaused()) {
            mutex_->unlock();
            continue;
        }

        ret = video_decoder_->ReadFrame(&data, &linesize, &num);
        if (ret == 0 || ret == AVERROR_EOF) {
            frame_time = video_decoder_->GetFrameTime();
            frame_num++;
            mutex_->unlock();
            emit frameready(uuid_hash_, data, linesize, num);
        }
        else {
            mutex_->unlock();
        }

        if (ret != 0)
            break;

        while (!this->isInterruptionRequested()) {
//            qint64 v = et_->elapsed();
            if (start_time_ * 1000 + et_->elapsed() > frame_time * 1000) {
//                qDebug("%d * 1000 + %lld > (%f - 0.3) * 1000", start_sec_, et_->elapsed(), frame_time);
                break;
            }
            QThread::msleep(10);
        }
    }

    qDebug("frame: %d time: %lld  per:%d/sec", frame_num, debug_et.elapsed(), frame_num * 1000 / debug_et.elapsed());
//    qDebug("total time: %lld", et.elapsed());
    qDebug() << "VideoFrameReadThread finish";
}
