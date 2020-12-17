#include "yuvrender.h"

#define HELP_JUST_HEAD_
#include "../help.h"

#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>

YUVRender::YUVRender()
{
    program_ = nullptr;
    vao_ = nullptr;
    texture_y_ = nullptr;
    texture_u_ = nullptr;
    texture_v_ = nullptr;
}

YUVRender::~YUVRender()
{
    Unbuild();
}

int YUVRender::Build()
{
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
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

    const GLfloat vertices[] = {
      //---- 位置 ----    - 纹理坐标 -
       -1.0f,  1.0f,     0.0f, 0.0f,   // 左上 -> 左下
       -1.0f, -1.0f,     0.0f, 1.0f,   // 左下 -> 左上
        1.0f,  1.0f,     1.0f, 0.0f,   // 右上 -> 右下
        1.0f, -1.0f,     1.0f, 1.0f,   // 右下 -> 右上
    };

    QOpenGLBuffer vbo;

    Unbuild();

//    SAFE_DELETE(program_);
    program_ = new QOpenGLShaderProgram;
    program_->addShaderFromSourceCode(QOpenGLShader::Vertex, versionedShaderCode(vertexShaderSource));
    program_->addShaderFromSourceCode(QOpenGLShader::Fragment, versionedShaderCode(fragmentShaderSource));
    program_->link();
    program_->bind();

//    SAFE_DELETE(vao_);
    vao_ = new QOpenGLVertexArrayObject;
    vao_->create();
    vao_->bind();

    vbo.create();
    vbo.bind();
    vbo.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    vbo.allocate(vertices, sizeof(vertices));

    f->glEnableVertexAttribArray(0);
    f->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
    f->glEnableVertexAttribArray(1);
    f->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));

#define CREATE_TEXTURE_(p) \
    /*SAFE_DELETE(p);*/ \
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

    return 0;
}

void YUVRender::Unbuild()
{
    SAFE_DELETE(program_);
    SAFE_DELETE(vao_);
    SAFE_DELETE(texture_y_);
    SAFE_DELETE(texture_u_);
    SAFE_DELETE(texture_v_);
}

void YUVRender::Render(char* buff, int size, int frame_width, int frame_height)
{
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
    if (size < frame_width * frame_height * 3 / 2) {
        qDebug() << "yuv size too small";
        return;
    }

    vao_->bind();
    program_->bind();

    f->glActiveTexture(GL_TEXTURE0);
    texture_y_->bind();
    f->glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, frame_width, frame_height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, buff);

    f->glActiveTexture(GL_TEXTURE1);
    texture_u_->bind();
    f->glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, frame_width / 2, frame_height / 2,
                     0, GL_LUMINANCE, GL_UNSIGNED_BYTE, buff + frame_width * frame_height);

    f->glActiveTexture(GL_TEXTURE2);
    texture_v_->bind();
    f->glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, frame_width / 2, frame_height / 2,
                     0, GL_LUMINANCE, GL_UNSIGNED_BYTE, buff + frame_width * frame_height * 5 / 4);

    f->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    program_->release();
    vao_->release();

    texture_y_->release();
    texture_u_->release();
    texture_v_->release();
}
