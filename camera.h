#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera
{
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
public:
    Camera();
    Camera(glm::vec3 pos, glm::vec3 target, glm::vec3 up);

    // Returns the view matrix
    glm::mat4 GetViewMatrix() const
    {
        return glm::lookAt(position, position + front, up);
    }

    void MoveForward(float delta);

};

#endif // CAMERA_H
