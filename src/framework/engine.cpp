#include "engine.h"
#include <iostream>

const color WHITE(1, 1, 1);
vec3 WHITE_VECT = {WHITE.red, WHITE.green, WHITE.blue};
const color BLACK(0, 0, 0);
const color YELLOW(1, 1, 0);
vec3 YELLOW_VECT = {YELLOW.red, YELLOW.green, YELLOW.blue};
const color RED(1, 0, 0);
int NUM_LIGHTS = 25;
static int moveCount = 0;


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
    shapeShader = this->shaderManager->loadShader("../res/shaders/rect.vert",
                                                  "../res/shaders/rect.frag",
                                                  nullptr, "rect");
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

    for (int column = 0; column < 5; column++) {
        for (int row = 0; row < 5; row++) {
            // TODO: Change this to be the correct vector of coordinates, may need to be done manually
            // This will probably just need to be guess-and-check
            coordinateMatrix.push_back({(int) (0.2 * column * WIDTH), (int) (0.2 * row * HEIGHT)});
        }
    }

    for(int ii = 0; ii < NUM_LIGHTS; ii++) {
        vector<int> coordVect = coordinateMatrix[ii];
        lights.push_back(make_unique<Rect>(shapeShader, vec2{coordVect[0], coordVect[1]}, vec2{WIDTH / 4, HEIGHT / 2},
                                           color{YELLOW.red, YELLOW.green, YELLOW.blue, YELLOW.alpha}));
    }
}

void Engine::processInput() {
    glfwPollEvents();

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
        screen = play;
    }

    if (screen == play) {
        if (glfwGetKey(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            for (const unique_ptr<Rect> &light: lights) {
                if (light->isOverlapping(*cursor)) {
                    moveCount++;
                    if (light->getColor3() == WHITE_VECT) { light->setColor(YELLOW); }
                    else { light->setColor(WHITE); }
                    // TODO: Change the color of the neighboring lights
                }
            }
        }

        for(const unique_ptr<Rect> &light : lights) {
            if(light->isOverlapping(*cursor)) {
                // TODO: Make the lights outline in red here
                // There will be a second set of rectangles with the same center coordinates
                // as the first set, but slightly larger and red, and they're only rendered
                // when their accompanying rectangle is hovered over (this if-statement)
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
        bool allLightsOff = true;
        for (const unique_ptr<Rect> &light: lights) {
            if (light->getColor3() == YELLOW_VECT) {
                allLightsOff = false;
            }
        }
        if (allLightsOff) {screen = over;}
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
            this->fontRenderer->renderText(message2, 200, 320 - fontSize - 4, 1, vec3{WHITE.red, WHITE.green, WHITE.blue});
            break;
        }
        case play: {
            for (const unique_ptr<Rect> &light: lights) {
                light->setUniforms();
                light->draw();
            }
        }
        case over: {
            // Show win message
            // TODO: Add instructions screen with text (freetype?)
            string message = "Winner!";
            // (12 * message.length()) is the offset to center text.
            // 12 pixels is the width of each character scaled by 1.
            this->fontRenderer->renderText(message, 320, 320 - fontSize - 4, 1, vec3{WHITE.red, WHITE.green, WHITE.blue});
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