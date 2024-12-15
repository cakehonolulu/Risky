cmake_minimum_required(VERSION 3.26)
project(risky LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CXX_STANDARD_REQUIRED ON)

add_compile_options(-fsanitize=address -g)
add_link_options(-fsanitize=address -g)

# Option to choose the frontend (default: ImGui)
option(USE_IMGUI_FRONTEND "Use ImGui frontend" ON)
option(USE_RAYLIB_FRONTEND "Use RayLib frontend" OFF)
option(USE_HEADLESS_FRONTEND "Use headless frontend" OFF)
option(RUN_TESTS "Enable testing" OFF)

# Both USE_IMGUI_FRONTEND and USE_HEADLESS_FRONTEND options can't be active at the same time, check against it
if(USE_IMGUI_FRONTEND AND USE_HEADLESS_FRONTEND AND USE_RAYLIB_FRONTEND)
    message(FATAL_ERROR "Can't enable ImGui, Raylib and Headless frontends at the same time!")
endif()

# Check if there's one of the frontend options enabled
if(NOT USE_IMGUI_FRONTEND AND NOT USE_HEADLESS_FRONTEND AND NOT USE_RAYLIB_FRONTEND AND NOT RUN_TESTS)
    message(FATAL_ERROR "No frontend selected, please enable ImGui, Raylib, Headless frontends, or testing!")
endif()

# Specify the main executable
add_executable(risky)

# Add the src/ subdirectory
add_subdirectory(src)

# Add frontend or testing based on options
if (RUN_TESTS)
    add_subdirectory(tests)
else()
    if(NOT USE_IMGUI_FRONTEND AND NOT USE_HEADLESS_FRONTEND AND NOT USE_RAYLIB_FRONTEND)
        message(FATAL_ERROR "No frontend selected, please enable ImGui, Raylib or Headless frontend!")
    else()
        add_subdirectory(frontend)
    endif()
endif()

# Add the required headers to the project (Found in include/)
target_include_directories(risky PRIVATE ${PROJECT_SOURCE_DIR}/include)
