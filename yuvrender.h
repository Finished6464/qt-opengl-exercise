#ifndef YUVRENDER_H
#define YUVRENDER_H

class QOpenGLShaderProgram;
class QOpenGLVertexArrayObject;
class QOpenGLTexture;

class YUVRender
{
public:
    YUVRender();
    virtual ~YUVRender();

    int Build();
    void Unbuild();

    void Render(char* buff, int size, int frame_width, int frame_height);

private:
    QOpenGLShaderProgram *program_;
    QOpenGLVertexArrayObject *vao_;
    QOpenGLTexture *texture_y_;
    QOpenGLTexture *texture_u_;
    QOpenGLTexture *texture_v_;
};

#endif // YUVRENDER_H
