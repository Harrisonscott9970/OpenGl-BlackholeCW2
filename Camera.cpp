#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <cmath>

Camera::Camera(glm::vec3 startPos)
{
    Position = startPos;

    WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);

    Yaw = -90.0f;
    Pitch = 0.0f;

    Speed = 0.1f;
    Sensitivity = 0.1f;

    updateVectors();
}

glm::mat4 Camera::getViewMatrix() const
{
    return glm::lookAt(Position, Position + Front, Up);
}

void Camera::processKeyboard(GLFWwindow* window, float moveSpeedMultiplier)
{
    float actualSpeed = Speed * moveSpeedMultiplier;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        Position += actualSpeed * Front;

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        Position -= actualSpeed * Front;

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        Position -= glm::normalize(glm::cross(Front, Up)) * actualSpeed;

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        Position += glm::normalize(glm::cross(Front, Up)) * actualSpeed;
}

void Camera::processMouse(GLFWwindow* window)
{
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    if (firstMouse)
    {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }

    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos;

    lastX = (float)xpos;
    lastY = (float)ypos;

    xoffset *= Sensitivity;
    yoffset *= Sensitivity;

    Yaw += xoffset;
    Pitch += yoffset;

    if (Pitch > 89.0f) Pitch = 89.0f;
    if (Pitch < -89.0f) Pitch = -89.0f;

    updateVectors();
}

void Camera::setTransform(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up)
{
    Position = position;
    Front = glm::normalize(forward);
    Right = glm::normalize(glm::cross(Front, up));
    Up = glm::normalize(glm::cross(Right, Front));
    WorldUp = glm::normalize(up);

    Yaw = glm::degrees(std::atan2(Front.z, Front.x));
    Pitch = glm::degrees(std::asin(glm::clamp(Front.y, -1.0f, 1.0f)));
}

void Camera::lookAt(const glm::vec3& position, const glm::vec3& target, const glm::vec3& upHint)
{
    glm::vec3 forward = glm::normalize(target - position);
    glm::vec3 right = glm::normalize(glm::cross(forward, upHint));
    glm::vec3 up = glm::normalize(glm::cross(right, forward));

    Position = position;
    Front = forward;
    Right = right;
    Up = up;
    WorldUp = glm::normalize(upHint);

    Yaw = glm::degrees(std::atan2(Front.z, Front.x));
    Pitch = glm::degrees(std::asin(glm::clamp(Front.y, -1.0f, 1.0f)));
}

void Camera::setFromOrientation(const glm::vec3& position, const glm::quat& orientation)
{
    glm::vec3 forward = orientation * glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up = orientation * glm::vec3(0.0f, 1.0f, 0.0f);
    setTransform(position, forward, up);
}

void Camera::updateVectors()
{
    glm::vec3 front;
    front.x = std::cos(glm::radians(Yaw)) * std::cos(glm::radians(Pitch));
    front.y = std::sin(glm::radians(Pitch));
    front.z = std::sin(glm::radians(Yaw)) * std::cos(glm::radians(Pitch));

    Front = glm::normalize(front);
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up = glm::normalize(glm::cross(Right, Front));
}
