#pragma once

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

class Camera {
private:
    // translation controls
    glm::vec3 up;
    // direction controls
    float mouseSens = 0.15f;
    float oldMouseX = 800.f / 2.f;
    float oldMouseY = 600.f / 2.f;
    float yaw = -90.f;
    float pitch = 0.f;

    void move(float deltaTime, glm::vec3 direction) {
        pos += speed * deltaTime * direction;
    }

public:
    // speed
    float speed = 20.f;
    // translation controls 
    glm::vec3 pos;
    glm::vec3 front;
    // direction controls
    bool firstMouseMovement = true;

    Camera(glm::vec3 pos, glm::vec3 front, glm::vec3 up) : pos(pos), front(front), up(up) {} 

    void updateDirection(double x, double y) {
        if (firstMouseMovement) {
            firstMouseMovement = false;
            oldMouseX = x;
            oldMouseY = y;
        }

        float xoffset = (x - oldMouseX)*mouseSens;
        float yoffset = (oldMouseY - y)*mouseSens;

        oldMouseX = x;
        oldMouseY = y;

        yaw += xoffset;
        pitch += yoffset;

        // restrict pitch to 180 degrees infront of camera.
        if (pitch > 89.f) pitch = 89.f;
        else if (pitch < -89.f) pitch = -89.f;

        glm::vec3 direction;
        direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction.y = sin(glm::radians(pitch));
        direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(direction);
    }

    glm::mat4 getViewMatrix() {
        return glm::lookAt(pos, pos+front, up);
    }

    glm::vec3 getPos() const {
        return pos;
    }

    float getOldMouseX() {
        return oldMouseX;
    }

    float getOldMouseY() {
        return oldMouseY;
    }

    void moveForward(float deltaTime) {
        move(deltaTime, front);
    }

    void moveBackward(float deltaTime) { 
        move(deltaTime, -front);
    }

    void moveRight(float deltaTime) { 
        move(deltaTime, glm::normalize(glm::cross(front, up)));
    }

    void moveLeft(float deltaTime) {
        move(deltaTime, -glm::normalize(glm::cross(front, up)));
    }

    void moveUp(float deltaTime) { 
        move(deltaTime, up);
    }

    void moveDown(float deltaTime) {
        move(deltaTime, -up);
    }
};