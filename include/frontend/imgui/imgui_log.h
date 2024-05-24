#pragma once

#include <vector>
#include <string>

#include <log/log.h>

namespace ImGuiLogger
{
	// Custom log function for ImGui frontend
	void ImGuiLogFunction(const std::string& message);

	// Initialize ImGui Logger
	void InitializeImGuiLogger();

	// Access the ImGui log buffer
	const std::vector<std::pair<std::string, LogLevel>>& GetImGuiLogBuffer();

    // Access the ImGui UART buffer
    const std::vector<uint8_t>& GetImGuiUARTBuffer();

	// Clear the ImGui log buffer
	void ClearImGuiLogBuffer();

    // Clear the ImGui UART buffer
    void ClearImGuiUARTBuffer();
}
