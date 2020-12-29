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

    int Build(int width, int height);
    void Unbuild();

    void Render(const unsigned char* const buff);
    void Render2(const unsigned char * const src_data[], const int src_linesize[]);

private:
    QOpenGLShaderProgram *program_;
    QOpenGLVertexArrayObject *vao_;
    QOpenGLTexture *texture_y_;
    QOpenGLTexture *texture_u_;
    QOpenGLTexture *texture_v_;

    int width_, height_;
};

#endif // YUVRENDER_H
