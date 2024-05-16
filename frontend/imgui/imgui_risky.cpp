#include <frontend/imgui/imgui_risky.h>

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include "cpu/core/rv32i.h"
#include "ImGuiFileDialog.h"
#include "cpu/core/rv64i.h"
#include "cpu/core/rv32e.h"
#include "cpu/core/core.h"
#include <cstdio>
#include <SDL3/SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL3/SDL_opengles2.h>
#else
#include <SDL3/SDL_opengl.h>
#include <vector>

#include <frontend/imgui/imgui_log.h>
#include <cpu/registers.h>
#include <cpu/disassembler.h>

#endif

// This example doesn't compile with Emscripten yet! Awaiting SDL3 support.
#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

#include <frontend/imgui/imgui_exit.h>

void ImGui_Risky::init() {

}

void ImGui_Risky::run() {
    ImGuiExitSystem imguiExitSystem;
    Risky::setExitSystem(&imguiExitSystem);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMEPAD) != 0)
	{
		printf("Error: SDL_Init(): %s\n", SDL_GetError());
		return;
	}

	ImGuiLogger::InitializeImGuiLogger();

#if defined(IMGUI_IMPL_OPENGL_ES2)
	// GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
	// GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
	// GL 3.0 + GLSL 130
	const char* glsl_version = "#version 130";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

	SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	Uint32 window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN;
	SDL_Window* window = SDL_CreateWindow("Risky - RISC-V Emulator", 1280, 720, window_flags);
	if (window == nullptr)
	{
		printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
		return;
	}
	SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, gl_context);
	SDL_GL_SetSwapInterval(1);
	SDL_ShowWindow(window);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

	ImGui::StyleColorsDark();

	ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
	ImGui_ImplOpenGL3_Init(glsl_version);

	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	bool done = false;

	bool log_debug_window = true;
	bool core_config_window = true;
	bool cpu_reg_debug_window = false;
	bool disassembly_window = false;
	IGFD::FileDialogConfig config; config.path = ".";

	// 0: RV32I, 1: RV32E, 2: RV64I
	int selected_riscv_variant = 3;
	std::string core_;
	std::string xlen_;
	bool has_M_extension = false;
	bool has_A_extension = false;
    bool has_C_extension = false;
	bool has_Zicsr_extension = false;
	bool has_Zifence_extension = false;
	bool nommu = true;
    bool built_core = false;
    bool core_empty_alert = false;
    bool core_invalid_alert = false;
    bool loaded_binary = false;
    bool symbols_loaded = false;
    Core core;

#ifdef __EMSCRIPTEN__
	// For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
	while (!done)
