#include "engine.h"
#include <iostream>

const color WHITE(1, 1, 1);
const color GRAY(0.5, 0.5, 0.5);
vec3 WHITE_VECT = {WHITE.red, WHITE.green, WHITE.blue};
const color BLACK(0, 0, 0);
const color YELLOW(1, 1, 0);
vec3 YELLOW_VECT = {YELLOW.red, YELLOW.green, YELLOW.blue};
const color RED(1, 0, 0);
int NUM_LIGHTS = 25;
static int moveCount = 0;
static vector<int> hoverIndices;


enum state {start, play, over};
state screen = start;

Engine::Engine() {
    this->initWindow();
    this->initShaders();
    this->initShapes();
}

Engine::~Engine() = default;

unsigned int Engine::initWindow(bool debug) {
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);
#endif
    glfwWindowHint(GLFW_RESIZABLE, false);

    window = glfwCreateWindow(WIDTH, HEIGHT, "engine", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        cout << "Failed to initialize GLAD" << endl;
        return -1;
    }

    // OpenGL configuration
    glViewport(0, 0, WIDTH, HEIGHT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glfwSwapInterval(1);

    return 0;
}

void Engine::initShaders() {
    shaderManager = make_unique<ShaderManager>();
    shapeShader = this->shaderManager->loadShader("../res/shaders/shape.vert",
                                                  "../res/shaders/shape.frag",
                                                  nullptr, "shape");
    shapeShader.use();
    shapeShader.setMatrix4("projection", this->PROJECTION);

    // Configure text shader and renderer
    textShader = shaderManager->loadShader("../res/shaders/text.vert", "../res/shaders/text.frag", nullptr, "text");
    fontRenderer = make_unique<FontRenderer>(shaderManager->getShader("text"), "../res/fonts/MxPlus_IBM_BIOS.ttf", fontSize);

    // Set uniforms
    textShader.use().setVector2f("vertex", vec4(100, 100, .5, .5));
    shapeShader.use().setMatrix4("projection", this->PROJECTION);
}

void Engine::initShapes() {
    // Shape object for the cursor
    cursor = make_unique<Rect>(shapeShader, vec2(10, 10), vec2(0, 0), WHITE);

    for (int column = 1; column <= 5; column++) {
        for (int row = 1; row <= 5; row++) {
            // This will probably just need to be guess-and-check
            coordinateMatrix.push_back({column * 160, row * 160});
        }
    }


    for(int ii = 0; ii < NUM_LIGHTS; ii++) {
        vector<int> coordVect = coordinateMatrix[ii];
        lights.push_back(make_unique<Rect>(shapeShader, vec2{coordVect[0], coordVect[1]}, vec2{140, 140},
                                           color{YELLOW.red, YELLOW.green, YELLOW.blue, YELLOW.alpha}));
        redOutline.push_back(make_unique<Rect>(shapeShader, vec2{coordVect[0], coordVect[1]}, vec2{155, 155},
                                           color{RED.red, RED.green, RED.blue, RED.alpha}));
    }
}

