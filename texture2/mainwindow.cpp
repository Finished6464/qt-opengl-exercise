#include "mainwindow.h"

#define HELP_JUST_HEAD_
#include "../help.h"

#include <QApplication>
#include <QScreen>

#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>

//#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QRadioButton>
#include <QLabel>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QComboBox>



GlWidget::GlWidget(QWidget *parent)
    :QOpenGLWidget(parent)
{
    program_ = nullptr;
    vao_ = nullptr;
    texture1_ = texture2_ =nullptr;

    face_flip_h_ = face_flip_v_ = false;
    texture_mix_val_ = 0.2f;
    texture_wrapmode_ = QOpenGLTexture::Repeat;
    texture_filter_ = QOpenGLTexture::Linear;
    texture_zoom_state_ = 0;

    container_img_ = new QImage("container.jpg");
    face_img_ = new QImage("awesomeface.png");
}


GlWidget::~GlWidget()
{
    makeCurrent();
    SAFE_DELETE(program_);
    SAFE_DELETE(vao_);
    SAFE_DELETE(texture1_);
    SAFE_DELETE(texture2_);
    doneCurrent();
    SAFE_DELETE(container_img_);
    SAFE_DELETE(face_img_);
}

void GlWidget::EnableFaceHorizontalFlip(bool enabled)
{
    face_flip_h_ = enabled;
    RenderNow();
}

void GlWidget::EnableFaceVerticalFlip(bool enabled)
{
    face_flip_v_ = enabled;
    RenderNow();
}


void GlWidget::SetTextureMixVal(float val)
{
    texture_mix_val_ = val;
    RenderNow();
}

void GlWidget::SetTextureWrapMode(int mode)
{
    texture_wrapmode_ = mode;
    RenderNow();
}

void GlWidget::SetTextureFilter(int val)
{
    texture_filter_ = val;
    RenderNow();
}

void GlWidget::SetTextureZoomState(int state)
{
    texture_zoom_state_ = state;
    RenderNow();
}

void GlWidget::RenderNow()
{
    makeCurrent();
    initializeGL();
    update();
}

void GlWidget::initializeGL()
{
   const char* vertext_shader_source =
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

    QString fragment_shader_source_fmt =
        "out vec4 FragColor;\n"
        "in vec3 ourColor;\n"
        "in vec2 TexCoord;\n"
        "uniform sampler2D texture1;\n"
        "uniform sampler2D texture2;\n"
        "void main()\n"
        "{\n"
        "   FragColor = mix(texture(texture1, TexCoord), texture(texture2, %1), %2);\n" // 水平翻转笑脸
        "}\n";

    float vertices[] {
    //---- 位置 ----       ---- 颜色 ----     - 纹理坐标(注意下面的2.0f显示4个笑脸) -
     1.0f,  1.0f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // 右上
     1.0f, -1.0f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   // 右下
    -1.0f, -1.0f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // 左下
    -1.0f,  1.0f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f    // 左上
    };

    const unsigned int indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };

    QString face_coord = QString("vec2(%1, %2)").arg(face_flip_h_ ? "1.0 - TexCoord.x" : "TexCoord.x").arg(face_flip_v_ ? "1.0 - TexCoord.y" : "TexCoord.y");
    QOpenGLBuffer *vbo, *ebo;

    initializeOpenGLFunctions();

    if (texture_zoom_state_ == 1) {
        vertices[6] = vertices[7] = vertices[14] = vertices[31] = 2.0f;
        vertices[15] = vertices[22] = vertices[23] = vertices[30] = 0.0f;
    }
    else if (texture_zoom_state_ == 2) { //显示局部图像
        vertices[6] = vertices[7] = vertices[14] = vertices[31] = 0.67f;
        vertices[15] = vertices[22] = vertices[23] = vertices[30] = 0.52f;
    }

    SAFE_DELETE(program_);
    program_ = new QOpenGLShaderProgram;
    program_->addShaderFromSourceCode(QOpenGLShader::Vertex, versionedShaderCode(vertext_shader_source));
    program_->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                      versionedShaderCode(fragment_shader_source_fmt.arg(face_coord).arg(texture_mix_val_).toLatin1()));
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
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)0);

    // 颜色属性(顶点着色器代码内已定义 location = 1) layout (location = 1) in vec3 aColor;
    // 颜色的每组索引需偏移3个float(3个顶点位置)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(3* sizeof(GLfloat)));

    // 纹理坐标属性(顶点着色器代码内已定义 location = 1) layout (location = 1) in vec3 aColor;
    // 纹理坐标的每组索引需偏移6个float(3个顶点位置 + 3个颜色)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(6* sizeof(GLfloat)));

    SAFE_DELETE(texture1_);
    texture1_ = new QOpenGLTexture(*container_img_);
    // 为当前绑定的纹理对象设置环绕、过滤方式
    texture1_->setWrapMode(QOpenGLTexture::DirectionS, QOpenGLTexture::WrapMode(texture_wrapmode_));
    texture1_->setWrapMode(QOpenGLTexture::DirectionT, QOpenGLTexture::WrapMode(texture_wrapmode_));
    texture1_->setMinificationFilter(QOpenGLTexture::Filter(texture_filter_));
    texture1_->setMagnificationFilter(QOpenGLTexture::Filter(texture_filter_));

    SAFE_DELETE(texture2_);
    texture2_ = new QOpenGLTexture(face_img_->mirrored());
    texture2_->setWrapMode(QOpenGLTexture::DirectionS, QOpenGLTexture::WrapMode(texture_wrapmode_));
    texture2_->setWrapMode(QOpenGLTexture::DirectionT, QOpenGLTexture::WrapMode(texture_wrapmode_));
    texture2_->setMinificationFilter(QOpenGLTexture::Filter(texture_filter_));
    texture2_->setMagnificationFilter(QOpenGLTexture::Filter(texture_filter_));

    glUniform1i(program_->uniformLocation("texture1"), 0); // 手动设置
