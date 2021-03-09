#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>



class QOpenGLShaderProgram;
class QOpenGLVertexArrayObject;
class QOpenGLTexture;
class QOffscreenSurface;


class GlWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    GlWidget(QWidget *parent = nullptr);

    GLuint textures_[3];

private:
    void initializeGL() override;
    void paintGL() override;
};

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void showEvent(QShowEvent *event) override;

private:
//    QOpenGLShaderProgram *program_;
//    QOpenGLVertexArrayObject *vao_;
//    QOpenGLBuffer *vbo_;
//    QOpenGLTexture *texture_y_;
//    QOpenGLTexture *texture_u_;
//    QOpenGLTexture *texture_v_;

//    uchar* buff_;
//    int buff_size_;

    GlWidget* glwidget_;
    bool is_initiated_;

//    QOffscreenSurface* surface_;
//    QOpenGLContext* context_;
};
#endif // MAINWINDOW_H
