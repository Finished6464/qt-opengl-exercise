#include "mainwindow.h"
#include "../help.h"

#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>

MainWindow::MainWindow(QWidget *parent)
    : QOpenGLWidget(parent)
{
    program_ = nullptr;
//    ebo_ = nullptr;
//    vbo_ = nullptr;
    vao_ = nullptr;
    resize(600,480);
}

MainWindow::~MainWindow()
{
    makeCurrent();
    SAFE_DELETE(program_);
    SAFE_DELETE(vao_);
//    SAFE_DELETE(vbo_);
//    SAFE_DELETE(ebo_);
    doneCurrent();
}

void MainWindow::initializeGL()
{
    const char *vertexShaderSourceCore =
        "layout (location = 0) in vec3 aPos;\n"   // 位置变量的属性位置值为 0
        "layout (location = 1) in vec3 aColor;\n" // 颜色变量的属性位置值为 1
        "out vec3 ourColor;\n" // 向片段着色器输出一个颜色
        "void main()\n"
        "{\n"
        "    gl_Position = vec4(aPos, 1.0);\n"
        "    ourColor = aColor;\n" // 将ourColor设置为我们从顶点数据那里得到的输入颜色
        "}\n";

    const char *fragmentShaderSourceCore =
        "out vec4 FragColor;\n"
        "in vec3 ourColor;\n"
        "void main()\n"
        "{\n"
        "   FragColor = vec4(ourColor, 1.0);\n"
        "}\n";

    const float vertices[] = {
        // 位置              // 颜色
         0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,   // 右下
        -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,   // 左下
         0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f    // 顶部
    };

    QOpenGLBuffer *vbo;

    initializeOpenGLFunctions();

    SAFE_DELETE(program_);
    program_ = new QOpenGLShaderProgram;
    program_->addShaderFromSourceCode(QOpenGLShader::Vertex, versionedShaderCode(vertexShaderSourceCore));
    program_->addShaderFromSourceCode(QOpenGLShader::Fragment, versionedShaderCode(fragmentShaderSourceCore));
//  //测试使用GLSL内定义的location，不绑定
//    program_->bindAttributeLocation("aPos", 0);

    program_->link();
    program_->bind();

    SAFE_DELETE(vao_);
    vao_ = new QOpenGLVertexArrayObject;
    vao_->create();
    vao_->bind();

    vbo = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vbo->create();
    vbo->bind();
    vbo->setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbo->allocate(vertices, sizeof(vertices));

    // 参看着色器教程https://learnopengl-cn.github.io/01%20Getting%20started/05%20Shaders/
    // 步长为6 (vertices 顶点位置3个float + 颜色3个float)
    // 位置属性(顶点着色器代码内已定义 location = 0) layout (location = 0) in vec3 aPos;
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)0);

    // 颜色属性(顶点着色器代码内已定义 location = 1) layout (location = 1) in vec3 aColor;
    // 颜色的每组索引需偏移3个float(3个顶点)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(3* sizeof(GLfloat)));

    program_->release();
    vao_->release();
    delete vbo;
}

void MainWindow::paintGL()
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    vao_->bind();
    program_->bind();

    glDrawArrays(GL_TRIANGLES, 0, 3);

    program_->release();
    vao_->release();
}
