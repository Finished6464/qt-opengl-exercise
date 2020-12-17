#include "textrender.h"
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>
#include <QOpenGLFunctions>
//#include <QVector2D>
//#include <QMessageBox>
#include <QColor>

#define HELP_JUST_HEAD_
#include "help.h"

struct CharacterContext_ {
    QOpenGLTexture *texture;  // 字形纹理
//    QVector2D  *size;       // 字形大小
//    QVector2D  *bearing;    // 从基准线到字形左部/顶部的偏移值
    uint width;  //位图宽度（像素）
    uint height;    //位图高度（像素）
    int bearing_x;  //水平距离，即位图相对于原点的水平位置（像素）
    int bearing_y;  //垂直距离，即位图相对于基准线的垂直位置（像素）
    long advance;    // 原点距下一个字形原点的距离
};

int create_char_context(ushort char_code, struct CharacterContext_* ctx, FT_Face ft_face)
{
    int err = 0;
    memset(ctx, 0, sizeof(struct CharacterContext_));

    // 加载字符的字形
    err = FT_Load_Char(ft_face, char_code, FT_LOAD_RENDER);
    if (err != 0) {
        qDebug("FT_Load_Char(%u) failed(%d)", char_code, err);
        return err;
    }

    ctx->texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    ctx->texture->setFormat(QOpenGLTexture::R8_UNorm);
    ctx->texture->setSize(ft_face->glyph->bitmap.width, ft_face->glyph->bitmap.rows);
    ctx->texture->allocateStorage();
    ctx->texture->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, (const void*)ft_face->glyph->bitmap.buffer);
    ctx->texture->setMagnificationFilter(QOpenGLTexture::Linear);
    ctx->texture->setMinificationFilter(QOpenGLTexture::Linear);
    ctx->texture->setWrapMode(QOpenGLTexture::ClampToEdge);
    ctx->width = ft_face->glyph->bitmap.width;
    ctx->height = ft_face->glyph->bitmap.rows;
    ctx->bearing_x = ft_face->glyph->bitmap_left;
    ctx->bearing_y = ft_face->glyph->bitmap_top;
    ctx->advance = ft_face->glyph->advance.x;
    return err;
}


FT_Library TextRender::ft_lib_ = nullptr;

TextRender::TextRender()
{
    ft_face_ = nullptr;
    program_ = nullptr;
    vao_ = nullptr;
    vbo_ = nullptr;
}

TextRender::~TextRender()
{
    Unbuild();
}

int TextRender::InitLib()
{
    int err = FT_Init_FreeType(&ft_lib_);
    if (err != 0) {
        qCritical("FT_Init_FreeType failed(%d)", err);
    }

    return err;
}

void TextRender::ReleaseLib()
{
    if (ft_lib_) {
        FT_Done_FreeType(ft_lib_);
        ft_lib_ = nullptr;
    }
}

int TextRender::Build(const char*  filepathname)
{
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
    int err = 0;
    const char *vertexShaderSource =
        "layout (location = 0) in vec4 vertexIn;\n"
        "layout (location = 1) in vec2 textureIn;\n"
        "uniform mat4 projection;\n"
        "out vec2 textCoord;\n"
        "void main(void) {\n"
        "    gl_Position = projection * vertexIn;\n"
        "    textCoord = textureIn;\n"
        "}\n";
    const char *fragmentShaderSource =
        "in vec2 textCoord;\n"
        "out vec4 fragmentColor;\n"
        "uniform sampler2D text;\n"
        "uniform vec4 textColor;\n"
        "void main(void) {\n"
        "    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, textCoord).r);\n"
        "    fragmentColor = textColor * sampled;\n"
        "}\n";

    Unbuild();

    err = FT_New_Face(ft_lib_, filepathname, 0, &ft_face_);
    if (err != 0) {
        qCritical("FT_New_Face failed(%d)", err);
        return err;
    }

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

//    SAFE_DELETE(vbo_);
    vbo_ = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vbo_->create();
    vbo_->bind();
    vbo_->allocate(sizeof(GLfloat) * 4 * 4);
    vbo_->setUsagePattern(QOpenGLBuffer::DynamicDraw);

    f->glEnableVertexAttribArray(0);
    f->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
    f->glEnableVertexAttribArray(1);
    f->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2* sizeof(GLfloat)));

    program_->setUniformValue("text", 0); //纹理采样单元

    program_->release();
    vbo_->release();
    vao_->release();

    return err;
}

