#include <risky.h>
#include <cstdlib>
#include <cstdint>
#include <string>
#include <sstream>
#include <typeinfo>
#include <log/log_imgui.hh>
#include <log/log_term.hh>

std::string colorCode(int r, int g, int b) {
    return "\033[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
}

std::string backgroundColorCode(int r, int g, int b) {
    return "\033[48;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
}

std::string resetColor() {
    return "\033[0m";
}

int Risky::exit(int code, Subsystem subsystem) {
    aborted = true;
    guilty_subsystem = subsystem;
    return code;
}

Risky::Risky(std::shared_ptr<LogBackend> logger) {
    if (logger) {
        Logger::add_backend(logger);
    }

    // Define the banner
    std::string banner =
        "  ____  _     _          \n"
        " |  _ \\(_)___| | ___   _ \n"
        " | |_) | / __| |/ / | | |\n"
        " |  _ <| \\__ \\   <| |_| |\n"
        " |_| \\_\\_|___/_|\\_\\\\__, |\n"
        "                    |___/   - A simple RISC-V emulator\n";

    int r_start = 238;
    int g_start = 255;
    int b_start = 51;

    int r_end = 255;
    int g_end = 87;
    int b_end = 51;

    int lines = 6;

    std::istringstream bannerStream(banner);
    std::string line;
    int line_index = 0;

    // Detect backend type
    bool is_terminal = false;
    ImGuiLogBackend* imgui_backend = nullptr;

    if (!Logger::get_backends().empty()) {
        // Check if the logger is an instance of ImGuiLogBackend
        imgui_backend = dynamic_cast<ImGuiLogBackend*>(Logger::get_backends().front().get());
        is_terminal = dynamic_cast<TerminalLogBackend*>(Logger::get_backends().front().get()) != nullptr;
    }

    // Print each line of the banner based on frontend type
    while (std::getline(bannerStream, line)) {
        float ratio = (float)line_index / (lines - 1);
        int r = r_start + static_cast<int>((r_end - r_start) * ratio);
        int g = g_start + static_cast<int>((g_end - g_start) * ratio);
        int b = b_start + static_cast<int>((b_end - b_start) * ratio);

        if (is_terminal) {
            Logger::raw("\033[1m" + backgroundColorCode(r, g, b) + colorCode(255, 255, 255) + line + resetColor() + "\n");
        } else if (imgui_backend) {
            std::ostringstream oss;
            oss << "[fg_color(255,255,255),bg_color(" << r << "," << g << "," << b << ")]" << line;
            imgui_backend->raw_special(oss.str(), LogLevel::Raw);
        }

        line_index++;
    }
}

Risky::~Risky()
{
    Logger::raw("project - Shutting down project system.");
}
