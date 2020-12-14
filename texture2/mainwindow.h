#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>


class QOpenGLShaderProgram;
class QOpenGLVertexArrayObject;
class QOpenGLTexture;
class QRadioButton;
class QCheckBox;
class QDoubleSpinBox;



class GlWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    GlWidget(QWidget *parent = nullptr);
    ~GlWidget();

    void EnableFaceHorizontalFlip(bool enabled);
    void EnableFaceVerticalFlip(bool enabled);
//    void EnableFourFaces(bool enabled);
    void SetTextureMixVal(float val);
    void SetTextureWrapMode(int mode);
    void SetTextureFilter(int val);
    void SetTextureZoomState(int state);
//    void EnablePartial(bool enabled);
//    void UpdateConfig(int face_look, int texture_wrapmode, int texture_filter, bool four_faces_enabled, bool partial_texture_enabled, float mix_val);


private:
    void initializeGL();
    void paintGL();

    void RenderNow();

private:
    QOpenGLShaderProgram *program_;
    QOpenGLVertexArrayObject *vao_;
    QOpenGLTexture *texture1_, *texture2_;

    bool face_flip_h_;
    bool face_flip_v_;
//    bool four_faces_enabled_;
    float texture_mix_val_;
    int texture_wrapmode_;
    int texture_filter_;
//    bool partial_texture_enabled_;
    int texture_zoom_state_;

    QImage *container_img_, *face_img_;

};


class MainWindow : public QWidget
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    GlWidget* gl_widget_;
};
#endif // MAINWINDOW_H
