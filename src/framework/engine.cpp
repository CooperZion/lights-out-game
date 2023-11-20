#include "engine.h"
#include <iostream>
#include <string>

using namespace std;

// Color objects
const color WHITE(1, 1, 1);
const color GRAY(0.5, 0.5, 0.5);
const color BLACK(0, 0, 0);
const color YELLOW(1, 1, 0);
const color RED(1, 0, 0);

// vec3 object representing the color for comparisons
vec3 YELLOW_VECT = {YELLOW.red, YELLOW.green, YELLOW.blue};

// counter for moves
static int moveCount = 0;

// Time keeping variables
static int startTime, endTime;


enum state {start, instructions, play, over};
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
    fontRenderer = make_unique<FontRenderer>(shaderManager->getShader("text"), "../res/fonts/MxPlus_IBM_BIOS.ttf", FONT_SIZE);

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

    switch (screen) {
        case start: {
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                startTime = clock();
                screen = play;
            } else if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) { screen = instructions; }
            break;
        }
        case instructions: {
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                startTime = clock();
                screen = play;
            }
            break;
        }
        case play: {
            // Variable to hold mouse press status
            bool mousePressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

            // If the mouse has been pressed and released, we're gonna do a bunch of stuff
            // For all the lights, if they were overlapping, invert the color of it and
            // the color of its neighbors. The logic for the neighbors is brute-forced
            // because I didn't have time to write a generalized system. Save the hovered-over
            // node to render its outline
            for (int ii = 0; ii < lights.size(); ii++) {
                if (lights[ii]->isOverlapping(*cursor)) {
                    if (!mousePressed && mousePressedLastFrame) {
                        moveCount++;
                        // All this mess is the logic making vectors of the lights to invert
                        if (ii == 0) { neighborVect = {ii, ii + 1, ii + 5}; }
                        else if (ii == 4) { neighborVect = {ii, ii - 1, ii + 5}; }
                        else if (ii == 20) { neighborVect = {ii, ii + 1, ii - 5}; }
                        else if (ii == 24) { neighborVect = {ii, ii - 1, ii - 5}; }
                        else if (ii >= 1 && ii <= 3) { neighborVect = {ii, ii - 1, ii + 1, ii + 5}; }
                        else if (ii >= 21 && ii <= 23) { neighborVect = {ii, ii - 1, ii + 1, ii - 5}; }
                        else if (ii == 5 || ii == 10 || ii == 15) { neighborVect = {ii, ii + 1, ii + 5, ii - 5}; }
                        else if (ii == 9 || ii == 14 || ii == 19) { neighborVect = {ii, ii - 1, ii + 5, ii - 5}; }
                        else { neighborVect = {ii, ii - 1, ii + 1, ii - 5, ii + 5}; }
                        // Invert each light
                        for (int jj: neighborVect) {
                            if (lights[jj]->getColor3() == YELLOW_VECT) { lights[jj]->setColor(GRAY); }
                            else { lights[jj]->setColor(YELLOW); }
                        }
                    }
                    // Save the index of the hovered index
                    hoverIndices.push_back(ii);
                }
            }
            // Save the status of the mouse press
            mousePressedLastFrame = mousePressed;
            break;
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
            endTime = clock();
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
            // TODO: Add instructions screen with text
            // Display intro screen
            string titleMessage = "Lights Out!";
            string commandMessage1 = "Commands:";
            string commandMessage2 = "[i] to show the directions";
            string commandMessage3 = "[s] to launch the game";
            string commandMessage4 = "[Esc] to quit";
            this->fontRenderer->renderText(titleMessage, 260, 500, 1, vec3{WHITE.red, WHITE.green, WHITE.blue});
            this->fontRenderer->renderText(commandMessage1, 290, 270, 1, vec3{WHITE.red, WHITE.green, WHITE.blue});
            this->fontRenderer->renderText(commandMessage2, 100, 240, 1, vec3{WHITE.red, WHITE.green, WHITE.blue});
            this->fontRenderer->renderText(commandMessage3, 140, 210, 1, vec3{WHITE.red, WHITE.green, WHITE.blue});
            this->fontRenderer->renderText(commandMessage4, 250, 180, 1, vec3{WHITE.red, WHITE.green, WHITE.blue});
            break;
        }
        case instructions: {
            string titleMessage = "Lights Out!";
            string instructionMessage1 = "The goal of this game is to";
            string instructionMessage2 = "turn off all the lights.";
            string instructionMessage3 = "Clicking a light inverts it as";
            string instructionMessage4 = "well as its immediate neighbors.";
            string instructionMessage5 = "Press [s] to launch the game when ready";
            this->fontRenderer->renderText(titleMessage, 260, 500, 1, vec3{WHITE.red, WHITE.green, WHITE.blue});
            this->fontRenderer->renderText(instructionMessage1, 140, 300, 0.8, vec3{WHITE.red, WHITE.green, WHITE.blue});
            this->fontRenderer->renderText(instructionMessage2, 170, 270, 0.8, vec3{WHITE.red, WHITE.green, WHITE.blue});
            this->fontRenderer->renderText(instructionMessage3, 110, 240, 0.8, vec3{WHITE.red, WHITE.green, WHITE.blue});
            this->fontRenderer->renderText(instructionMessage4, 95, 210, 0.8, vec3{WHITE.red, WHITE.green, WHITE.blue});
            this->fontRenderer->renderText(instructionMessage5, 30, 180, 0.8, vec3{WHITE.red, WHITE.green, WHITE.blue});
            break;
        }
        case play: {
            // Show the light squares and the hover outlines if there are any
            for(int ii = 0; ii < hoverIndices.size(); ii++) {
                redOutline[hoverIndices[ii]]->setUniforms();
                redOutline[hoverIndices[ii]]->draw();
                hoverIndices.pop_back();
            }
            for (const unique_ptr<Rect> &light: lights) {
                light->setUniforms();
                light->draw();
            }

            // Display the moves taken and the timer
            string movesMessage = "Moves: " + to_string(moveCount);
            string timerMessage = "Time: " + to_string((clock() - startTime) / 1000);
            this->fontRenderer->renderText(movesMessage, 550, 400, 1, vec3{WHITE.red, WHITE.green, WHITE.blue});
            this->fontRenderer->renderText(timerMessage, 550, 200, 1, vec3{WHITE.red, WHITE.green, WHITE.blue});

            break;
        }
        case over: {
            // Show the lights all turned off
            for (const unique_ptr<Rect> &light: lights) {
                light->setUniforms();
                light->draw();
            }

            // Show win message
            string winMessage = "Winner!";
            string movesMessage = "Moves: " + to_string(moveCount);
            string timerMessage = "Time: " + to_string((endTime - startTime) / 1000);
            this->fontRenderer->renderText(winMessage, 220, 290, 1, vec3{WHITE.red, WHITE.green, WHITE.blue});
            this->fontRenderer->renderText(movesMessage, 550, 400, 1, vec3{WHITE.red, WHITE.green, WHITE.blue});
            this->fontRenderer->renderText(timerMessage, 550, 200, 1, vec3{WHITE.red, WHITE.green, WHITE.blue});
            break;
        }
    }
    cursor->setUniforms();
    cursor->draw();
    glfwSwapBuffers(window);
}

bool Engine::shouldClose() {
    return glfwWindowShouldClose(window);
}