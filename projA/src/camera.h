#pragma once
#include <glm/glm.hpp>

class Camera
{
public:
    void setCameraData(glm::vec4 pos,
                       glm::vec4 look,
                       glm::vec4 up,
                       float heightAngle);

    void setAspectRatio(float aspect);
    void setNearFar(float nearPlane, float farPlane);

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjMatrix() const;
    glm::mat4 getViewProjMatrix() const;
    glm::vec3 getPosition() const;
    glm::vec3 getForward() const;
    glm::vec3 getUpVector() const;
    glm::vec3 getRightVector() const;
    void translate(const glm::vec3 &delta);
    void rotateAroundWorldUp(float radians);
    void rotateAroundRight(float radians);

private:
    glm::vec3 m_pos;
    glm::vec3 m_look;
    glm::vec3 m_up;

    float m_heightAngle;
    float m_aspect;
    float m_near;
    float m_far;
};