//    func->glUniform1i(program_->uniformLocation("texture2"), 1);
    program_->setUniformValue("texture2", 1); // 或者使用着色器类设置

    program_->release();
    vao_->release();
    delete vbo;
    delete ebo;
}

void GlWidget::paintGL()
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (program_ && texture1_ && texture2_) {
        glActiveTexture(GL_TEXTURE0);
        texture1_->bind();
        glActiveTexture(GL_TEXTURE1);
        texture2_->bind();
        vao_->bind();
        program_->bind();

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        program_->release();
        vao_->release();
        texture1_->release();
        texture2_->release();
    }
}



MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
{
    QGroupBox *group_box;
    QBoxLayout *layout, *controls_layout, *layout2;
//    QGridLayout *grid_layout;
    QRadioButton* radio_btn;
    QCheckBox *check_box;
    QDoubleSpinBox *double_spin_box;
    QComboBox* cbo_box;

    layout = new QHBoxLayout;

    gl_widget_ = new GlWidget(this);
    layout->addWidget(gl_widget_);
    layout->setStretch(0, 2);

    controls_layout = new QVBoxLayout;

    //face flip
    group_box = new QGroupBox("Face Flip");
    layout2 = new QHBoxLayout;
    check_box = new QCheckBox("Horizontal");
    connect(check_box, &QCheckBox::stateChanged, this, [=](int state) { gl_widget_->EnableFaceHorizontalFlip(state == Qt::Checked); });
    layout2->addWidget(check_box);
    check_box = new QCheckBox("Vertical");
    QObject::connect(check_box, &QCheckBox::stateChanged, this, [&](int state){ gl_widget_->EnableFaceVerticalFlip(state == Qt::Checked); });
    layout2->addWidget(check_box);
//    layout2->addStretch(1);
    group_box->setLayout(layout2);
    controls_layout->addWidget(group_box);


    group_box = new QGroupBox("Face Texture Wrap Mode");
    layout2 = new QVBoxLayout;
    radio_btn = new QRadioButton("GL_REPEAT");
    radio_btn->setChecked(true); //default
    QObject::connect(radio_btn, &QRadioButton::clicked, this, [=](bool checked){ if (checked) {gl_widget_->SetTextureWrapMode(QOpenGLTexture::Repeat);} });
    layout2->addWidget(radio_btn);
    radio_btn = new QRadioButton("GL_MIRRORED_REPEAT");
    QObject::connect(radio_btn, &QRadioButton::clicked, this, [=](bool checked){ if (checked) {gl_widget_->SetTextureWrapMode(QOpenGLTexture::MirroredRepeat);} });
    layout2->addWidget(radio_btn);
    radio_btn = new QRadioButton("GL_CLAMP_TO_EDGE");
    QObject::connect(radio_btn, &QRadioButton::clicked, this, [=](bool checked){ if (checked) {gl_widget_->SetTextureWrapMode( QOpenGLTexture::ClampToEdge);} });
    layout2->addWidget(radio_btn);
    radio_btn = new QRadioButton("GL_CLAMP_TO_BORDER");
    QObject::connect(radio_btn, &QRadioButton::clicked, this, [=](bool checked){ if (checked) {gl_widget_->SetTextureWrapMode(QOpenGLTexture::ClampToBorder);} });
    layout2->addWidget(radio_btn);
    layout2->addStretch(1);
    group_box->setLayout(layout2);
    controls_layout->addWidget(group_box);

    group_box = new QGroupBox("Face Texture Filtering");
    layout2 = new QVBoxLayout;
    radio_btn = new QRadioButton("GL_NEAREST");
    QObject::connect(radio_btn, &QRadioButton::toggled, this, [=](bool checked){ if (checked) {gl_widget_->SetTextureFilter(QOpenGLTexture::Nearest);} });
    layout2->addWidget(radio_btn);
    radio_btn = new QRadioButton("GL_LINEAR");
    radio_btn->setChecked(true); //default
    QObject::connect(radio_btn, &QRadioButton::toggled, this, [=](bool checked){ if (checked) {gl_widget_->SetTextureFilter(QOpenGLTexture::Linear);} });
    layout2->addWidget(radio_btn);
    radio_btn = new QRadioButton("GL_NEAREST_MIPMAP_NEAREST");
    QObject::connect(radio_btn, &QRadioButton::toggled, this, [=](bool checked){ if (checked) {gl_widget_->SetTextureFilter(QOpenGLTexture::NearestMipMapNearest);} });
    layout2->addWidget(radio_btn);
    radio_btn = new QRadioButton("GL_NEAREST_MIPMAP_LINEAR");
    QObject::connect(radio_btn, &QRadioButton::toggled, this, [=](bool checked){ if (checked) {gl_widget_->SetTextureFilter(QOpenGLTexture::NearestMipMapLinear);} });
    layout2->addWidget(radio_btn);
    radio_btn = new QRadioButton("GL_LINEAR_MIPMAP_NEAREST");
    QObject::connect(radio_btn, &QRadioButton::toggled, this, [=](bool checked){ if (checked) {gl_widget_->SetTextureFilter(QOpenGLTexture::LinearMipMapNearest);} });
    layout2->addWidget(radio_btn);
    radio_btn = new QRadioButton("GL_LINEAR_MIPMAP_LINEAR");
    QObject::connect(radio_btn, &QRadioButton::toggled, this, [=](bool checked){ if (checked) {gl_widget_->SetTextureFilter(QOpenGLTexture::LinearMipMapLinear);} });
    layout2->addWidget(radio_btn);
    layout2->addStretch(1);
    group_box->setLayout(layout2);
    controls_layout->addWidget(group_box);

    layout2 = new QHBoxLayout;
    layout2->addWidget(new QLabel("Zoom: "));
    cbo_box = new QComboBox;
    cbo_box->addItem("Normal");
    cbo_box->addItem("Four");
    cbo_box->addItem("Partial");
    cbo_box->setCurrentIndex(0);
    connect(cbo_box, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int index){ gl_widget_->SetTextureZoomState(index); });
    layout2->addWidget(cbo_box);
    layout2->addStretch(1);
    controls_layout->addLayout(layout2);

