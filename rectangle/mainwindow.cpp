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
        "layout (location = 0) in vec3 aPos;\n"
        "void main() {\n"
        "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
        "}\n";

    const char *fragmentShaderSourceCore =
        "out vec4 FragColor;\n"
        "void main() {\n"
        "   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
        "}\n";


    const float vertices[] = {
        0.0f, 0.75f, 0.0f,   // 右上角
        0.0f, 0.0f, 0.0f,  // 右下角
       -0.75f, 0.0f, 0.0f, // 左下角
       -0.75f, 0.75f, 0.0f,   // 左上角

        0.5f, 0.0f, 0.0f,   // 右上角
        0.5f, -0.5f, 0.0f,  // 右下角
        0.0f, -0.5f, 0.0f, // 左下角
        0.0f, 0.0f, 0.0f   // 左上角
    };

    const unsigned int indices[] = { // 注意索引从0开始!
        0, 1, 3,  // 第1个三角形
        1, 2, 3,  // 第2个三角形

        4, 5, 7,  // 第3个三角形
        5, 6, 7   // 第4个三角形
    };

    QOpenGLBuffer *vbo, *ebo;

    initializeOpenGLFunctions();

    SAFE_DELETE(program_);
    program_ = new QOpenGLShaderProgram;
    program_->addShaderFromSourceCode(QOpenGLShader::Vertex, versionedShaderCode(vertexShaderSourceCore));
    program_->addShaderFromSourceCode(QOpenGLShader::Fragment, versionedShaderCode(fragmentShaderSourceCore));
    program_->bindAttributeLocation("aPos", 0);

    program_->link();
    program_->bind();
//    apos_id_ = program_->uniformLocation("aPos");
//    FragColor_id_ = program_->uniformLocation("FragColor");
//    color_location = program_->uniformLocation("color");

    SAFE_DELETE(vao_);
    vao_ = new QOpenGLVertexArrayObject;
    vao_->create();
    vao_->bind();

//    SAFE_DELETE(vbo_);
    vbo = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vbo->create();
    vbo->bind();
    vbo->setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbo->allocate(vertices, sizeof(vertices));

//    SAFE_DELETE(ebo_);
    ebo = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    ebo->create();
    ebo->bind();
    ebo->setUsagePattern(QOpenGLBuffer::StaticDraw);
    ebo->allocate(indices, sizeof(indices));

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);

    program_->release();
    vao_->release();
    //注意在VAO解绑之后再解绑EBO
    //参看https://learnopengl-cn.github.io/01%20Getting%20started/04%20Hello%20Triangle/#_10
    //其中红框内提到
    //---当目标是GL_ELEMENT_ARRAY_BUFFER的时候，VAO会储存glBindBuffer的函数调用。
    //---这也意味着它也会储存解绑调用，所以确保你没有在解绑VAO之前解绑索引数组缓冲，否则它就没有这个EBO配置了。
    //ebo_->release();
    //vbo_->release();
    delete ebo;
    delete vbo;
}

void MainWindow::paintGL()
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//    glEnable(GL_DEPTH_TEST);
//    glEnable(GL_CULL_FACE);

    //混合函数
//    glEnable(GL_BLEND);
//    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    vao_->bind();
    program_->bind();

    //第1个矩形
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    //第2个矩形
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void *)(6 * sizeof(GLfloat)));
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    program_->release();
    vao_->release();
}
