#pragma once

// Defined before including GLEW to suppress deprecation messages on macOS
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif
#include <GL/glew.h>
#include <glm/glm.hpp>

#include <unordered_map>
#include <QElapsedTimer>
#include <QOpenGLWidget>
#include <QTime>
#include <QTimer>
#include "settings.h"
#include "shapes/Cube.h"
#include "shapes/Cone.h"
#include "shapes/Cylinder.h"
#include "shapes/Sphere.h"
#include "shapes/Star.h"
#include "camera.h"
#include "utils/sceneparser.h"

class Realtime : public QOpenGLWidget
{
public:
    Realtime(QWidget *parent = nullptr);
    void finish();                                      // Called on program exit
    void sceneChanged();
    void settingsChanged();
    void saveViewportImage(std::string filePath);
    void setSceneFilePath(std::string path);

public slots:
    void tick(QTimerEvent* event);                      // Called once per tick of m_timer

protected:
    void initializeGL() override;                       // Called once at the start of the program
    void paintGL() override;                            // Called whenever the OpenGL context changes or by an update() request
    void resizeGL(int width, int height) override;      // Called when window size changes

private:
    // void keyPressEvent(QKeyEvent *event) override;
    // void keyReleaseEvent(QKeyEvent *event) override;
    // void mousePressEvent(QMouseEvent *event) override;
    // void mouseReleaseEvent(QMouseEvent *event) override;
    // void mouseMoveEvent(QMouseEvent *event) override;
    void timerEvent(QTimerEvent *event) override;

    // Tick Related Variables
    int m_timer;                                        // Stores timer which attempts to run ~60 times per second
    QElapsedTimer m_elapsedTimer;                       // Stores timer which keeps track of actual time between frames

    // Input Related Variables
    bool m_mouseDown = false;                           // Stores state of left mouse button
    glm::vec2 m_prev_mouse_pos;                         // Stores mouse position
    std::unordered_map<Qt::Key, bool> m_keyMap;         // Stores whether keys are pressed or not

    // Device Correction Variables
    double m_devicePixelRatio;

    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_shader;
    GLint m_uniViewProj;
    Camera m_camera;
    RenderData m_renderData;
    std::string m_sceneFilePath;

    Cube m_cube;
    Sphere m_sphere;
    Cone m_cone;
    Cylinder m_cylinder;
    Star m_star;
    GLuint m_backgroundTex = 0;

    std::vector<GLuint> m_vaos;
    std::vector<GLuint> m_vbos;
    std::vector<int>    m_vboSizes;

    void buildVAOsFromRenderData();

    // === NEW: For Bloom / offscreen rendering ===
    GLuint m_sceneFBO = 0;
    GLuint m_sceneColorTex = 0;
    GLuint m_sceneDepthRBO = 0;

    GLuint m_brightFBO = 0;
    GLuint m_brightTex = 0;

    // fullscreen quad + post-processiong shader
    GLuint m_quadVAO = 0;
    GLuint m_quadVBO = 0;
    GLuint m_brightShader = 0;   // bright-pass
    GLuint m_screenShader = 0;   // draw texture on the screen

    // blur
    GLuint m_pingFBO = 0;
    GLuint m_pingTex = 0;
    GLuint m_pongFBO = 0;
    GLuint m_pongTex = 0;
    GLuint m_blurShader = 0;

    float m_scrollTime = 0.f;
    float m_bgScrollOffset = 0.f;

};
