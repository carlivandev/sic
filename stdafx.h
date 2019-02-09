#ifndef PCH_H
#define PCH_H

#include "defines.h"

#include <assert.h>
#include <vector>
#include <type_traits>
#include <memory>
#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <optional>
#include <unordered_set>
#include <set>
#include <algorithm>
#include <fstream>
#include <thread>
#include <string>
#include <array>

#pragma warning(push, 0)

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/glm.hpp>

#pragma warning(pop)

#endif //PCH_H
