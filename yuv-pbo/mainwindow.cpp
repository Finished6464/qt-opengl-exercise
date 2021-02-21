#include "mainwindow.h"

#include <QApplication>
#include <QScreen>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>
#include <QFile>
#include <QMessageBox>
#include <QOpenGLExtraFunctions>
#include <QElapsedTimer>
#include <QThread>

#define SAFE_DELETE(p) do {if (p) {delete (p); (p) = nullptr;}} while(0)

const char* YUV_FILE_PATH = "bbb_sunflower_480_270.yuv";
const unsigned FRAME_WIDTH = 480; //video frame width, hardcoded for PoC
const unsigned FRAME_HEIGHT = 270; //video frame height, hardcoded for PoC
const unsigned FRAME_SIZE = FRAME_WIDTH * FRAME_HEIGHT * 3/2; //I420




MainWindow::MainWindow(QWidget *parent)
    : QOpenGLWidget(parent)
{
    program_ = nullptr;
    vao_ = nullptr;
    texture_y_ = nullptr;
    texture_u_ = nullptr;
    texture_v_ = nullptr;
    buff_ = nullptr;
    pbo_ = nullptr;

    resize(600,480);
    move(QApplication::screens().at(0)->geometry().center() - frameGeometry().center());
}

MainWindow::~MainWindow()
{
    makeCurrent();
    SAFE_DELETE(program_);
    SAFE_DELETE(vao_);
    SAFE_DELETE(pbo_);
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
        "#version 330\n"
        "layout (location = 0) in vec4 posIn;\n"
        "layout (location = 1) in vec2 textureIn;\n"
        "out vec2 textureCoord;\n"
        "void main(void) {\n"
        "    gl_Position = posIn;\n"
        "    textureCoord = textureIn;\n"
        "}\n";

    const char *fragmentShaderSource =
        "#version 330\n"
        "in mediump vec2 textureCoord;\n"
        "out mediump vec4 fragmentColor;\n"
        "uniform sampler2D tex_y;\n"
        "uniform sampler2D tex_u;\n"
        "uniform sampler2D tex_v;\n"
        "void main(void) {\n"
        "    mediump vec3 yuv;\n"
        "    mediump vec3 rgb;\n"
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
    program_->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    program_->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    program_->link();
    program_->bind();


    SAFE_DELETE(vao_);
    vao_ = new QOpenGLVertexArrayObject;
    vao_->create();
    vao_->bind();

//    SAFE_DELETE(vbo_);
    vbo = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vbo->create();
    vbo->bind();
    vbo->setUsagePattern(QOpenGLBuffer::StreamDraw);
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

    texture_y_->bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, FRAME_WIDTH, FRAME_HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, 0);
    texture_u_->bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, FRAME_WIDTH / 2, FRAME_HEIGHT / 2, 0, GL_RED, GL_UNSIGNED_BYTE, 0);
    texture_v_->bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, FRAME_WIDTH / 2, FRAME_HEIGHT / 2, 0, GL_RED, GL_UNSIGNED_BYTE, 0);


    QElapsedTimer et;

//#define  _USE_PBO
//#define  _USE_QT_PBO
#ifdef _USE_PBO
#ifdef _USE_QT_PBO
    SAFE_DELETE(pbo_);
    pbo_ = new QOpenGLBuffer(QOpenGLBuffer::PixelUnpackBuffer);
    pbo_->create();
    pbo_->bind();
    pbo_->setUsagePattern(QOpenGLBuffer::StreamDraw);
    pbo_->allocate(FRAME_SIZE);
#ifdef _USE_MAP
    void *bufPtr = pbo_->mapRange(0, FRAME_SIZE, QOpenGLBuffer::RangeWrite | QOpenGLBuffer::RangeInvalidateBuffer |  QOpenGLBuffer::RangeUnsynchronized);
    et.start();
    if (bufPtr) {
        memcpy(bufPtr, buff_, FRAME_SIZE);
        pbo_->unmap();
        qDebug("memcpy use time: %lld", et.elapsed());
    }
#else
    et.start();
    pbo_->write(0, buff_, FRAME_SIZE);
    qDebug("pbo_->write use time: %lld", et.elapsed());
#endif //_USE_MAP
#else
    GLuint pbo;
    glGenBuffers(1, &pbo);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, FRAME_SIZE, 0, GL_STREAM_DRAW);
    QOpenGLExtraFunctions* exf = QOpenGLContext::currentContext()->extraFunctions();
    GLubyte *bufPtr = (GLubyte *) exf->glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0,
                                                   FRAME_SIZE,
                                                   GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
    et.start();
    if (bufPtr) {
        memcpy(bufPtr, buff_, FRAME_SIZE);
        exf->glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
        qDebug("memcpy use time: %lld", et.elapsed());
    }
#endif   //_USE_QT_PBO
#endif   //_USE_MAP
    et.start();
    texture_y_->bind();
    qDebug("texture_y_->bind use time: %lld", et.elapsed());
#ifdef _USE_PBO
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, FRAME_WIDTH, FRAME_HEIGHT, GL_RED, GL_UNSIGNED_BYTE, 0);
#else
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, FRAME_WIDTH, FRAME_HEIGHT, GL_RED, GL_UNSIGNED_BYTE, buff_);
#endif
    qDebug("glTexSubImage2D y use time: %lld", et.elapsed());

    texture_u_->bind();
#ifdef _USE_PBO
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,  FRAME_WIDTH / 2, FRAME_HEIGHT / 2, GL_RED, GL_UNSIGNED_BYTE, (char*)(FRAME_WIDTH * FRAME_HEIGHT));
#else
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,  FRAME_WIDTH / 2, FRAME_HEIGHT / 2, GL_RED, GL_UNSIGNED_BYTE, buff_ + FRAME_WIDTH * FRAME_HEIGHT);
#endif

    texture_v_->bind();
#ifdef _USE_PBO
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,  FRAME_WIDTH / 2, FRAME_HEIGHT / 2, GL_RED, GL_UNSIGNED_BYTE, (char*)(FRAME_WIDTH * FRAME_HEIGHT * 5 / 4));
#else
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,  FRAME_WIDTH / 2, FRAME_HEIGHT / 2, GL_RED, GL_UNSIGNED_BYTE, buff_ + FRAME_WIDTH * FRAME_HEIGHT * 5 / 4);
#endif

    qDebug("use tim: %lld", et.elapsed());

    program_->setUniformValue("tex_y", 0);
    program_->setUniformValue("tex_u", 1);
    program_->setUniformValue("tex_v", 2);

//    program_->release();
    vao_->release();
//    texture_y_->release();
//    texture_u_->release();
//    texture_v_->release();
#if defined (_USE_PBO) && defined (_USE_QT_PBO)
    pbo_->release();
#else
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
#endif
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

        glActiveTexture(GL_TEXTURE1);
        texture_u_->bind();

        glActiveTexture(GL_TEXTURE2);
        texture_v_->bind();

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        vao_->release();
    }
}