#endif
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			ImGui_ImplSDL3_ProcessEvent(&event);
			if (event.type == SDL_EVENT_QUIT)
				done = true;
			if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
				done = true;
		}

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();

		if (core_empty_alert) {
			ImGui::OpenPopup("Load Binary Unavailable");
			if (ImGui::BeginPopupModal("Load Binary Unavailable", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
				ImGui::TextWrapped("You must create a RISC-V core first before loading a binary!");
				ImGui::Separator();

				if (ImGui::Button("OK", ImVec2(120, 0))) {
					core_empty_alert = false;
					ImGui::CloseCurrentPopup();
				}
				ImGui::SetItemDefaultFocus();

				ImGui::EndPopup();
			}
		}

        if (core_invalid_alert) {
            ImGui::OpenPopup("Invalid Core Configuration");
            if (ImGui::BeginPopupModal("Invalid Core Configuration", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::TextWrapped("You must select a valid core configuration!");
                ImGui::Separator();

                if (ImGui::Button("OK", ImVec2(120, 0))) {
                    core_invalid_alert = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SetItemDefaultFocus();

                ImGui::EndPopup();
            }
        }

		if (ImGui::BeginMainMenuBar())
		{
            if (built_core)
            {
                if (ImGui::BeginMenu("File"))
                {
                    if (ImGui::MenuItem("Open Binary", ""))
                    {
                        if (core_.empty())
                        {
                            core_empty_alert = true;
                        }
                        else
                        {
                            ImGuiFileDialog::Instance()->OpenDialog("BinaryLoadDlg", "Choose File", ".bin", config);
                        }
                    }

                    if (ImGui::MenuItem("Load Symbol File")) {
                        if (!symbols_loaded)
                        {
                            ImGuiFileDialog::Instance()->OpenDialog("SymbolsLoadDlg", "Choose File", ".map", config);
                        }
                    }

                    ImGui::EndMenu();
                }
            }

			if (ImGui::BeginMenu("Config"))
			{
				static bool menu_toggle_core_config_window = true;

				ImGui::MenuItem("Core", "", &menu_toggle_core_config_window, true);

				if (menu_toggle_core_config_window)
				{
					core_config_window = true;
				}
				else
				{
					core_config_window = false;
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Debug"))
			{
				static bool menu_toggle_cpu_reg_window = false;
				static bool menu_toggle_log_window = true;
				static bool menu_toggle_disassembler_window = false;

                ImGui::MenuItem("Log", "", &menu_toggle_log_window, true);

                if (built_core)
                {
                    ImGui::MenuItem("CPU Registers", "", &menu_toggle_cpu_reg_window, true);
                    ImGui::MenuItem("Disassembler", "", &menu_toggle_disassembler_window, true);
                }

				if (menu_toggle_cpu_reg_window)
				{
					cpu_reg_debug_window = true;
				}
				else
				{
					cpu_reg_debug_window = false;
				}

				if (menu_toggle_log_window)
				{
					log_debug_window = true;
				}
				else
				{
					log_debug_window = false;
				}

				if (menu_toggle_disassembler_window)
				{
					disassembly_window = true;
				}
				else
				{
					disassembly_window = false;
				}

				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

		{
			static float f = 0.0f;
			static int counter = 0;

			ImGui::Begin("ImGui Debug");

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
			ImGui::End();
		}

        if (Risky::requestedExit()) {
            ImGui::OpenPopup("Exception occurred");
        }

        if (ImGui::BeginPopupModal("Exception occurred", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("An error has occurred, check the log window!");

            ImGui::Text("Do you want to exit the application?");

            if (ImGui::Button("Yes")) {
                std::exit(0);
            }

            ImGui::SameLine();

            if (ImGui::Button("No")) {
                // Close the popup
                Risky::requested_exit = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

		if (disassembly_window)
		{
			Disassembler disassembler;

			ImGui::Text("PC:");

            ImGui::SameLine();

			// Input box to jump to a specific PC value
			static char jumpToAddressBuffer[9] = "00000000"; // Assumes 32-bit addresses
			ImGui::InputText("->", jumpToAddressBuffer,
			                 sizeof(jumpToAddressBuffer),
			                 ImGuiInputTextFlags_CharsHexadecimal);

			// Convert the input buffer to a uint32_t
			std::uint32_t jumpToAddress =
					std::strtoul(jumpToAddressBuffer, nullptr, 16);

            ImGui::SameLine();

			// Button to jump to the specified PC value
			if (ImGui::Button("Jump")) {
				if (core_.contains("RV32I"))
				{
					riscv_core_32->pc = jumpToAddress;
				}
				else if (core_.contains("RV32E"))
				{
					riscv_core_32e->pc = jumpToAddress;
				}
				else if (core_.contains("RV64I"))
				{
					riscv_core_64->pc = jumpToAddress;
				}
			}

            ImGui::Separator();

            if (Risky::aborted()) {
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f); // Reduce alpha for disabled appearance
                ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 1.0f); // Ensure full alpha for text in disabled button
                ImGui::ArrowButton("##PlayButton", ImGuiDir_Right);
                ImGui::PopStyleVar(2);
            } else {
                if (ImGui::ArrowButton("##PlayButton", ImGuiDir_Right)) {
                    if (core_.contains("RV32I")) {
                        riscv_core_32->run();
                    } else if (core_.contains("RV32E")) {
                        riscv_core_32e->run();
                    } else if (core_.contains("RV64I")) {
                        riscv_core_64->run();
                    }
                }
            }

            ImGui::SameLine();

            if (Risky::aborted()) {
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha,
                                    ImGui::GetStyle().Alpha * 0.5f); // Reduce alpha for disabled appearance
                ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 1.0f); // Ensure full alpha for text in disabled button
                ImGui::Button("Step");
                ImGui::PopStyleVar(2);
            }
            else
            {
                if (ImGui::Button("Step")) {
                    if (core_.contains("RV32I")) {
                        riscv_core_32->step();
                    } else if (core_.contains("RV32E")) {
                        riscv_core_32e->step();
                    } else if (core_.contains("RV64I")) {
                        riscv_core_64->step();
                    }
                }
            }

            if (Risky::aborted()) {
                ImGui::SameLine();
                if (ImGui::Button("Reset")) {
                    if (core_.contains("RV32I")) {
                        riscv_core_32->reset();
                        Risky::abort = false;
                    } else if (core_.contains("RV32E")) {
                        riscv_core_32e->reset();
                        Risky::abort = false;
                    } else if (core_.contains("RV64I")) {
                        riscv_core_64->reset();
                        Risky::abort = false;
                    }
                }
            }

			// ImGui window for disassembled code
			ImGui::BeginChild("Disassembly", ImVec2(0, 0), true);

			// Calculate the number of instructions to display based on the window size
			int numInstructions = ImGui::GetWindowHeight() / ImGui::GetTextLineHeight();

			// Get the current PC value from the CPU
			std::uint32_t currentPC;

			if (core_.contains("RV32I"))
			{
				currentPC = riscv_core_32->pc;
			}
			else if (core_.contains("RV32E"))
			{
				currentPC = riscv_core_32e->pc;
			}
			else if (core_.contains("RV64I"))
			{
				currentPC = riscv_core_64->pc;
			}

			// Loop to disassemble and display instructions
			ImGuiListClipper clipper;
			clipper.Begin(numInstructions); // Pass the number of items
			while (clipper.Step()) {
				for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
					// Disassemble the instruction at the current PC
					std::uint32_t opcode;
					if (core_.contains("RV32I"))
					{
						opcode = riscv_core_32->bus.read32(currentPC);
					}
					else if (core_.contains("RV32E"))
					{
						opcode = riscv_core_32e->bus.read32(currentPC);
					}
					else if (core_.contains("RV64I"))
					{
						opcode = riscv_core_64->bus.read32(currentPC);
					}

					std::string disassembly = disassembler.Disassemble(opcode);

                    char opcodeBuffer[12];
                    snprintf(opcodeBuffer, sizeof(opcodeBuffer), "%02X %02X %02X %02X",
                             (opcode >> 24) & 0xFF,
                             (opcode >> 16) & 0xFF,
                             (opcode >> 8) & 0xFF,
                             (opcode >> 0) & 0xFF);

                    // Check if it's a jump instruction (e.g., jal, jalr)
                    bool isJumpInstruction = (disassembly.find("jal") != std::string::npos || disassembly.find("jalr") != std::string::npos);

                    // Highlight the current instruction
                    bool isCurrentInstruction = false;
                    if (core_.contains("RV32I")) {
                        isCurrentInstruction = (currentPC == riscv_core_32->pc);
                    } else if (core_.contains("RV32E")) {
                        isCurrentInstruction = (currentPC == riscv_core_32e->pc);
                    } else if (core_.contains("RV64I")) {
                        isCurrentInstruction = (currentPC == riscv_core_64->pc);
                    }

                    if (isCurrentInstruction) {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f)); // Yellow text color
                    }

                    // Display the instruction with appropriate formatting
                    if (isJumpInstruction) {
                        std::istringstream iss(disassembly);
                        std::string opcode;
                        std::string rd;
                        std::string rs;
                        std::string immediate;

                        iss >> opcode;

                        if (opcode == "jal") {
                            iss >> rd >> immediate;
                            rd.erase(std::remove(rd.begin(), rd.end(), ','), rd.end());
                        } else if (opcode == "jalr") {
                            iss >> rd >> rs >> immediate;
                            rd.erase(std::remove(rd.begin(), rd.end(), ','), rd.end());
                            rs.erase(std::remove(rs.begin(), rs.end(), ','), rs.end());
                        }

                        std::uint32_t jumpOffset;
                        try {
                            jumpOffset = std::stoul(immediate);
                        } catch (const std::invalid_argument& e) {
                            Logger::Instance().Error("[DEBUGGER] Invalid jump offset!");
                            Risky::exit();
                        }

                        std::uint32_t jumpAddress = currentPC + jumpOffset;

                        auto jumpSymbol = symbols.find(jumpAddress);
                        if (jumpSymbol != symbols.end()) {
                            if (opcode == "jal")
                            {
                                ImGui::Text("%08X: %s %s %s, <%s>", currentPC, opcodeBuffer, opcode.c_str(), rd.c_str(), jumpSymbol->second.name.c_str());
                            }
                            else if (opcode == "jalr")
                            {
                                ImGui::Text("%08X: %s %s %s, %s, <%s>", currentPC, opcodeBuffer, opcode.c_str(), rd.c_str(), rs.c_str(), jumpSymbol->second.name.c_str());
                            }
                            else
                            {
                                ImGui::Text("%08X: %s %s %s, %s, <%s>", currentPC, opcodeBuffer, opcode.c_str(), rd.c_str(), rs.c_str(), jumpSymbol->second.name.c_str());
                            }

                        } else {
                            ImGui::Text("%08X: %s %s %s", currentPC, opcodeBuffer, disassembly.c_str(), (isCurrentInstruction) ? " <-" : "");
                        }
                    } else {
                        ImGui::Text("%08X: %s %s", currentPC, opcodeBuffer, disassembly.c_str());

                        auto it = symbols.find(currentPC);
                        if (it != symbols.end()) {
                            ImGui::SameLine();
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f)); // Gray text color for symbols
                            ImGui::Text("<%s>", it->second.name.c_str());
                            ImGui::PopStyleColor();
                        }
                    }

                    // Reset text color if it was changed for highlighting
                    if (isCurrentInstruction) {
                        ImGui::PopStyleColor();
                    }

					// Update the current PC for the next instruction
					currentPC += 4;
				}
			}

			ImGui::EndChild();
		}

		if (cpu_reg_debug_window)
		{
            if (core.get_xlen() == 32)
            {
                imgui_registers_window_32(&core, &cpu_reg_debug_window);
            }
            else if (core.get_xlen() == 64)
            {
                imgui_registers_window_64(&core, &cpu_reg_debug_window);
            }
		}

		if (core_config_window)
		{
			ImGui::Begin("RISC-V Core Configuration");

			ImGui::Text("Base Integer Instruction Set:");
			ImGui::RadioButton("RV32I", &selected_riscv_variant, 0);
			ImGui::SameLine();
			ImGui::RadioButton("RV32E", &selected_riscv_variant, 1);
			ImGui::SameLine();
			ImGui::RadioButton("RV64I", &selected_riscv_variant, 2);

			ImGui::Text("Extensions:");
			ImGui::Checkbox("M (Multiplication & Division)", &has_M_extension);
			ImGui::SameLine();
			ImGui::Checkbox("A (Atomic)", &has_A_extension);;
            ImGui::SameLine();
            ImGui::Checkbox("C (Compressed)", &has_C_extension);;


			ImGui::Checkbox("Zicsr", &has_Zicsr_extension);
			ImGui::SameLine();
			ImGui::Checkbox("Zifence", &has_Zifence_extension);

			ImGui::Text("MMU Emulation:");
			ImGui::Checkbox("None (nommu)", &nommu);

			if (core_.empty())
			{
				if (ImGui::Button("Build Core"))
				{
					std::vector<std::string> extensions;
					if (has_M_extension)
						extensions.push_back("M");
					if (has_A_extension)
						extensions.push_back("A");
                    if (has_C_extension)
                        extensions.push_back("C");
					if (has_Zicsr_extension)
						extensions.push_back("Zicsr");
					if (has_Zifence_extension)
						extensions.push_back("Zifence");

					switch (selected_riscv_variant)
					{
						// RV32I
						case 0:
						{
							riscv_core_32 = std::make_unique<RV32I>(extensions);
                            core.assign(riscv_core_32.get());
							core_ = "RV32I";
							xlen_ = "32";
                            built_core = true;
							break;
						}
                        // RV32E
						case 1:
						{
							riscv_core_32e = std::make_unique<RV32E>(extensions);
                            core.assign(riscv_core_32e.get());
							core_ = "RV32E";
							xlen_ = "32";
                            built_core = true;
							break;
						}
                        // RV64I
						case 2:
						{
							riscv_core_64 = std::make_unique<RV64I>(extensions);
                            core.assign(riscv_core_64.get());
							core_ = "RV64I";
							xlen_ = "64";
                            built_core = true;
							break;
						}
						default:
                            core_invalid_alert = true;
							break;
					}

                    if (built_core)
                    {
                        Logger::Instance().Log("[RISKY] Built " + core_ + " core (XLEN: " + xlen_ + ")");

                        if (extensions.size() > 0)
                        {
                            std::string extensionString;

                            for (size_t i = 0; i < extensions.size(); ++i) {
                                if (i > 0) {
                                    extensionString += ", ";
                                }
                                extensionString += extensions[i];
                            }

                            Logger::Instance().Log("[RISKY] Extensions: " + extensionString);
                        }
                        else
                        {
                            Logger::Instance().Log("[RISKY] Extensions: None (Base spec)");
                        }
                    }
				}

			}
			else
			{
				ImGui::Separator();
				ImGui::Text("There's a core in use, reset the state to create a new one!");
			}

			ImGui::End();
		}


		if (log_debug_window) {
			const auto& logMessages = ImGuiLogger::GetImGuiLogBuffer();

			ImGui::Begin("Log Messages");

			for (const auto& logEntry : logMessages) {
				const std::string &message = logEntry.first;
				LogLevel level = logEntry.second;

				if (level == LogLevel::Error) {
					ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", message.c_str());
				} else if (level == LogLevel::Warning) {
					ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", message.c_str());
				} else {
					ImGui::TextUnformatted(message.c_str());
				}
			}

			ImGui::End();
		}

        if (ImGuiFileDialog::Instance()->Display("SymbolsLoadDlg")) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
                std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();

                symbols = parse_symbols_linux_map(filePathName);
                symbols_loaded = true;
            }

            ImGuiFileDialog::Instance()->Close();
        }

		if (ImGuiFileDialog::Instance()->Display("BinaryLoadDlg")) {
			if (ImGuiFileDialog::Instance()->IsOk()) {
				std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
				std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();

				if (core_.contains("RV32I"))
				{
					riscv_core_32->bus.load_binary(filePathName);
				}
				else if (core_.contains("RV32E"))
				{
					riscv_core_32e->bus.load_binary(filePathName);
				}
				else if (core_.contains("RV64I"))
				{
					riscv_core_64->bus.load_binary(filePathName);
				}
			}

			ImGuiFileDialog::Instance()->Close();
		}

		ImGui::Render();
		glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
		glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(window);
	}
