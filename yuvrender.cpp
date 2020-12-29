#include "yuvrender.h"

#define HELP_JUST_HEAD_
#include "help.h"

#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>
#include <QOpenGLFramebufferObject>

YUVRender::YUVRender()
{
    program_ = nullptr;
    vao_ = nullptr;
    texture_y_ = nullptr;
    texture_u_ = nullptr;
    texture_v_ = nullptr;

    width_ = height_ = 0;
}

YUVRender::~YUVRender()
{
    Unbuild();
}

int YUVRender::Build(int width, int height)
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
    texture_y_->bind();
    f->glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
    texture_u_->bind();
    f->glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width >> 1, height >> 1, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
    texture_v_->bind();
    f->glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width >> 1, height >> 1, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

    program_->setUniformValue("tex_y", 0);
    program_->setUniformValue("tex_u", 1);
    program_->setUniformValue("tex_v", 2);

    program_->release();
    vao_->release();
    texture_y_->release();
    texture_u_->release();
    texture_v_->release();

    width_ = width;
    height_ = height;

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

void YUVRender::Render(const unsigned char* const buff)
{
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
//    if (size < frame_width * frame_height * 3 >> 1) {
//        qDebug() << "yuv size too small";
//        return;
//    }

    vao_->bind();
    program_->bind();

    f->glActiveTexture(GL_TEXTURE0);
    texture_y_->bind();
//    f->glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, frame_width, frame_height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, buff);
    f->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_, GL_RED, GL_UNSIGNED_BYTE, buff);

    f->glActiveTexture(GL_TEXTURE1);
    texture_u_->bind();
//    f->glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, frame_width >> 1, frame_height >> 1,
//                     0, GL_LUMINANCE, GL_UNSIGNED_BYTE, buff + frame_width * frame_height);
    f->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_ >> 1,height_ >> 1, GL_RED, GL_UNSIGNED_BYTE, buff + width_ * height_);

    f->glActiveTexture(GL_TEXTURE2);
    texture_v_->bind();
//    f->glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, frame_width >> 1, frame_height >> 1,
//                     0, GL_LUMINANCE, GL_UNSIGNED_BYTE, buff + (frame_width * frame_height * 5 >> 2));
    f->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_ >> 1, height_ >> 1, GL_RED, GL_UNSIGNED_BYTE, buff + (width_ * height_ * 5 >> 2));


    f->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    program_->release();
    vao_->release();

    texture_y_->release();
    texture_u_->release();
    texture_v_->release();
}

void YUVRender::Render2(const unsigned char * const src_data[], const int src_linesize[])
{
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
//    if (size < frame_width * frame_height * 3 >> 1) {
//        qDebug() << "yuv size too small";
//        return;
//    }

    vao_->bind();
    program_->bind();

    f->glPixelStorei(GL_UNPACK_ROW_LENGTH, src_linesize[0]);
    f->glActiveTexture(GL_TEXTURE0);
    texture_y_->bind();
    f->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_, GL_RED, GL_UNSIGNED_BYTE, src_data[0]);

    f->glPixelStorei(GL_UNPACK_ROW_LENGTH, src_linesize[1]);
    f->glActiveTexture(GL_TEXTURE1);
    texture_u_->bind();
    f->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_ >> 1,height_ >> 1, GL_RED, GL_UNSIGNED_BYTE, src_data[1]);

    f->glPixelStorei(GL_UNPACK_ROW_LENGTH, src_linesize[2]);
    f->glActiveTexture(GL_TEXTURE2);
    texture_v_->bind();
    f->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_ >> 1, height_ >> 1, GL_RED, GL_UNSIGNED_BYTE, src_data[2]);

    f->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    program_->release();
    vao_->release();

    texture_y_->release();
    texture_u_->release();
    texture_v_->release();
}
