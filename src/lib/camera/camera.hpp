#pragma once

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include <algorithm>
#include <cmath>

class Camera {
private:
    // translation controls
    glm::vec3 up;
    // direction controls
    float oldMouseX = 800.f / 2.f;
    float oldMouseY = 600.f / 2.f;
    float yaw = -90.f;
    float pitch = 0.f;

    void move(float deltaTime, glm::vec3 direction) {
        pos += speed * deltaTime * direction;
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

public:
    // direction controls
    float mouseSens = 0.15f;
    // speed
    float speed = 20.f;
    // translation controls 
    glm::vec3 pos;
    glm::vec3 front;
    // actions
    bool actionForwards, actionBackwards, actionLeft, actionRight, actionUp, actionDown;

    Camera(glm::vec3 pos, glm::vec3 front, glm::vec3 up) : pos(pos), front(glm::normalize(front)), up(up) {
        // updateCamera rebuilds `front` from yaw and pitch, so the spawn direction has to land there.
        const float y = std::clamp(this->front.y, -1.f, 1.f);
        pitch = glm::degrees(std::asin(y));
        yaw = glm::degrees(glm::atan(this->front.z, this->front.x));
    } 

    // dx is to the right, dy is downward, both in pixels.
    void addLook(float dx, float dy) {
        yaw += dx * mouseSens;
        pitch -= dy * mouseSens;
        if (pitch > 89.f) pitch = 89.f;
        else if (pitch < -89.f) pitch = -89.f;
    }

    void updateDirection(double x, double y) {
        addLook(static_cast<float>(x - oldMouseX), static_cast<float>(y - oldMouseY));
        oldMouseX = static_cast<float>(x);
        oldMouseY = static_cast<float>(y);
    }

    void updateMousePos(double x, double y) {
        oldMouseX = x;
        oldMouseY = y;
    }

    void updateCamera(float dt) {
        glm::vec3 direction;
        direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction.y = sin(glm::radians(pitch));
        direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(direction);

        if (actionForwards) moveForward(dt);
        if (actionBackwards) moveBackward(dt);
        if (actionLeft) moveLeft(dt);
        if (actionRight) moveRight(dt);
        if (actionUp) moveUp(dt);
        if (actionDown) moveDown(dt);
    }

    void disableActions() {
        actionForwards = false;
        actionBackwards = false;
        actionLeft = false;
        actionRight = false;
        actionUp = false;
        actionDown = false;
    }

    glm::mat4 getViewMatrix() {
        return glm::lookAt(pos, pos+front, up);
    }

    glm::vec3 getPos() const {
        return pos;
    }
};