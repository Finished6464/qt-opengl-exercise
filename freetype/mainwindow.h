#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>

#include <ft2build.h>
#include FT_FREETYPE_H

class QOpenGLShaderProgram;
class QOpenGLVertexArrayObject;
class QOpenGLBuffer;
struct OverlayTextChar_;


//typedef QHash<ushort, struct Character_Parameter_*> CharacterParamterTable;

class FreeTypeOpenGlContext
{
public:
    FreeTypeOpenGlContext();
    virtual ~FreeTypeOpenGlContext();

    static int InitLib();
    static void ReleaseLib();

    int Load(const char*  filepathname);
    void Unload();

    void Render(const QString& text, GLfloat x, GLfloat y, int font_height, GLfloat scale, const QColor &color);
    void Render(const ushort* charcodes, int size, GLfloat x, GLfloat y, int font_height, GLfloat scale, const QColor &color);

//private:
//    int GetCharacterParameter(ushort char_code, struct Character_Parameter_** param);

private:
    static FT_Library ft_lib_;
    FT_Face ft_face_;

    QOpenGLShaderProgram *program_;
    QOpenGLVertexArrayObject *vao_;
    QOpenGLBuffer *vbo_;

//    CharacterParamterTable char_param_table_;
};


class MainWindow : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void initializeGL();
    void paintGL();

private:
    FreeTypeOpenGlContext *ft_context_;
};
#endif // MAINWINDOW_H
