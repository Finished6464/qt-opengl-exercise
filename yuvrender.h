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

    int Build(int video_width, int video_height);
    void Unbuild();
    bool IsBulit() const { return is_built; }
    void Resize();
    void SetupPixel(const unsigned char *yuv_buff);
    void SetupPixel(const unsigned char* const *data, const int *linesize);
    void Draw();

private:
    QOpenGLShaderProgram *program_;
    QOpenGLVertexArrayObject *vao_;
    QOpenGLTexture *texture_y_;
    QOpenGLTexture *texture_u_;
    QOpenGLTexture *texture_v_;

    int video_width_, video_height_;

    bool is_built;
};

#endif // YUVRENDER_H
