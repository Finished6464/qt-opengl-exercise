#ifndef TEXT_RENDER_H
#define TEXT_RENDER_H

#include <ft2build.h>
#include FT_FREETYPE_H

class QOpenGLShaderProgram;
class QOpenGLVertexArrayObject;
class QOpenGLBuffer;
class QString;
class QColor;

class TextRender
{
public:
    TextRender();
    virtual ~TextRender();

    static int InitLib();
    static void ReleaseLib();

    int Build(const char*  filepathname);
    void Unbuild();

    void Draw(const QString& text, float x, float y, int font_height, float scale, const QColor &color);
    void Draw(const ushort* charcodes, int size, float x, float y, int font_height, float scale, const QColor &color);

private:
    static FT_Library ft_lib_;
    FT_Face ft_face_;

    QOpenGLShaderProgram *program_;
    QOpenGLVertexArrayObject *vao_;
    QOpenGLBuffer *vbo_;
};


#endif // TEXT_RENDER_H
