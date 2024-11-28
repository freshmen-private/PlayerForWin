#ifndef GLPLAYER_H
#define GLPLAYER_H

#include <QObject>
#include <QtOpenGLWidgets/QtOpenGLWidgets>
#include <QOpenGLFunctions_3_3_Core>
#include <QQueue>
#include <QImage>

class GLPlayer:public QOpenGLWidget, QOpenGLFunctions_3_3_Core
{
    Q_OBJECT
public:
    explicit GLPlayer(QWidget* parent = nullptr);
    ~GLPlayer();
    void OpenFile(QImage& tmpimg);
protected:
    virtual void initializeGL() override;
    virtual void resizeGL(int w, int h) override;
    virtual void paintGL() override;

protected slots:
    void getOneFrame(QImage wait2Display);

private:
    int srcW, srcH;
    float ww,hh;
    float x,y,width,height;
    GLuint VAO, VBO;
    GLuint Texture;
    QOpenGLShaderProgram program;
    QImage image;
};



#endif // GLPLAYER_H
