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
    program_ = nullptr;
    vao_ = nullptr;
    texture_y_ = nullptr;
    texture_u_ = nullptr;
    texture_v_ = nullptr;
    buff_ = nullptr;

    resize(600,480);
    move(QApplication::screens().at(0)->geometry().center() - frameGeometry().center());
}

MainWindow::~MainWindow()
{
    makeCurrent();
    SAFE_DELETE(program_);
    SAFE_DELETE(vao_);
    SAFE_DELETE(texture_y_);
    SAFE_DELETE(texture_u_);
    SAFE_DELETE(texture_v_);
    doneCurrent();

    if (buff_) {
        free(buff_);
    }
}

void MainWindow::initializeGL()
{
    const char *vertexShaderSource =
        "layout (location = 0) in vec4 posIn;\n"
        "layout (location = 1) in vec2 textureIn;\n"
        "out vec2 textureCoord;\n"
        "void main(void) {\n"
        "    gl_Position = posIn;\n"
        "    textureCoord = textureIn;\n"
        "}\n";

    const char *fragmentShaderSource =
        "in vec2 textureCoord;\n"
        "out vec4 fragmentColor;\n"
        "uniform sampler2D tex_y;\n"
        "uniform sampler2D tex_u;\n"
        "uniform sampler2D tex_v;\n"
        "void main(void) {\n"
        "    vec3 yuv;\n"
        "    vec3 rgb;\n"
        "    yuv.x = texture(tex_y, textureCoord).r;\n"
        "    yuv.y = texture(tex_u, textureCoord).r - 0.5;\n"
        "    yuv.z = texture(tex_v, textureCoord).r - 0.5;\n"
        "    rgb = mat3( 1,       1,         1,\n"
        "                0,       -0.39465,  2.03211,\n"
        "                1.13983, -0.58060,  0) * yuv;\n"
        "    fragmentColor = vec4(rgb, 1.0);\n"
        "}\n";

    const float vertices[] = {
      //---- 位置 ----    - 纹理坐标 -
       -1.0f,  1.0f,     0.0f, 0.0f,   // 左上 -> 左下
       -1.0f, -1.0f,     0.0f, 1.0f,   // 左下 -> 左上
        1.0f,  1.0f,     1.0f, 0.0f,   // 右上 -> 右下
        1.0f, -1.0f,     1.0f, 1.0f,   // 右下 -> 右上
    };


    QOpenGLBuffer *vbo;
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

    SAFE_DELETE(program_);
    program_ = new QOpenGLShaderProgram;
    program_->addShaderFromSourceCode(QOpenGLShader::Vertex, versionedShaderCode(vertexShaderSource));
    program_->addShaderFromSourceCode(QOpenGLShader::Fragment, versionedShaderCode(fragmentShaderSource));
    program_->link();
    program_->bind();

    SAFE_DELETE(vao_);
    vao_ = new QOpenGLVertexArrayObject;
    vao_->create();
    vao_->bind();

    vbo = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vbo->create();
    vbo->bind();
    vbo->setUsagePattern(QOpenGLBuffer::DynamicDraw);
    vbo->allocate(vertices, sizeof(vertices));

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));

#define CREATE_TEXTURE_(p) \
    SAFE_DELETE(p); \
    p = new QOpenGLTexture(QOpenGLTexture::Target2D); \
    p->setWrapMode(QOpenGLTexture::DirectionS, QOpenGLTexture::ClampToEdge);\
    p->setWrapMode(QOpenGLTexture::DirectionT, QOpenGLTexture::ClampToEdge);\
    p->setMinificationFilter(QOpenGLTexture::Linear);\
    p->setMagnificationFilter(QOpenGLTexture::Linear);

    CREATE_TEXTURE_(texture_y_)
    CREATE_TEXTURE_(texture_u_)
    CREATE_TEXTURE_(texture_v_)
#undef CREATE_TEXTURE_

    program_->setUniformValue("tex_y", 0);
    program_->setUniformValue("tex_u", 1);
    program_->setUniformValue("tex_v", 2);

    program_->release();
    vao_->release();
    delete vbo;
}

void MainWindow::paintGL()
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (program_ && vao_ && buff_) {
        vao_->bind();
        program_->bind();

        glActiveTexture(GL_TEXTURE0);
        texture_y_->bind();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, FRAME_WIDTH, FRAME_HEIGHT, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, buff_);

        glActiveTexture(GL_TEXTURE1);
        texture_u_->bind();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, FRAME_WIDTH / 2, FRAME_HEIGHT / 2
                         , 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, (char*)buff_ + FRAME_WIDTH * FRAME_HEIGHT);

        glActiveTexture(GL_TEXTURE2);
        texture_v_->bind();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, FRAME_WIDTH / 2, FRAME_HEIGHT / 2
                         , 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, (char*)buff_ + FRAME_WIDTH * FRAME_HEIGHT * 5 / 4);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        program_->release();
        vao_->release();
        texture_y_->release();
        texture_u_->release();
        texture_v_->release();
    }
}


