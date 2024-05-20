# Check if USE_IMGUI_FRONTEND is enabled
if(USE_IMGUI_FRONTEND)
    add_subdirectory(imgui)
endif()

# Check if USE_RAYLIB_FRONTEND is enabled
if(USE_RAYLIB_FRONTEND)
    add_subdirectory(raylib)
endif()

# Check if USE_HEADLESS_FRONTEND is enabled
if (USE_HEADLESS_FRONTEND)

endif ()