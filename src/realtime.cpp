#include "realtime.h"

#include <QCoreApplication>
#include <QMouseEvent>
#include <QKeyEvent>
#include <iostream>
#include "settings.h"
#include "utils/shaderloader.h"
#include "camera.h"
#include "utils/sceneparser.h"


// ================== Rendering the Scene!

Realtime::Realtime(QWidget *parent)
    : QOpenGLWidget(parent)
{
    m_prev_mouse_pos = glm::vec2(size().width()/2, size().height()/2);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    m_keyMap[Qt::Key_W]       = false;
    m_keyMap[Qt::Key_A]       = false;
    m_keyMap[Qt::Key_S]       = false;
    m_keyMap[Qt::Key_D]       = false;
    m_keyMap[Qt::Key_Control] = false;
    m_keyMap[Qt::Key_Space]   = false;

    // If you must use this function, do not edit anything above this
}

void Realtime::finish() {
    killTimer(m_timer);
    this->makeCurrent();

    // Students: anything requiring OpenGL calls when the program exits should be done here
    glDeleteBuffers(1, &m_vbo);
    glDeleteVertexArrays(1, &m_vao);
    glDeleteProgram(m_shader);

    this->doneCurrent();
}

void Realtime::initializeGL() {
    m_devicePixelRatio = this->devicePixelRatio();

    m_timer = startTimer(1000/60);
    m_elapsedTimer.start();

    // Initializing GL.
    // GLEW (GL Extension Wrangler) provides access to OpenGL functions.
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "Error while initializing GL: " << glewGetErrorString(err) << std::endl;
    }
    std::cout << "Initialized GL: Version " << glewGetString(GLEW_VERSION) << std::endl;

    // Allows OpenGL to draw objects appropriately on top of one another
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClearColor(0.f, 0.f, 0.f, 1.0f);
    // Tells OpenGL how big the screen is
    glViewport(0, 0, size().width() * m_devicePixelRatio, size().height() * m_devicePixelRatio);

    // Students: anything requiring OpenGL calls when the program starts should be done here
    m_shader = ShaderLoader::createShaderProgram(
        ":/resources/shaders/default.vert",
        ":/resources/shaders/default.frag"
        );

}

void Realtime::paintGL() {
    // 1) Clear the default framebuffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 2) Bail out if we have nothing to draw
    if (!m_shader || m_vaos.empty()) {
        return;
    }

    // 3) Bind the shader once per frame
    glUseProgram(m_shader);

    // 4) Upload view & projection matrices derived from the current camera
    glm::mat4 view = m_camera.getViewMatrix();
    glm::mat4 proj = m_camera.getProjMatrix();

    if (GLint uView = glGetUniformLocation(m_shader, "view"); uView != -1) {
        glUniformMatrix4fv(uView, 1, GL_FALSE, &view[0][0]);
    }
    if (GLint uProj = glGetUniformLocation(m_shader, "proj"); uProj != -1) {
        glUniformMatrix4fv(uProj, 1, GL_FALSE, &proj[0][0]);
    }

    // 5) Push the scene-wide lighting constants (global coefficients & camera position)
    const SceneGlobalData &g = m_renderData.globalData;
    if (GLint loc = glGetUniformLocation(m_shader, "global_ka"); loc != -1)
        glUniform1f(loc, g.ka);
    if (GLint loc = glGetUniformLocation(m_shader, "global_kd"); loc != -1)
        glUniform1f(loc, g.kd);
    if (GLint loc = glGetUniformLocation(m_shader, "global_ks"); loc != -1)
        glUniform1f(loc, g.ks);

    glm::vec3 camPos = glm::vec3(m_renderData.cameraData.pos);
    if (GLint loc = glGetUniformLocation(m_shader, "cameraPos"); loc != -1)
        glUniform3f(loc, camPos.x, camPos.y, camPos.z);

    // 6) Upload lighting parameters
    glm::vec3 lightPos(5.f, 5.f, 5.f);
    glm::vec3 lightColor(1.f, 1.f, 1.f);

    if (GLint loc = glGetUniformLocation(m_shader, "lightPos"); loc != -1)
        glUniform3fv(loc, 1, &lightPos[0]);
    if (GLint loc = glGetUniformLocation(m_shader, "lightColor"); loc != -1)
        glUniform3fv(loc, 1, &lightColor[0]);

    // 7) Draw each mesh with its own material and model matrix
    for (int i = 0; i < static_cast<int>(m_vaos.size()); ++i) {
        glBindVertexArray(m_vaos[i]);

        const SceneMaterial &mat = m_renderData.shapes[i].primitive.material;
        if (GLint loc = glGetUniformLocation(m_shader, "matAmbient"); loc != -1)
            glUniform4fv(loc, 1, &mat.cAmbient[0]);
        if (GLint loc = glGetUniformLocation(m_shader, "matDiffuse"); loc != -1)
            glUniform4fv(loc, 1, &mat.cDiffuse[0]);
        if (GLint loc = glGetUniformLocation(m_shader, "matSpecular"); loc != -1)
            glUniform4fv(loc, 1, &mat.cSpecular[0]);
        if (GLint loc = glGetUniformLocation(m_shader, "matShininess"); loc != -1)
            glUniform1f(loc, mat.shininess);

        glm::mat4 model = m_renderData.shapes[i].ctm;
        if (GLint loc = glGetUniformLocation(m_shader, "model"); loc != -1)
            glUniformMatrix4fv(loc, 1, GL_FALSE, &model[0][0]);

        glDrawArrays(GL_TRIANGLES, 0, m_vboSizes[i]);
        glBindVertexArray(0);
    }

    // 8) Unbind the shader
    glUseProgram(0);
}