void Engine::processInput() {
    glfwPollEvents();

    static bool mousePressedLastFrame = false;
    vector<int> neighborVect;

    // Close window if escape key is pressed
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    // Mouse position saved to check for collisions
    glfwGetCursorPos(window, &mouseX, &mouseY);
    mouseY = HEIGHT - mouseY; // make sure mouse y-axis isn't flipped

    cursor->setPosX(mouseX);
    cursor->setPosY(mouseY);

    if (screen == start && glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        // TODO: Start timer here
        screen = play;
    }

    if (screen == play) {
        // Variable to track mouse press
        bool mousePressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

        // If the mouse has been pressed and released, we're gonna do a bunch of stuff
        if (!mousePressed && mousePressedLastFrame) {
            // For all the lights, if they were overlapping, invert the color of it and
            // the color of its neighbors. The logic for the neighbors is brute-forced
            // because I didn't have time to write a generalized system
            for (int ii = 0; ii < lights.size(); ii++) {
                if (lights[ii]->isOverlapping(*cursor)) {
                    moveCount++;
                    if (ii == 0) {neighborVect = {ii, ii + 1, ii + 5};}
                    else if (ii == 4) {neighborVect = {ii, ii - 1, ii + 5};}
                    else if (ii == 20) {neighborVect = {ii, ii + 1, ii - 5};}
                    else if (ii == 24) {neighborVect = {ii, ii - 1, ii - 5};}
                    else if (ii >= 1 && ii <= 3) {neighborVect = {ii, ii - 1, ii + 1, ii + 5};}
                    else if (ii >= 21 && ii <= 23) {neighborVect = {ii, ii - 1, ii + 1, ii - 5};}
                    else if (ii == 5 || ii == 10 || ii == 15) {neighborVect = {ii, ii + 1, ii + 5, ii - 5};}
                    else if (ii == 9 || ii == 14 || ii == 19) {neighborVect = {ii, ii - 1, ii + 5, ii - 5};}
                    else {neighborVect = {ii, ii - 1, ii + 1, ii - 5, ii + 5};}
                    for (int jj : neighborVect) {
                        if (lights[jj]->getColor3() == YELLOW_VECT) {lights[jj]->setColor(GRAY);}
                        else {lights[jj]->setColor(YELLOW);}
                    }
                }
            }
        }
        mousePressedLastFrame = mousePressed;

        // Save the index of hovered-over lights to render the outline shapes
        for(int ii = 0; ii < lights.size(); ii++) {
            if(lights[ii]->isOverlapping(*cursor)) {
                hoverIndices.push_back(ii);
            }
        }
    }
}

void Engine::update() {
    // Change values of objects
    // Calculate delta time
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    // If we're playing and all the lights are off, change screen to over (end the game)
    if (screen == play) {
        allLightsOff = true;
        for (const unique_ptr<Rect> &light: lights) {
            if (light->getRed() == YELLOW.red &&
                light->getGreen() == YELLOW.green &&
                light->getBlue() == YELLOW.blue) {
                allLightsOff = false;
            }
        }
        if (allLightsOff) {
            // TODO: End timer here
            screen = over;
        }
    }
}

void Engine::render() {
    // Draw objects
    glClearColor(BLACK.red, BLACK.green, BLACK.blue, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    shapeShader.use();

    switch (screen) {
        case start: {
            // Add instructions screen with text (freetype?)
            string message1 = "Put out all the lights!";
            string message2 = "Press s to begin";
            this->fontRenderer->renderText(message1, 130, 320, 1, vec3{WHITE.red, WHITE.green, WHITE.blue});
            this->fontRenderer->renderText(message2, 200, 280, 1, vec3{WHITE.red, WHITE.green, WHITE.blue});
            break;
        }
        case play: {
            for(int ii = 0; ii < hoverIndices.size(); ii++) {
                redOutline[hoverIndices[ii]]->setUniforms();
                redOutline[hoverIndices[ii]]->draw();
                hoverIndices.pop_back();
            }
            for (const unique_ptr<Rect> &light: lights) {
                light->setUniforms();
                light->draw();
            }
            break;
        }
        case over: {
            // Show win message
            string message = "Winner!";
            for(int ii = 0; ii < hoverIndices.size(); ii++) {
                redOutline[hoverIndices[ii]]->setUniforms();
                redOutline[hoverIndices[ii]]->draw();
                hoverIndices.pop_back();
            }
            for (const unique_ptr<Rect> &light: lights) {
                light->setUniforms();
                light->draw();
            }
            this->fontRenderer->renderText(message, 320, 280, 1, vec3{WHITE.red, WHITE.green, WHITE.blue});
            break;
        }
    }

//    cursor->setUniforms();
//    cursor->draw();

    glfwSwapBuffers(window);
}

bool Engine::shouldClose() {
    return glfwWindowShouldClose(window);
}