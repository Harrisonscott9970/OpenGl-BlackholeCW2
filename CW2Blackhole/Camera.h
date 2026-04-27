#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <GLFW/glfw3.h>

class Camera
{
public:
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    float Yaw;
    float Pitch;

    float Speed;
    float Sensitivity;

    bool firstMouse = true;
    float lastX = 0.0f;
    float lastY = 0.0f;

    Camera(glm::vec3 startPos = glm::vec3(0.0f, 0.0f, 0.0f));

    void processKeyboard(GLFWwindow* window, float moveSpeedMultiplier = 1.0f);
    void processMouse(GLFWwindow* window);

    glm::mat4 getViewMatrix() const;

    // Extra helpers for ship-driven / third-person use.
    void setTransform(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up);
    void lookAt(const glm::vec3& position, const glm::vec3& target, const glm::vec3& upHint = glm::vec3(0.0f, 1.0f, 0.0f));
    void setFromOrientation(const glm::vec3& position, const glm::quat& orientation);

private:
    void updateVectors();
};
