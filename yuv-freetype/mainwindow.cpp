#include "mainwindow.h"

#define HELP_JUST_HEAD_
#include "../help.h"


#include <QApplication>
#include <QScreen>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>
#include <QFile>
#include <QMessageBox>

const char* YUV_FILE_PATH = "bbb_sunflower_I420_320_180_25fps.yuv";
const unsigned FRAME_WIDTH = 320; //video frame width, hardcoded for PoC
const unsigned FRAME_HEIGHT = 180; //video frame height, hardcoded for PoC
//const unsigned FRAME_SIZE = FRAME_WIDTH * FRAME_HEIGHT * 3/2; //I420


MainWindow::MainWindow(QWidget *parent)
    : QOpenGLWidget(parent)
{
    text_render_ = new TextRender;
    yuv_render_ = new YUVRender;

    buff_ = nullptr;

    resize(600,480);
    move(QApplication::screens().at(0)->geometry().center() - frameGeometry().center());
}

MainWindow::~MainWindow()
{
    makeCurrent();
    delete text_render_;
    delete yuv_render_;
    doneCurrent();

    if (buff_) {
        free(buff_);
    }
}

void MainWindow::initializeGL()
{
    QFile f(YUV_FILE_PATH);
    if (f.open(QIODevice::ReadOnly)) {
        buff_size_ = f.size();
        buff_ = (unsigned char*)malloc(buff_size_ * sizeof(uchar));
        f.read((char *)buff_, buff_size_);
        f.close();
    }
    else {
        QMessageBox::critical(this, "error", "load yuv file failed");
    }

    initializeOpenGLFunctions();

    yuv_render_->Build();
    text_render_->Build("test.ttf");
}

void MainWindow::paintGL()
{
    QPoint pt = this->pos();
    QSize sz = QSize(FRAME_WIDTH, FRAME_HEIGHT);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (this->width() < sz.width() || this->height() < sz.height()) {
        sz.scale(this->width(), this->height(), Qt::KeepAspectRatio);
    }
    if (this->width() > sz.width()) {
        pt.setX((this->width() - sz.width()) / 2);
    }
    if (this->height() > sz.height()) {
        pt.setY((this->height() - sz.height()) / 2);
    }
    glViewport(pt.x(), pt.y(), sz.width(), sz.height());

    yuv_render_->Render((char*)buff_, buff_size_, FRAME_WIDTH, FRAME_HEIGHT);

    glViewport(0, 0, this->width(), this->height());

    text_render_->Render("12:53", 0.0f, 0.0f, 48, 1.0f, QColor(Qt::red));

    ushort charcodes[] = {0x36, 0x37, 0xe001, 0xe002};
    text_render_->Render(charcodes, sizeof(charcodes) / sizeof(charcodes[0]), 10.0f, 200.0f, 96, 2.0f, QColor(128, 255, 0));
}
