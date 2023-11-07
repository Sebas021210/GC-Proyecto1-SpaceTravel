#pragma once
#include "glm/glm.hpp"
#include "fragment.h"

struct Uniforms {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 viewport;
    ObjectType objectType;
    float time;
};