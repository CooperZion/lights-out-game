#ifndef GRAPHICS_LIGHT_H
#define GRAPHICS_LIGHT_H

#include <GLFW/glfw3.h>
#include "../shapes/rect.h"
#include "../shapes/shape.h"
#include <memory>

using namespace std;

class light {
public:
    bool isOn;
    unique_ptr<Rect> shape;

private:

};


#endif //GRAPHICS_LIGHT_H
