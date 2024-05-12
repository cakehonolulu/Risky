#include <log/log.h>
#include <imgui.h>

namespace ImGuiLogger
{
	static std::vector<std::pair<std::string, LogLevel>> ImGuiLogBuffer;

	void ImGuiLogFunction(const std::string& message) {
		ImGuiLogBuffer.push_back({message, LogLevel::Info});
	}

	void ImGuiErrorFunction(const std::string& message) {
		ImGuiLogBuffer.push_back({message, LogLevel::Error});
	}

	void ImGuiWarnFunction(const std::string& message) {
		ImGuiLogBuffer.push_back({message, LogLevel::Warning});
	}

	void InitializeImGuiLogger()
	{
		Logger::Instance().SetLogFunction(ImGuiLogFunction);
		Logger::Instance().SetErrorFunction(ImGuiErrorFunction);
		Logger::Instance().SetWarnFunction(ImGuiWarnFunction);
	}

	const std::vector<std::pair<std::string, LogLevel>>& GetImGuiLogBuffer()
	{
		return ImGuiLogBuffer;
	}

	void ClearImGuiLogBuffer()
	{
		ImGuiLogBuffer.clear();
	}
}
