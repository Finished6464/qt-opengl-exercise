#include "mainwindow.h"

#define HELP_JUST_HEAD_
#include "../help.h"

#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>


ShaderProgram::ShaderProgram()
{
    program_ = nullptr;
    vao_ = nullptr;
    texture_ = nullptr;
}


ShaderProgram::~ShaderProgram()
{
    SAFE_DELETE(program_);
    SAFE_DELETE(vao_);
    SAFE_DELETE(texture_);
}



TriangleShaderProgram::TriangleShaderProgram()
{
}


void TriangleShaderProgram::Init(QOpenGLFunctions *func)
{
    const char *vertexShaderSourceCore =
        "layout (location = 0) in vec3 aPos;\n"   // 位置变量的属性位置值为 0
        "layout (location = 1) in vec2 aTexCoord;\n" //纹理坐标的属性位置值为 1
        "out vec2 TexCoord;\n" // 向片段着色器输出一个纹理坐标
        "void main()\n"
        "{\n"
        "    gl_Position = vec4(aPos, 1.0);\n"
        "    TexCoord = aTexCoord;\n"
        "}\n";

    const char *fragmentShaderSourceCore =
        "out vec4 FragColor;\n"
        "in vec2 TexCoord;\n"
        "uniform sampler2D ourTexture;\n"
        "void main()\n"
        "{\n"
        "   FragColor = texture(ourTexture, TexCoord);\n" //输出就是纹理的（插值）纹理坐标上的(过滤后的)颜色
        "}\n";

    const float vertices[] = {
        //---- 位置 ----       - 纹理坐标 -
        -0.5f, 1.0f, 0.0f,   0.5f, 1.0f,   // 顶部
        -1.0f, 0.0f, 0.0f,   0.0f, 0.0f,   // 左下
         0.0f, 0.0f, 0.0f,   1.0f, 0.0f,   // 右下
    };

    QOpenGLBuffer *vbo;

    SAFE_DELETE(program_);
    program_ = new QOpenGLShaderProgram;
    program_->addShaderFromSourceCode(QOpenGLShader::Vertex, versionedShaderCode(vertexShaderSourceCore));
    program_->addShaderFromSourceCode(QOpenGLShader::Fragment, versionedShaderCode(fragmentShaderSourceCore));
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

    // 参看纹理教程https://learnopengl-cn.github.io/01%20Getting%20started/06%20Textures/#_6
    // 步长为5 (vertices 顶点位置3个float + 纹理坐标2个float)
    // 位置属性(顶点着色器代码内已定义 location = 0) layout (location = 0) in vec3 aPos;
    func->glEnableVertexAttribArray(0);
    func->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)0);

    // 纹理坐标属性(顶点着色器代码内已定义 location = 1) layout (location = 1) in vec3 aColor;
    // 纹理坐标的每组索引需偏移3个float(3个顶点位置)
    func->glEnableVertexAttribArray(1);
    func->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(3* sizeof(GLfloat)));

    texture_ = new QOpenGLTexture(QImage("wall.jpg"));
    // 为当前绑定的纹理对象设置环绕、过滤方式
    texture_->setWrapMode(QOpenGLTexture::DirectionS, QOpenGLTexture::Repeat);
    texture_->setWrapMode(QOpenGLTexture::DirectionT, QOpenGLTexture::Repeat);
    texture_->setMinificationFilter(QOpenGLTexture::Linear);
    texture_->setMagnificationFilter(QOpenGLTexture::Linear);

//    func->glUniform1i(program_->uniformLocation("ourTexture"), 0); // 手动设置
    program_->setUniformValue("ourTexture", 0); // 或者使用着色器类设置

    program_->release();
    vao_->release();
    delete vbo;
}

void TriangleShaderProgram::Render(QOpenGLFunctions *func)
{
//    Q_UNUSED( func )
    func->glActiveTexture(GL_TEXTURE0);
    texture_->bind();
    vao_->bind();
    program_->bind();

    func->glDrawArrays(GL_TRIANGLES, 0, 3);

    program_->release();
    vao_->release();
    texture_->release();
}


ContainerShaderProgram::ContainerShaderProgram()
{
}