void Realtime::resizeGL(int w, int h) {
    // Tells OpenGL how big the screen is
    glViewport(0, 0, size().width() * m_devicePixelRatio, size().height() * m_devicePixelRatio);

    // Students: anything requiring OpenGL calls when the program starts should be done here
}

void Realtime::sceneChanged() {
    m_renderData = RenderData();

    if (SceneParser::parse(m_sceneFilePath, m_renderData)) {
        // load camera data
        m_camera.setCameraData(
            m_renderData.cameraData.pos,
            m_renderData.cameraData.look,
            m_renderData.cameraData.up,
            m_renderData.cameraData.heightAngle
        );

        m_camera.setAspectRatio(float(width()) / float(height()));
        m_camera.setNearFar(settings.nearPlane, settings.farPlane);
    } else {
        std::cout << "ERROR: Scene parsing failed!" << std::endl;
    }

    // regenerate VAO / VBO
    buildVAOsFromRenderData();

    update();
}

// load VAOn from render data
void Realtime::buildVAOsFromRenderData()
{
    makeCurrent();

    // clean old VAO / VBO
    for (GLuint vao : m_vaos) {
        glDeleteVertexArrays(1, &vao);
    }
    for (GLuint vbo : m_vbos) {
        glDeleteBuffers(1, &vbo);
    }
    m_vaos.clear();
    m_vbos.clear();
    m_vboSizes.clear();

    // generate a VAO + VBO for each primitive
    for (const RenderShapeData &shape : m_renderData.shapes)
    {
        std::vector<float> vertexData;

        // ---- generates vertex normal data of the primitive type ----
        switch (shape.primitive.type) {
        case PrimitiveType::PRIMITIVE_CUBE:
            m_cube.updateParams(settings.shapeParameter1);
            vertexData = m_cube.generateShape();
            break;
        case PrimitiveType::PRIMITIVE_SPHERE:
            m_sphere.updateParams(settings.shapeParameter1,
                                  settings.shapeParameter2);
            vertexData = m_sphere.generateShape();
            break;
        case PrimitiveType::PRIMITIVE_CONE:
            m_cone.updateParams(settings.shapeParameter1,
                                settings.shapeParameter2);
            vertexData = m_cone.generateShape();
            break;
        case PrimitiveType::PRIMITIVE_CYLINDER:
            m_cylinder.updateParams(settings.shapeParameter1,
                                    settings.shapeParameter2);
            vertexData = m_cylinder.generateShape();
            break;
        default:
            continue;
        }

        if (vertexData.empty()) {
            continue;
        }

        // ---- create and bind VAO / VBO ----
        GLuint vao, vbo;
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        glBufferData(GL_ARRAY_BUFFER,
                     sizeof(float) * vertexData.size(),
                     vertexData.data(),
                     GL_STATIC_DRAW);

        int stride = 6 * sizeof(float);   // 3 pos + 3 normal

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                              stride, (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));

        // ---- unbind ----
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        // push back
        m_vaos.push_back(vao);
        m_vbos.push_back(vbo);
        m_vboSizes.push_back(static_cast<int>(vertexData.size() / 6));

    }
}

