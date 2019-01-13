#include "camera.h"

Camera::Camera()
    : position(0.0f, 0.0f, 0.0f),
      front(0.0f, 0.0f, -1.0f),
      up(0.0f, 1.0f, 0.0f)
{
}

Camera::Camera(glm::vec3 pos, glm::vec3 target, glm::vec3 upv)
    : position(pos),
      front(glm::normalize(target - pos)),
      up(upv)
{
}

void Camera::MoveForward(float delta)
{
    position += front * delta;
}
