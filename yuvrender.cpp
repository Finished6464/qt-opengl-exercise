#include "yuvrender.h"

#define HELP_JUST_HEAD_
#include "help.h"

#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>
#include <QMatrix4x4>


static const char* YUV420P_VS = ""
  "uniform mat4 scale;\n"
  "\n"
  "const vec2 verts[4] = vec2[] (\n"
  "  vec2(-1.0,  1.0), \n" // top left
  "  vec2(-1.0, -1.0), \n" // bottom left
  "  vec2( 1.0,  1.0), \n" // top right
  "  vec2( 1.0, -1.0)  \n" // bottom right
  ");\n"
  "\n"
  "const vec2 texcoords[4] = vec2[] (\n"
  "  vec2(0.0, 0.0), \n" // bottom left
  "  vec2(0.0, 1.0), \n" // top left
  "  vec2(1.0, 0.0), \n" // bottom right
  "  vec2(1.0, 1.0)  \n" // top right
  "); \n"
  "\n"
  "out vec2 v_coord; \n"
  "\n"
  "void main() {\n"
  "   gl_Position = scale * vec4(verts[gl_VertexID],0,1);\n"
  "   v_coord = texcoords[gl_VertexID];\n"
  "}\n"
  "";


static const char* YUV420P_FS = ""
  "uniform sampler2D y_tex;\n"
  "uniform sampler2D u_tex;\n"
  "uniform sampler2D v_tex;\n"
  "in mediump vec2 v_coord;\n"
  "out mediump vec4 fragcolor;\n"
  "\n"
  "const mediump vec3 R_cf = vec3(1.164383,  0.000000,  1.596027);\n"
  "const mediump vec3 G_cf = vec3(1.164383, -0.391762, -0.812968);\n"
  "const mediump vec3 B_cf = vec3(1.164383,  2.017232,  0.000000);\n"
  "const mediump vec3 offset = vec3(-0.0625, -0.5, -0.5);\n"
  "\n"
  "void main() {\n"
  "  mediump float y = texture(y_tex, v_coord).r;\n"
  "  mediump float u = texture(u_tex, v_coord).r;\n"
  "  mediump float v = texture(v_tex, v_coord).r;\n"
  "  mediump vec3 yuv = vec3(y,u,v);\n"
  "  yuv += offset;\n"
  "  fragcolor = vec4(0.0, 0.0, 0.0, 1.0);\n"
  "  fragcolor.r = dot(yuv, R_cf);\n"
  "  fragcolor.g = dot(yuv, G_cf);\n"
  "  fragcolor.b = dot(yuv, B_cf);\n"
  "}\n"
  "";

YUVRender::YUVRender()
{
    program_ = nullptr;
    vao_ = nullptr;
    texture_y_ = nullptr;
    texture_u_ = nullptr;
    texture_v_ = nullptr;

    video_width_ = video_height_ = 0;

    is_built = false;
}

YUVRender::~YUVRender()
{
    Unbuild();
}

int YUVRender::Build(int video_width, int video_height)
{
    assert(video_width > 0 && video_height > 0);
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();

//    const char *vertexShaderSource =
//        "layout (location = 0) in vec4 posIn;\n"
//        "layout (location = 1) in vec2 textureIn;\n"
//        "out vec2 v_coord;\n"
//        "void main(void) {\n"
//        "    gl_Position = posIn;\n"
//        "    v_coord = textureIn;\n"
//        "}\n";

//    const char *fragmentShaderSource =
//        "in mediump vec2 textureCoord;\n"
//        "out mediump vec4 fragmentColor;\n"
//        "uniform sampler2D tex_y;\n"
//        "uniform sampler2D tex_u;\n"
//        "uniform sampler2D tex_v;\n"
//        "\n"
//        "const vec3 R_cf = vec3(1.164383,  0.000000,  1.596027);\n"
//        "const vec3 G_cf = vec3(1.164383, -0.391762, -0.812968);\n"
//        "const vec3 B_cf = vec3(1.164383,  2.017232,  0.000000);\n"
//        "const vec3 offset = vec3(-0.0625, -0.5, -0.5);\n"
//        "\n"
//        "void main(void) {\n"
//        "  float y = texture(tex_y, textureCoord).r;\n"
//        "  float u = texture(tex_u, textureCoord).r;\n"
//        "  float v = texture(tex_v, textureCoord).r;\n"
//        "  vec3 yuv = vec3(y,u,v);\n"
//        "  yuv += offset;\n"
//        "  fragmentColor = vec4(0.0, 0.0, 0.0, 1.0);\n"
//        "  fragmentColor.r = dot(yuv, R_cf);\n"
//        "  fragmentColor.g = dot(yuv, G_cf);\n"
//        "  fragmentColor.b = dot(yuv, B_cf);\n"
//        "}\n";



//    const GLfloat vertices[] = {
//      //---- 位置 ----    - 纹理坐标 -
//       -1.0f,  1.0f,     0.0f, 0.0f,   // 左上 -> 左下
//       -1.0f, -1.0f,     0.0f, 1.0f,   // 左下 -> 左上
//        1.0f,  1.0f,     1.0f, 0.0f,   // 右上 -> 右下
//        1.0f, -1.0f,     1.0f, 1.0f,   // 右下 -> 右上
//    };


//    QOpenGLBuffer vbo;

    Unbuild();

//    SAFE_DELETE(program_);
    program_ = new QOpenGLShaderProgram;
    program_->addShaderFromSourceCode(QOpenGLShader::Vertex, versionedShaderCode(YUV420P_VS));
    program_->addShaderFromSourceCode(QOpenGLShader::Fragment, versionedShaderCode(YUV420P_FS));
    program_->link();
    program_->bind();

//    SAFE_DELETE(vao_);
    vao_ = new QOpenGLVertexArrayObject;
    vao_->create();
    vao_->bind();

//    vbo.create();
//    vbo.bind();
//    vbo.setUsagePattern(QOpenGLBuffer::DynamicDraw);
//    vbo.allocate(vertices, sizeof(vertices));

//    f->glEnableVertexAttribArray(0);
//    f->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
//    f->glEnableVertexAttribArray(1);
//    f->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));

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
    f->glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, video_width, video_height, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
    texture_u_->bind();
    f->glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, video_width >> 1, video_height >> 1, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
    texture_v_->bind();
    f->glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, video_width >> 1, video_height >> 1, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

    program_->setUniformValue("y_tex", 0);
    program_->setUniformValue("u_tex", 1);
    program_->setUniformValue("v_tex", 2);

//    program_->release();
//    vao_->release();
//    texture_y_->release();
//    texture_u_->release();
//    texture_v_->release();

    video_width_ = video_width;
    video_height_ = video_height;

    is_built = true;

    Resize();

    return 0;
}

