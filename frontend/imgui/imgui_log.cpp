#include <log/log.h>
#include <imgui.h>

namespace ImGuiLogger
{
	static std::vector<std::pair<std::string, LogLevel>> ImGuiLogBuffer;
    static std::vector<uint8_t> ImGuiUARTBuffer;

	void ImGuiLogFunction(const std::string& message) {
		ImGuiLogBuffer.push_back({message, LogLevel::Info});
	}

	void ImGuiErrorFunction(const std::string& message) {
		ImGuiLogBuffer.push_back({message, LogLevel::Error});
	}

	void ImGuiWarnFunction(const std::string& message) {
		ImGuiLogBuffer.push_back({message, LogLevel::Warning});
	}

    void ImGuiUARTFunction(std::uint8_t byte) {
        ImGuiUARTBuffer.push_back(byte);
    }

	void InitializeImGuiLogger()
	{
		Logger::Instance().SetLogFunction(ImGuiLogFunction);
		Logger::Instance().SetErrorFunction(ImGuiErrorFunction);
		Logger::Instance().SetWarnFunction(ImGuiWarnFunction);
        Logger::Instance().SetUARTFunction(ImGuiUARTFunction);
	}

	const std::vector<std::pair<std::string, LogLevel>>& GetImGuiLogBuffer()
	{
		return ImGuiLogBuffer;
	}

    const std::vector<uint8_t>& GetImGuiUARTBuffer()
    {
        return ImGuiUARTBuffer;
    }

    void ClearImGuiLogBuffer()
	{
		ImGuiLogBuffer.clear();
	}

    void ClearImGuiUARTBuffer()
    {
        ImGuiUARTBuffer.clear();
    }
}
