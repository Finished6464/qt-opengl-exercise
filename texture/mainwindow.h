#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>

class QOpenGLShaderProgram;
class QOpenGLVertexArrayObject;
class QOpenGLTexture;



class ShaderProgram
{
public:
    ShaderProgram();
    virtual ~ShaderProgram();

    virtual void Init(QOpenGLFunctions *func) = 0;
    virtual void Render(QOpenGLFunctions *func) = 0;

protected:
    QOpenGLShaderProgram *program_;
    QOpenGLVertexArrayObject *vao_;
    QOpenGLTexture *texture_;
};

class TriangleShaderProgram : public ShaderProgram
{
public:
    TriangleShaderProgram();

    void Init(QOpenGLFunctions *func);
    void Render(QOpenGLFunctions *func);
};

class ContainerShaderProgram : public ShaderProgram
{
public:
    ContainerShaderProgram();

    void Init(QOpenGLFunctions *func);
    void Render(QOpenGLFunctions *func);
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
    ShaderProgram *triangle_shader_prog_;
    ShaderProgram *container_shader_prog_;
};
#endif // MAINWINDOW_H
