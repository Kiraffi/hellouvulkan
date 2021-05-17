#pragma once


struct GLFWwindow;
struct Camera;

void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
void checkKeypresses(float deltaTime, Camera &camera);