//    check_box = new QCheckBox("Four Faces");
//    connect(check_box, &QCheckBox::stateChanged, this, [=](int state){ gl_widget_->EnableFourFaces(state == Qt::Checked); });
//    layout2->addWidget(check_box);
//    check_box = new QCheckBox("Partial Texture");
////    QObject::connect(check_box, &QCheckBox::stateChanged, this, [&](int state){ partial_texture_enabled_ = state == Qt::Checked; });
//    layout2->addWidget(check_box);
////    layout2->addStretch(1);
//    controls_layout->addLayout(layout2);

    layout2 = new QHBoxLayout;
    layout2->addWidget(new QLabel("Texture Mix: "));
    double_spin_box = new QDoubleSpinBox;
    double_spin_box->setValue(0.2); //default
    double_spin_box->setRange(0.0, 1.0);
    double_spin_box->setSingleStep(0.1);
    QObject::connect(double_spin_box, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [=](double val){ gl_widget_->SetTextureMixVal(val); });
    layout2->addWidget(double_spin_box);
    layout2->addStretch(1);
    controls_layout->addLayout(layout2);

    controls_layout->addStretch(1);
    controls_layout->setMargin(5);
    controls_layout->setSpacing(15);
    layout->addLayout(controls_layout);

    setLayout(layout);
    resize(900, 600);
    move(QApplication::screens().at(0)->geometry().center() - frameGeometry().center());
}

MainWindow::~MainWindow()
{
}