void Realtime::settingsChanged() {
    if (!m_renderData.shapes.empty()) {
        buildVAOsFromRenderData();
        m_camera.setAspectRatio(float(width()) / float(height()));
        m_camera.setNearFar(settings.nearPlane, settings.farPlane);
    }
    update(); // asks for a PaintGL() call to occur
}

// ================== Camera Movement! ---- Not needed

// void Realtime::keyPressEvent(QKeyEvent *event) {
//     m_keyMap[Qt::Key(event->key())] = true;
// }

// void Realtime::keyReleaseEvent(QKeyEvent *event) {
//     m_keyMap[Qt::Key(event->key())] = false;
// }

// void Realtime::mousePressEvent(QMouseEvent *event) {
//     if (event->buttons().testFlag(Qt::LeftButton)) {
//         m_mouseDown = true;
//         m_prev_mouse_pos = glm::vec2(event->position().x(), event->position().y());
//     }
// }

// void Realtime::mouseReleaseEvent(QMouseEvent *event) {
//     if (!event->buttons().testFlag(Qt::LeftButton)) {
//         m_mouseDown = false;
//     }
// }

// void Realtime::mouseMoveEvent(QMouseEvent *event) {
//     if (m_mouseDown) {
//         int posX = event->position().x();
//         int posY = event->position().y();
//         int deltaX = posX - m_prev_mouse_pos.x;
//         int deltaY = posY - m_prev_mouse_pos.y;
//         m_prev_mouse_pos = glm::vec2(posX, posY);

//         const float sensitivity = 0.005f;
//         m_camera.rotateAroundWorldUp(-deltaX * sensitivity);
//         m_camera.rotateAroundRight(-deltaY * sensitivity);

//         update(); // asks for a PaintGL() call to occur
//     }
// }

// void Realtime::timerEvent(QTimerEvent *event) {
//     int elapsedms   = m_elapsedTimer.elapsed();
//     float deltaTime = elapsedms * 0.001f;
//     m_elapsedTimer.restart();

//     const float moveSpeed = 5.f;
//     glm::vec3 movement(0.f);

//     glm::vec3 forward = m_camera.getForward();
//     glm::vec3 right = m_camera.getRightVector();

//     if (m_keyMap[Qt::Key_W]) movement += forward;
//     if (m_keyMap[Qt::Key_S]) movement -= forward;
//     if (m_keyMap[Qt::Key_D]) movement += right;
//     if (m_keyMap[Qt::Key_A]) movement -= right;
//     if (m_keyMap[Qt::Key_Space]) movement += glm::vec3(0.f, 1.f, 0.f);
//     if (m_keyMap[Qt::Key_Control]) movement += glm::vec3(0.f, -1.f, 0.f);

//     if (glm::length(movement) > 0.f) {
//         movement = glm::normalize(movement) * moveSpeed * deltaTime;
//         m_camera.translate(movement);
//     }

//     update(); // asks for a PaintGL() call to occur
// }

// DO NOT EDIT
void Realtime::saveViewportImage(std::string filePath) {
    // Make sure we have the right context and everything has been drawn
    makeCurrent();

    // int fixedWidth = 1024;
    // int fixedHeight = 768;

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    int fixedWidth = viewport[2];
    int fixedHeight = viewport[3];

    // Create Frame Buffer
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // Create a color attachment texture
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, fixedWidth, fixedHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    // Optional: Create a depth buffer if your rendering uses depth testing
    GLuint rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, fixedWidth, fixedHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Error: Framebuffer is not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return;
    }

    // Render to the FBO
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, fixedWidth, fixedHeight);

    // Clear and render your scene here
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    paintGL();

    // Read pixels from framebuffer
    std::vector<unsigned char> pixels(fixedWidth * fixedHeight * 3);
    glReadPixels(0, 0, fixedWidth, fixedHeight, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    // Unbind the framebuffer to return to default rendering to the screen
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Convert to QImage
    QImage image(pixels.data(), fixedWidth, fixedHeight, QImage::Format_RGB888);
    QImage flippedImage = image.mirrored(); // Flip the image vertically

    // Save to file using Qt
    QString qFilePath = QString::fromStdString(filePath);
    if (!flippedImage.save(qFilePath)) {
        std::cerr << "Failed to save image to " << filePath << std::endl;
    }

    // Clean up
    glDeleteTextures(1, &texture);
    glDeleteRenderbuffers(1, &rbo);
    glDeleteFramebuffers(1, &fbo);
}

void Realtime::setSceneFilePath(std::string path) { m_sceneFilePath = path; }