void ContainerShaderProgram::Init(QOpenGLFunctions *func)
{
    const char *vertext_shader_source =
        "layout (location = 0) in vec3 aPos;\n"   // 位置变量的属性位置值为 0
        "layout (location = 1) in vec3 aColor;\n" // 颜色变量的属性位置值为 1
        "layout (location = 2) in vec2 aTexCoord;\n" //纹理坐标的属性位置值为 2
        "out vec3 ourColor;\n" // 向片段着色器输出一个颜色
        "out vec2 TexCoord;\n" // 向片段着色器输出一个纹理坐标
        "void main()\n"
        "{\n"
        "    gl_Position = vec4(aPos, 1.0);\n"
        "    ourColor = aColor;\n" // 将ourColor设置为我们从顶点数据那里得到的输入颜色
        "    TexCoord = aTexCoord;\n"
        "}\n";

    const char *fragment_shader_source =
        "out vec4 FragColor;\n"
        "in vec3 ourColor;\n"
        "in vec2 TexCoord;\n"
        "uniform sampler2D ourTexture;\n"
        "void main()\n"
        "{\n"
        //"   FragColor = texture(ourTexture, TexCoord);\n" //输出就是纹理的（插值）纹理坐标上的(过滤后的)颜色
        "    FragColor = texture(ourTexture, TexCoord) * vec4(ourColor, 1.0);\n"
        "}\n";


    const float vertices[] = {
         //---- 位置 ----       ---- 颜色 ----     - 纹理坐标 -
         1.0f,  0.0f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // 右上
         1.0f, -1.0f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   // 右下
         0.0f, -1.0f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // 左下
         0.0f,  0.0f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f    // 左上
     };

    const unsigned int indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };

    QOpenGLBuffer *vbo, *ebo;

    SAFE_DELETE(program_);
    program_ = new QOpenGLShaderProgram;
    program_->addShaderFromSourceCode(QOpenGLShader::Vertex, versionedShaderCode(vertext_shader_source));
    program_->addShaderFromSourceCode(QOpenGLShader::Fragment, versionedShaderCode(fragment_shader_source));
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

    ebo = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    ebo->create();
    ebo->bind();
    ebo->setUsagePattern(QOpenGLBuffer::StaticDraw);
    ebo->allocate(indices, sizeof(indices));

    // 参看纹理教程https://learnopengl-cn.github.io/01%20Getting%20started/06%20Textures/#_6
    // 步长为8 (vertices 顶点位置3个float + 颜色3个float + 纹理坐标2个float)
    // 位置属性(顶点着色器代码内已定义 location = 0) layout (location = 0) in vec3 aPos;
    func->glEnableVertexAttribArray(0);
    func->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)0);

    // 颜色属性(顶点着色器代码内已定义 location = 1) layout (location = 1) in vec3 aColor;
    // 颜色的每组索引需偏移3个float(3个顶点位置)
    func->glEnableVertexAttribArray(1);
    func->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(3* sizeof(GLfloat)));

    // 纹理坐标属性(顶点着色器代码内已定义 location = 1) layout (location = 1) in vec3 aColor;
    // 纹理坐标的每组索引需偏移6个float(3个顶点位置 + 3个颜色)
    func->glEnableVertexAttribArray(2);
    func->glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(6* sizeof(GLfloat)));

    SAFE_DELETE(texture_);
//    texture_ = new QOpenGLTexture(QOpenGLTexture::Target2D);
    texture_ = new QOpenGLTexture(QImage("container.jpg"));
    // 为当前绑定的纹理对象设置环绕、过滤方式
    texture_->setWrapMode(QOpenGLTexture::DirectionS, QOpenGLTexture::Repeat);
    texture_->setWrapMode(QOpenGLTexture::DirectionT, QOpenGLTexture::Repeat);
    texture_->setMinificationFilter(QOpenGLTexture::Linear);
    texture_->setMagnificationFilter(QOpenGLTexture::Linear);

//    img = new QImage("test2.jpg");
//    if (img->format() != QImage::Format_RGB888)
//        img->convertTo(QImage::Format_RGB888);

//    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img->width(), img->height(), 0, GL_RGB, GL_UNSIGNED_BYTE, img->constBits());
//    glGenerateMipmap(GL_TEXTURE_2D);

    func->glUniform1i(program_->uniformLocation("texture1"), 0); // 手动设置
//    func->glUniform1i(program_->uniformLocation("texture2"), 1);

    program_->release();
    vao_->release();
    delete vbo;
    delete ebo;
}

void ContainerShaderProgram::Render(QOpenGLFunctions *func)
{
    func->glActiveTexture(GL_TEXTURE0);
    texture_->bind();
    vao_->bind();
    program_->bind();

    func->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    program_->release();
    vao_->release();
    texture_->release();
}


MainWindow::MainWindow(QWidget *parent)
    : QOpenGLWidget(parent)
{
    triangle_shader_prog_ = nullptr;
    container_shader_prog_ = nullptr;
    resize(600,480);
}

MainWindow::~MainWindow()
{
    makeCurrent();
    SAFE_DELETE(triangle_shader_prog_);
    SAFE_DELETE(container_shader_prog_);
    doneCurrent();
}

void MainWindow::initializeGL()
{
    initializeOpenGLFunctions();

    SAFE_DELETE(triangle_shader_prog_);
    triangle_shader_prog_ = new TriangleShaderProgram;
    triangle_shader_prog_->Init(this);

    SAFE_DELETE(container_shader_prog_);
    container_shader_prog_ = new ContainerShaderProgram;
    container_shader_prog_->Init(this);
}

void MainWindow::paintGL()
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (triangle_shader_prog_) {
        triangle_shader_prog_->Render(this);
    }

    if (container_shader_prog_) {
        container_shader_prog_->Render(this);
    }
}
