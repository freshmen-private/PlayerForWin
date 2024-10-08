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
    void OpenFile(QString FileName);
protected:
    virtual void initializeGL() override;
    virtual void resizeGL(int w, int h) override;
    virtual void paintGL() override;

private:
    GLuint VAO, VBO;
    GLuint Texture;
    QOpenGLShaderProgram program;
    QImage image;
};



#endif // GLPLAYER_H
