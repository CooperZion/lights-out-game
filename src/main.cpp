#include "framework/engine.h"

#include <iostream>


int main(int argc, char *argv[]) {
    glfwInit();
    Engine engine;

    while (!engine.shouldClose()) {
        engine.processInput();
        engine.update();
        engine.render();
    }

    glfwTerminate();
    return 0;
}