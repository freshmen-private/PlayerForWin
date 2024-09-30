#include "glplayer.h"
#include <QDebug>

GLPlayer::GLPlayer(QWidget* parent)
    : QOpenGLWidget{parent}
{

}

void GLPlayer::initializeGL()
{
    initializeOpenGLFunctions();

    GLfloat vertices[] = {
        -1.0, 1.0,0.0,
         1.0, 1.0,0.0,
        -1.0,-1.0,0.0,
         1.0,-1.0,0.0
    };

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(GL_FLOAT), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glGenTextures(1, &Texture);
    glBindTexture(GL_TEXTURE_2D, Texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_LINEAR);

    QOpenGLShader vshader(QOpenGLShader::Vertex);
    vshader.compileSourceCode("#version 330 core\n"
                              "layout (location = 0) in vec3 aPos;\n"
                              "in vec2 aTexture;\n"
                              "out vec2 TexPos"
                              "void main(){\n"
                              "TexPos = aTexture;\n"
                              "gl_Position = vec4(aPos, 1.0);\n"
                              "}");
    QOpenGLShader fshader(QOpenGLShader::Fragment);
    fshader.compileSourceCode("#version 330 core\n"
                              "in vec2 TexPos;\n"
                              "uniform sampler2D tex;\n"
                              "out vec4 FragColor;\n"
                              "void main(){\n"
                              "FragColor = texture(tex, TexPos);\n"
                              "}");
    program.addShader(&vshader);
    program.addShader(&fshader);
    program.link();
    if(!program.isLinked())
    {
        qDebug()<<"opengl link failed!";
    }
}

void GLPlayer::resizeGL(int w, int h)
{

}

void GLPlayer::paintGL()
{
    // 使用指定的清除颜色清除颜色缓冲区
    glClearColor(0.2f, 0.4f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    // 激活着色器程序，准备进行绘制
    program.bind();
    glActiveTexture(0);
    glBindTexture(GL_TEXTURE_2D, Texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1280, 720, 0, GL_RGB, GL_UNSIGNED_BYTE, );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // 绑定之前创建的顶点数组对象
    glBindVertexArray(VAO);
    // 使用OpenGL函数绘制一个三角形，0 是指从顶点数组的第一个顶点开始绘制。3 是指绘制的顶点数量，这里是一个三角形
    glDrawArrays(GL_TRIANGLES,0,3);
}
