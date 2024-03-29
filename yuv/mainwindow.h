#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>

class QOpenGLShaderProgram;
class QOpenGLVertexArrayObject;
class QOpenGLTexture;

class MainWindow :  public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void initializeGL();
    void paintGL();

private:
    QOpenGLShaderProgram *program_;
    QOpenGLVertexArrayObject *vao_;
    QOpenGLTexture *texture_y_;
    QOpenGLTexture *texture_u_;
    QOpenGLTexture *texture_v_;

    uchar* buff_;
    int buff_size_;
};
#endif // MAINWINDOW_H
