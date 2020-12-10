#include "mainwindow.h"
#include "../help.h"

#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>

MainWindow::MainWindow(QWidget *parent)
    : QOpenGLWidget(parent)
{
    program_ = nullptr;
    vbo_ = nullptr;
    vao_ = nullptr;
}

MainWindow::~MainWindow()
{
    makeCurrent();
    if (vbo_)
        delete vbo_;
    if (vao_)
        delete vao_;
    if (program_)
        delete program_;
    doneCurrent();
}

void MainWindow::initializeGL()
{
    const char *vertexShaderSourceCore =
        "layout (location = 0) in vec3 aPos;\n"
        "void main() {\n"
        "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
        "}\n";

    const char *fragmentShaderSourceCore =
        "out vec4 FragColor;\n"
        "void main() {\n"
        "   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
        "}\n";

    static float vertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f
    };

    initializeOpenGLFunctions();

    program_ = new QOpenGLShaderProgram;
    program_->addShaderFromSourceCode(QOpenGLShader::Vertex, versionedShaderCode(vertexShaderSourceCore));
    program_->addShaderFromSourceCode(QOpenGLShader::Fragment, versionedShaderCode(fragmentShaderSourceCore));
    program_->bindAttributeLocation("aPos", 0);
    program_->link();

    program_->bind();
    apos_id_ = program_->uniformLocation("aPos");
    FragColor_id_ = program_->uniformLocation("FragColor");

    vao_ = new QOpenGLVertexArrayObject;
    vao_->create();
    vao_->bind();

    vbo_ = new QOpenGLBuffer;
    vbo_->create();
    vbo_->bind();
    vbo_->setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbo_->allocate(vertices, sizeof(vertices));

//VAO有效情况的设定
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
//不使用vao，则需以下设定
//    program_->enableAttributeArray("aPos");
//    program_->setAttributeBuffer("aPos", GL_FLOAT, 0, 3, 3 * sizeof(GLfloat));

    vbo_->release();
    vao_->release();
    program_->release();
}

void MainWindow::paintGL()
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    vao_->bind();
    program_->bind();

    glDrawArrays(GL_TRIANGLES, 0, 3);

    program_->release();
    vao_->release();
}