void YUVRender::Unbuild()
{
    SAFE_DELETE(program_);
    SAFE_DELETE(vao_);
    SAFE_DELETE(texture_y_);
    SAFE_DELETE(texture_u_);
    SAFE_DELETE(texture_v_);
    is_built = false;
}

void YUVRender::Resize()
{
    assert(video_width_ > 0 && video_height_ > 0);
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
    GLint viewport[4];

    QMatrix4x4 mt_scale;
    float scale, scale_x = 1, scale_y = 1;
    if (!is_built) {
        fprintf(stderr, "YUVRender has not been built\n");
        return;
    }

    f->glGetIntegerv(GL_VIEWPORT, viewport);
    if (viewport[2] <= 0 || viewport[3] <= 0) {
        fprintf(stderr, "get GL_VIEWPORT return error value\n");
        return;
    }

    scale_x = video_width_ > viewport[2] ? viewport[2] / (float)video_width_ : 1.0f;
    scale_y = video_height_ > viewport[3] ? viewport[3] / (float)video_height_ : 1.0f;
    scale = scale_x < scale_y ? scale_x : scale_y;
    qDebug("viewport: %d, %d", viewport[2], viewport[3]);
    qDebug("scale: %f, %f, %f", scale_x, scale_y, scale);

    mt_scale.scale(scale * video_width_ / viewport[2],
            scale * video_height_ / viewport[3],
            1);

    program_->bind();
    program_->setUniformValue("scale", mt_scale);
}

void YUVRender::SetupPixel(const unsigned char* const *data, const int *linesize)
{
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
    GLint old_row_len;
    if (!is_built) {
        fprintf(stderr, "YUVRender has not been built\n");
        return;
    }

    vao_->bind();
    program_->bind();

    f->glGetIntegerv(GL_UNPACK_ROW_LENGTH, &old_row_len);

    f->glPixelStorei(GL_UNPACK_ROW_LENGTH, linesize[0]);
    texture_y_->bind();
    f->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, video_width_, video_height_, GL_RED, GL_UNSIGNED_BYTE, data[0]);

    f->glPixelStorei(GL_UNPACK_ROW_LENGTH, linesize[1]);
    texture_u_->bind();
    f->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, video_width_ >> 1,video_height_ >> 1, GL_RED, GL_UNSIGNED_BYTE, data[1]);

    f->glPixelStorei(GL_UNPACK_ROW_LENGTH, linesize[2]);
    texture_v_->bind();
    f->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, video_width_ >> 1, video_height_ >> 1, GL_RED, GL_UNSIGNED_BYTE, data[2]);

    f->glPixelStorei(GL_UNPACK_ROW_LENGTH, old_row_len);
}

void YUVRender::SetupPixel(const unsigned char *yuv_buff)
{
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();

    if (!is_built) {
        fprintf(stderr, "YUVRender has not been built\n");
        return;
    }

    vao_->bind();
    program_->bind();

    texture_y_->bind();
    f->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, video_width_, video_height_, GL_RED, GL_UNSIGNED_BYTE, yuv_buff);

    texture_u_->bind();
    f->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, video_width_ >> 1,video_height_ >> 1, GL_RED, GL_UNSIGNED_BYTE, yuv_buff + video_width_ * video_height_);

    texture_v_->bind();
    f->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, video_width_ >> 1, video_height_ >> 1, GL_RED, GL_UNSIGNED_BYTE, yuv_buff + (video_width_ * video_height_ * 5 >> 2));
}

void YUVRender::Draw()
{
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
    if (!is_built) {
        fprintf(stderr, "YUVRender has not been built\n");
        return;
    }

    vao_->bind();
    program_->bind();

    f->glActiveTexture(GL_TEXTURE0);
    texture_y_->bind();

    f->glActiveTexture(GL_TEXTURE1);
    texture_u_->bind();

    f->glActiveTexture(GL_TEXTURE2);
    texture_v_->bind();

    f->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
