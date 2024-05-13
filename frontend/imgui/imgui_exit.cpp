#include <frontend/imgui/imgui_exit.h>
#include <risky.h>

void ImGuiExitSystem::exitApplication() {
    // Set ApplicationManger::requested_exit to true
    Risky::requested_exit = true;

    // Set ApplicationManger::abort to true
    Risky::abort = true;
}
