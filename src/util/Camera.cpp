#include "util/Camera.hpp"
#include "BaseScene.hpp"
namespace vulpes {

    void Camera::UpdateMousePos(const float & x, const float & y) {
        float x_offset = x * BaseScene::SceneConfiguration.MouseSensitivity;
        float y_offset = y * BaseScene::SceneConfiguration.MouseSensitivity;

        Yaw += x_offset;
        Pitch += y_offset;

        // Make sure that when pitch is out of bounds, screen doesn't get flipped

        {
            if (Pitch > 89.0f) {
                Pitch = 89.0f;
            }
            if (Pitch < -89.0f) {
                Pitch = -89.0f;
            }
        }

        // Update Front, Right and Up Vectors using the updated Eular angles
        updateCameraVectors();

    }
    void vulpes::Camera::updateCameraVectors() {

        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);

        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up = glm::normalize(glm::cross(Right, Front));

    }

    void cameraBase::ProcessKeyboard(const Direction & dir, const float & delta_time) {
        float velocity = BaseScene::SceneConfiguration.MovementSpeed * delta_time;
        switch (dir) {
        case Direction::FORWARD:
            translate(Front, velocity);
            break;
        case Direction::BACKWARD:
            translate(-Front, velocity);
            break;
        case Direction::LEFT:
            translate(-Right, velocity);
            break;
        case Direction::RIGHT:
            translate(Right, velocity);
            break;
        case Direction::UP:
            translate(Up, velocity);
            break;
        case Direction::DOWN:
            translate(-Up, velocity);
            break;
        }
    }

}