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
#include <QOffscreenSurface>
#include <QOpenGLFramebufferObject>
#include <QVBoxLayout>
#include <QShowEvent>


#define SAFE_DELETE(p) do {if (p) {delete (p); (p) = nullptr;}} while(0)

const char* YUV_FILE_PATH = "bbb_sunflower_480_270.yuv";
const unsigned FRAME_WIDTH = 480; //video frame width, hardcoded for PoC
const unsigned FRAME_HEIGHT = 270; //video frame height, hardcoded for PoC
const unsigned FRAME_SIZE = FRAME_WIDTH * FRAME_HEIGHT * 3/2; //I420

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

GlWidget::GlWidget(QWidget *parent)
    :QOpenGLWidget(parent)
{
    memset(textures_, 0, sizeof(textures_));
}

void GlWidget::initializeGL()
{

}

void GlWidget::paintGL()
{
    initializeOpenGLFunctions();

    if (!textures_[0])
        return;

    QOpenGLShaderProgram program;
    program.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    program.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    program.link();
    program.bind();

    QOpenGLBuffer vbo(QOpenGLBuffer::VertexBuffer);
    vbo.create();
    vbo.bind();
    vbo.setUsagePattern(QOpenGLBuffer::StreamDraw);
    vbo.allocate(vertices, sizeof(vertices));

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));

    program.setUniformValue("tex_y", 0);
    program.setUniformValue("tex_u", 1);
    program.setUniformValue("tex_v", 2);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textures_[0]);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textures_[1]);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, textures_[2]);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

}

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
{
//    program_ = nullptr;
//    vao_ = nullptr;
//    texture_y_ = nullptr;
//    texture_u_ = nullptr;
//    texture_v_ = nullptr;
//    buff_ = nullptr;

    is_initiated_ =false;

//    surface_ = new QOffscreenSurface;
//    context_ = new QOpenGLContext(this);

    glwidget_ = new GlWidget(this);
    QVBoxLayout* layout = new QVBoxLayout;
    layout->addWidget(glwidget_);
    setLayout(layout);

    resize(600,480);
    move(QApplication::screens().at(0)->geometry().center() - frameGeometry().center());
}

MainWindow::~MainWindow()
{
//    makeCurrent();
//    SAFE_DELETE(program_);
//    SAFE_DELETE(vao_);
//    SAFE_DELETE(pbo_);
//    SAFE_DELETE(texture_y_);
//    SAFE_DELETE(texture_u_);
//    SAFE_DELETE(texture_v_);

//    doneCurrent();

//    if (buff_) {
//        free(buff_);
//    }

//    SAFE_DELETE(context_);
//    SAFE_DELETE(surface_);
}

void MainWindow::showEvent(QShowEvent *event)
{
    if (is_initiated_ || event->spontaneous())
        return;

//    qDebug() << QSurfaceFormat::defaultFormat();

    is_initiated_ = true;

    QFile f(YUV_FILE_PATH);
    if (!f.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(nullptr, "error", "load yuv file failed");
        return;
    }

    int buff_size = f.size();
    uchar* buff = (unsigned char*)malloc(buff_size * sizeof(uchar));
    f.read((char *)buff, buff_size);
    f.close();

    QOpenGLContext* share_cxt = glwidget_->context();

    QOffscreenSurface surface;
    surface.setFormat(share_cxt->format());
    surface.setScreen(share_cxt->screen());
    surface.create();
//    qDebug() << surface_->screen();

    QOpenGLContext context;
    context.setShareContext(share_cxt);
    context.create();
    context.makeCurrent(&surface);

    QOpenGLFramebufferObject fbo(FRAME_WIDTH, FRAME_HEIGHT);
    fbo.bind();
    fbo.addColorAttachment(FRAME_WIDTH / 2, FRAME_HEIGHT / 2);
    fbo.addColorAttachment(FRAME_WIDTH / 2, FRAME_HEIGHT / 2);

    QOpenGLFunctions func(&context);
    func.glBindTexture(GL_TEXTURE_2D, fbo.textures()[0]);
    func.glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, FRAME_WIDTH, FRAME_HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, buff);
    func.glBindTexture(GL_TEXTURE_2D, fbo.textures()[1]);
    func.glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, FRAME_WIDTH / 2, FRAME_HEIGHT / 2, 0, GL_RED, GL_UNSIGNED_BYTE, buff + FRAME_WIDTH * FRAME_HEIGHT);
    func.glBindTexture(GL_TEXTURE_2D, fbo.textures()[2]);
    func.glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, FRAME_WIDTH / 2, FRAME_HEIGHT / 2, 0, GL_RED, GL_UNSIGNED_BYTE, buff + FRAME_WIDTH * FRAME_HEIGHT * 5 / 4);

//    qDebug() << fbo.sizes();
    glwidget_->textures_[0] = fbo.takeTexture(0);
    glwidget_->textures_[1] = fbo.takeTexture(1);
    glwidget_->textures_[2] = fbo.takeTexture(2);

    GLenum status = func.glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        qWarning("glCheckFramebufferStatus return error(%d)", status);
    }

    free(buff);
}

