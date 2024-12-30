#include <frontends/imgui/imgui_risky.h>

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include "ImGuiFileDialog.h"
#include <cpu/core/rv32/rv32e.h>
#include <cpu/core/rv32/rv32i.h>
#include <cpu/core/rv64/rv64i.h>
#include <cpu/core/core.h>
#include <cstdio>
#include <SDL3/SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL3/SDL_opengles2.h>
#else
#include <SDL3/SDL_opengl.h>
#include <vector>

#include <log/log.hh>
#include <cpu/registers.h>
#include <cpu/disassembler.h>
#include <log/log_term.hh>
#include <frontends/imgui/imgui_risky.h>

#endif

// This example doesn't compile with Emscripten yet! Awaiting SDL3 support.
#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

ImGui_Risky::ImGui_Risky() 
	: Risky(std::make_shared<ImGuiLogBackend>())
{
	imgui_logger = std::dynamic_pointer_cast<ImGuiLogBackend>(Logger::get_backends().back());
	Logger::set_subsystem("RISKY");
    Logger::info("Logger initialized\n");
}


void ImGui_Risky::init() {
}

void ImGui_Risky::run() {
	// Setup SDL
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
	{
		printf("Error: SDL_Init(): %s\n", SDL_GetError());
		return;
	}

	// Create window with SDL_Renderer graphics context
	Uint32 window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN;
	SDL_Window* window = SDL_CreateWindow("Risky - ImGui + SDL3", 1280, 720, window_flags);
	if (window == nullptr)
	{
		printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
		return;
	}
	SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
	SDL_SetRenderVSync(renderer, 1);
	if (renderer == nullptr)
	{
		SDL_Log("Error: SDL_CreateRenderer(): %s\n", SDL_GetError());
		return;
	}
	SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	SDL_ShowWindow(window);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends
	ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
	ImGui_ImplSDLRenderer3_Init(renderer);

	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	bool done = false;
    static bool suppress_exit_notification = false;

	bool log_debug_window = true;
	bool core_config_window = true;
	bool cpu_reg_debug_window = false;
	bool disassembly_window = false;
	IGFD::FileDialogConfig config; config.path = ".";

	// 0: RV32I, 1: RV32E, 2: RV64I
	int selected_riscv_variant = 0;
	std::string core_;
	std::string xlen_;
	bool has_M_extension = false;
	bool has_A_extension = false;
    bool has_C_extension = false;
	bool has_Zicsr_extension = false;
	bool has_Zifencei_extension = false;
	bool nommu = true;
    bool built_core = false;
    bool core_empty_alert = false;
    bool core_invalid_alert = false;
    bool loaded_binary = false;
    symbols_loaded = false;
    Core core;
    EmulationType selected_emulation_type = EmulationType::Interpreter;

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

		ImGui_ImplSDLRenderer3_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::DockSpaceOverViewport(0, viewport);

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
	                if (ImGui::MenuItem("Load ELF")) {
		                ImGuiFileDialog::Instance()->OpenDialog("ElfLoadDlg", "Choose File", ".elf", config);
	                }

	                if (ImGui::MenuItem("Load Binary", ""))
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

        if (Risky::is_aborted() && !suppress_exit_notification) {
            // Set window size and open popup
            ImGui::SetNextWindowSize(ImVec2(350, 150), ImGuiCond_Appearing);
            ImGui::OpenPopup("Exit Notification");

            bool open = true;
            if (ImGui::Begin("Exit Notification", &open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse)) {
                ImGui::Text("The emulator encountered a critical error.");

                // Reset button to clear the aborted state and resume
                if (ImGui::Button("Reset")) {
                    Risky::reset_aborted();
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();

                // Close button to exit the emulator
                if (ImGui::Button("Exit emulator")) {
                    done = true;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::End();
            }

            // If "X" button or other close action is taken, suppress the popup for now
            if (!open) {
                suppress_exit_notification = true;
            }
        }

        // Reset suppress_exit_notification if aborted state is cleared
        if (!Risky::is_aborted()) {
            suppress_exit_notification = false;
        }

		if (disassembly_window)
		{
			if (core.get_xlen() == 32)
            {
                imgui_disassembly_window_32(&core);
            }
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

        /*if (true)
        {
            ImGui::Begin("UART Console");

            const std::vector<uint8_t>& uartData = ImGuiLogger::GetImGuiUARTBuffer();

            std::string currentLine; // Store characters until a newline is encountered
            for (const auto& byte : uartData) {
                if (byte == '\n') {
                    // Output the current line
                    ImGui::Text("%s", currentLine.c_str());
                    // Clear the line for the next iteration
                    currentLine.clear();
                } else {
                    // Add the character to the current line
                    currentLine += byte;
                }
            }

            // Output any remaining characters if the last line doesn't end with a newline
            if (!currentLine.empty()) {
                ImGui::Text("%s", currentLine.c_str());
            }

            ImGui::End();
        }*/

		if (core_config_window)
		{
			ImGui::Begin("RISC-V Core Configuration");

			static const char* presetNames[] = { "None", "rv32imac_zifencei_zicsr", "Custom" };
			static int selectedPreset = 0;

			ImGui::Text("Presets:");
			ImGui::Combo("##PresetsCombo", &selectedPreset, presetNames, IM_ARRAYSIZE(presetNames));

			if (selectedPreset != 0) {
				has_M_extension = false;
				has_A_extension = false;
				has_C_extension = false;
				has_Zicsr_extension = false;
				has_Zifencei_extension = false;
				nommu = true;

				switch (selectedPreset) {
					case 1:  // RV32I+M+A+C+Zifence+Zicsr
						selected_riscv_variant = 0;
						has_M_extension = true;
                        has_A_extension = true;
                        has_C_extension = true;
						has_Zifencei_extension = true;
						has_Zicsr_extension = true;
						break;
					case 2:
						break;
					default:
						break;
				}
			}

			ImGui::Separator();

			ImGui::Text("Base Integer Instruction Set:");
			ImGui::RadioButton("RV32I", &selected_riscv_variant, 0);
			ImGui::SameLine();
			ImGui::RadioButton("RV32E", &selected_riscv_variant, 1);
			ImGui::SameLine();
			ImGui::RadioButton("RV64I", &selected_riscv_variant, 2);

			ImGui::Text("Extensions:");
			ImGui::Checkbox("M (Multiplication & Division)", &has_M_extension);
			ImGui::SameLine();
			ImGui::Checkbox("A (Atomic)", &has_A_extension);
            ImGui::SameLine();
            ImGui::Checkbox("C (Compressed)", &has_C_extension);


			ImGui::Checkbox("Zicsr", &has_Zicsr_extension);
			ImGui::SameLine();
			ImGui::Checkbox("Zifencei", &has_Zifencei_extension);

			ImGui::Text("MMU Emulation:");
			ImGui::Checkbox("None (nommu)", &nommu);

            ImGui::Text("Emulation Type:");
            ImGui::RadioButton("Interpreter", reinterpret_cast<int*>(&selected_emulation_type), static_cast<int>(EmulationType::Interpreter));
            ImGui::SameLine();
            ImGui::RadioButton("Recompiler (LLVM)", reinterpret_cast<int*>(&selected_emulation_type), static_cast<int>(EmulationType::JIT));

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
					if (has_Zifencei_extension)
						extensions.push_back("Zifencei");

					switch (selected_riscv_variant)
					{
						// RV32I
						case 0:
						{
							riscv_core_32 = std::make_unique<RV32I>(extensions, selected_emulation_type);
                            core.assign(riscv_core_32.get(), selected_emulation_type);
							core_ = "RV32I";
							xlen_ = "32";
                            built_core = true;
							break;
						}
                        // RV32E
						case 1:
						{
							riscv_core_32e = std::make_unique<RV32E>(extensions, selected_emulation_type);
                            core.assign(riscv_core_32e.get(), selected_emulation_type);
							core_ = "RV32E";
							xlen_ = "32";
                            built_core = true;
							break;
						}
                        // RV64I
						case 2:
						{
							riscv_core_64 = std::make_unique<RV64I>(extensions, selected_emulation_type);
                            core.assign(riscv_core_64.get(), selected_emulation_type);
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
                        Logger::info("Built " + core_ + " core (XLEN: " + xlen_ + ")");

                        if (extensions.size() > 0)
                        {
                            std::string extensionString;

                            for (size_t i = 0; i < extensions.size(); ++i) {
                                if (i > 0) {
                                    extensionString += ", ";
                                }
                                extensionString += extensions[i];
                            }

                            Logger::info("Extensions: " + extensionString);
                        }
                        else
                        {
                            Logger::info("Extensions: None (Base spec)");
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

		imgui_logger->render();

        if (ImGuiFileDialog::Instance()->Display("SymbolsLoadDlg")) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
                std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();

                symbols = parse_symbols_map(filePathName);
                symbols_loaded = true;
            }

            ImGuiFileDialog::Instance()->Close();
        }

        if (ImGuiFileDialog::Instance()->Display("ElfLoadDlg")) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
                std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();

                core.load_elf<std::uint32_t>(filePathName);
            }

            ImGuiFileDialog::Instance()->Close();
        }

		if (ImGuiFileDialog::Instance()->Display("BinaryLoadDlg")) {
			if (ImGuiFileDialog::Instance()->IsOk()) {
				std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
				std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();

                core.load_binary<std::uint32_t>(filePathName);
			}

			ImGuiFileDialog::Instance()->Close();
		}

		ImGui::Render();
		//SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
		SDL_SetRenderDrawColorFloat(renderer, clear_color.x, clear_color.y, clear_color.z, clear_color.w);
		SDL_RenderClear(renderer);
		ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
		SDL_RenderPresent(renderer);
	}
#ifdef __EMSCRIPTEN__
	EMSCRIPTEN_MAINLOOP_END;
#endif

	// Cleanup
	ImGui_ImplSDLRenderer3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}
