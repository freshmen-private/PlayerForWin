#include "glplayer.h"
#include <QDebug>
#include <QTimer>
#include <QThread>

GLPlayer::GLPlayer(QWidget* parent)
    : QOpenGLWidget(parent),
    VAO(0),
    VBO(0),
    Texture(0),
    program(nullptr)
{
    image.load("C:/Users/Colorful/Desktop/picture5/pic0.jpg");
    image = image.convertToFormat(QImage::Format_RGBA8888);
    QTimer* timer = new QTimer(this);
    timer->start(16);
}

GLPlayer::~GLPlayer()
{
}

void GLPlayer::OpenFile(QImage& tmpimg)
{
    image = tmpimg;
    update();
}

void GLPlayer::initializeGL()
{
    initializeOpenGLFunctions();
    GLfloat vertices[] = {
        -1.0,  1.0, 0.0, 0.0, 0.0,
         1.0,  1.0, 0.0, 1.0, 0.0,
        -1.0, -1.0, 0.0, 0.0, 1.0,
        -1.0, -1.0, 0.0, 0.0, 1.0,
         1.0, -1.0, 0.0, 1.0, 1.0,
         1.0,  1.0, 0.0, 1.0, 0.0
    };
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(GL_FLOAT), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5*sizeof(GL_FLOAT), (void*)(3 * sizeof(GL_FLOAT)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glGenTextures(1, &Texture);
    glBindTexture(GL_TEXTURE_2D, Texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width(), image.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, image.bits());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_LINEAR);

    const char* vshader = R"(#version 330 core
                             layout (location = 0) in vec3 aPos;
                             in vec2 aTexture;
                             out vec2 TexPos;
                             void main(){
                                 TexPos = aTexture;
                                 gl_Position = vec4(aPos, 1.0);
                             }
                            )";
    const char* fshader = R"(#version 330 core
                             in vec2 TexPos;
                             uniform sampler2D tex;
                             out vec4 FragColor;
                             void main(){
                                 FragColor = texture(tex, TexPos);
                                 //FragColor = vec4(1.0,1.0,0.0,1.0);
                             }
                            )";
    program.addShaderFromSourceCode(QOpenGLShader::Vertex, vshader);
    program.addShaderFromSourceCode(QOpenGLShader::Fragment, fshader);
    program.link();
    program.bind();
    program.setUniformValue("Tex", 0);
    program.release();
}

void GLPlayer::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

void GLPlayer::paintGL()
{
    // qDebug()<<image.width();
    // 使用指定的清除颜色清除颜色缓冲区
    glClearColor(0.2f, 0.4f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    // 激活着色器程序，准备进行绘制
    program.bind();
    //glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, Texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width(), image.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, image.bits());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_LINEAR);
    // 绑定之前创建的顶点数组对象
    glBindVertexArray(VAO);
    // 使用OpenGL函数绘制一个四边形，0 是指从顶点数组的第一个顶点开始绘制。6 是指绘制的顶点数量，这里是一个四边形
    glDrawArrays(GL_TRIANGLES, 0, 6);
}