#ifdef __EMSCRIPTEN__
	EMSCRIPTEN_MAINLOOP_END;
#endif

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

void ImGui_Risky::imgui_registers_window_32(Core *core, bool *debug_window) {
    ImGui::Begin("CPU Registers", debug_window);
    ImGui::Text("General-Purpose Registers:");

    const int numColumns = 4;
    const float columnWidth = 150.0f;

    auto pc = std::any_cast<std::uint32_t>(core->pc());

    ImGui::Columns(numColumns, nullptr, false);

    for (int i = 0; i < 32; ++i)
    {
        ImGui::Text("%s:", cpu_abi_register_names[i].c_str());

        ImGui::NextColumn();

        ImGui::Text("0x%08X", core->register_32(i));

        ImGui::NextColumn();
    }

    ImGui::Columns(1);

    ImGui::Separator();

    ImGui::Text("Program Counter (PC): 0x%08X", pc);

    ImGui::End();
}

void ImGui_Risky::imgui_registers_window_64(Core *core, bool *debug_window) {
    ImGui::Begin("CPU Registers", debug_window);
    ImGui::Text("General-Purpose Registers:");

    const int numColumns = 4;
    const float columnWidth = 150.0f;

    auto pc = std::any_cast<std::uint64_t>(core->pc());

    ImGui::Columns(numColumns, nullptr, false);

    for (int i = 0; i < 32; ++i)
    {
        ImGui::Text("%s:", cpu_abi_register_names[i].c_str());

        ImGui::NextColumn();

        ImGui::Text("0x%016lX", core->register_64(i));

        ImGui::NextColumn();
    }

    ImGui::Columns(1);

    ImGui::Separator();

    ImGui::Text("Program Counter (PC): 0x%016lX", pc);

    ImGui::End();
}