void TextRender::Unbuild()
{
    SAFE_DELETE(program_);
    SAFE_DELETE(vao_);
    SAFE_DELETE(vbo_);

    if (ft_face_) {
        FT_Done_Face(ft_face_);
        ft_face_ = nullptr;
    }
}

void TextRender::Render(const QString& text, GLfloat x, GLfloat y, int font_height, GLfloat scale, const QColor &color)
{
    int size = text.size();
    if (size > 0) {
        ushort* data = (ushort*)malloc(sizeof(ushort) * size);
        for (int i = 0; i < text.size(); i++) {
            data[i] = text.at(i).unicode();
        }
        Render(data, size, x, y, font_height, scale, color);
        free(data);
    }
}

void TextRender::Render(const ushort* charcodes, int size, GLfloat x, GLfloat y, int font_height, GLfloat scale, const QColor &color)
{
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
    QMatrix4x4 matrix;
    int i, err;
    GLfloat w, h, offset_x, offset_y;
    GLfloat vertices[] = {
       0.0, 0.0,   0.0, 0.0,
       0.0, 0.0,   1.0, 0.0,
       0.0, 0.0,   0.0, 1.0,
       0.0, 0.0,   1.0, 1.0
    };
    GLboolean old_blend_enabled;
    GLint old_aligment;
    GLint aiViewport[4];
//    struct CharacterContext_* char_contexts = nullptr;
    struct CharacterContext_ ch_ctx;

    if (!ft_face_) {
        qDebug() << "FT_Face instance invalid";
        return;
    }

    if (!charcodes || size <= 0)
        return;

    err = FT_Set_Pixel_Sizes(ft_face_, 0, font_height);
    if (err != 0) {
        qCritical("FT_New_Face failed(%d)", err);
    }

    f->glGetIntegerv(GL_VIEWPORT, aiViewport);
    qDebug("%d, %d %d, %d", aiViewport[0], aiViewport[1], aiViewport[2], aiViewport[3]);
    matrix.ortho(
            aiViewport[0],
            aiViewport[0] + aiViewport[2],
            aiViewport[1] + aiViewport[3],
            aiViewport[1],
            -1, 1
            );

    f->glGetIntegerv(GL_UNPACK_ALIGNMENT, &old_aligment);
    f->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    old_blend_enabled = f->glIsEnabled(GL_BLEND);
    f->glEnable(GL_BLEND);
    f->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    f->glActiveTexture(GL_TEXTURE0);

    vao_->bind();
    program_->bind();
    program_->setUniformValue("projection", matrix);
    program_->setUniformValue("textColor", color);

//    char_contexts = (struct CharacterContext_*)malloc(sizeof(struct CharacterContext_) * size);
//    for (i = 0; i < size; i++) {
//        create_char_context(charcodes[i], char_contexts + i, ft_face_);
//    }

    for (i = 0; i < size; i++) {
        ch_ctx.texture = nullptr;
        create_char_context(charcodes[i], &ch_ctx, ft_face_);
//        struct CharacterContext_* ch_ctx = char_contexts + i;
        if (ch_ctx.texture) {
            w = ch_ctx.width * scale;
            h = ch_ctx.height * scale;
            offset_x = x + ch_ctx.bearing_x * scale;
            offset_y = y + (ch_ctx.height - ch_ctx.bearing_y) * scale + font_height - h;

            vertices[0] = vertices[8] = offset_x;
            vertices[1] = vertices[5] = offset_y;
            vertices[4] = vertices[12] = offset_x + w;
            vertices[9] = vertices[13] = offset_y + h;

            ch_ctx.texture->bind();
            vbo_->bind();
            vbo_->write(0, vertices, sizeof(vertices));
//            f->glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
            vbo_->release();
            f->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            // Now advance cursors for next glyph (note that advance is number of 1/64 pixels)
            // Bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
            x += (ch_ctx.advance >> 6) * scale;
            ch_ctx.texture->release();
            delete ch_ctx.texture;
        }
    }

    program_->release();
    vao_->release();

    if (!old_blend_enabled)
        f->glDisable(GL_BLEND);
    f->glPixelStorei(GL_UNPACK_ALIGNMENT, old_aligment);

//    //free char_contexts
//    if (char_contexts) {
//        for (i = 0; i < size; i++) {
//            struct CharacterContext_* ch_ctx = char_contexts + i;
//            SAFE_DELETE(ch_ctx->texture);
//        }

//        free(char_contexts);
//    }
}
