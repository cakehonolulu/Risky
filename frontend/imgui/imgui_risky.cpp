#include <frontend/imgui/imgui_risky.h>

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include "cpu/core/rv32i.h"
#include "ImGuiFileDialog.h"
#include "cpu/core/rv64i.h"
#include "cpu/core/rv32e.h"
#include <cstdio>
#include <SDL3/SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL3/SDL_opengles2.h>
#else
#include <SDL3/SDL_opengl.h>
#include <vector>

#include <frontend/imgui/imgui_log.h>

#endif

// This example doesn't compile with Emscripten yet! Awaiting SDL3 support.
#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

void ImGui_Risky::init() {

}

void ImGui_Risky::run() {
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

	bool log_debug_window = false;
	IGFD::FileDialogConfig config; config.path = ".";

	// 0: RV32I, 1: RV32E, 2: RV64I
	int riscv_variant = 3;
	bool has_M_extension = false;
	bool has_A_extension = false;
	bool nommu = true;

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

		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Open Binary", ""))
				{
					ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".*,.cpp,.h,.hpp", config);
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Debug"))
			{
				static bool menu_toggle_cpu_reg_window = false;
				static bool menu_toggle_log_window = false;
				static bool menu_toggle_disassembler_window = false;

				ImGui::MenuItem("CPU Registers", "", &menu_toggle_cpu_reg_window, true);
				ImGui::MenuItem("Log", "", &menu_toggle_log_window, true);
				ImGui::MenuItem("Disassembler", "", &menu_toggle_disassembler_window, true);

				if (menu_toggle_log_window)
				{
					log_debug_window = true;
				}
				else
				{
					log_debug_window = false;
				}

				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

		/*std::vector<std::string> extensions = {"M"};

		RV32I riscv(extensions);

		riscv.bus.load_binary("../DownloadedImage");

		riscv.run();*/

		{
			static float f = 0.0f;
			static int counter = 0;

			ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

			ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)

			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
			ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

			if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
				counter++;
			ImGui::SameLine();
			ImGui::Text("counter = %d", counter);

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
			ImGui::End();
		}

		{
			ImGui::Begin("RISC-V Configuration");

			ImGui::Text("Base Integer Instruction Set:");
			ImGui::RadioButton("RV32I", &riscv_variant, 0);
			ImGui::SameLine();
			ImGui::RadioButton("RV32E", &riscv_variant, 1);
			ImGui::SameLine();
			ImGui::RadioButton("RV64I", &riscv_variant, 2);

			ImGui::Text("Extensions:");
			ImGui::Checkbox("M (Multiplication & Division)", &has_M_extension);
			ImGui::SameLine();
			ImGui::Checkbox("A (Atomic)", &has_A_extension);

			ImGui::Text("MMU Emulation:");
			ImGui::Checkbox("None (nommu)", &nommu);

			if (ImGui::Button("Build Core"))
			{
				std::vector<std::string> extensions;
				if (has_M_extension)
					extensions.push_back("M");
				if (has_A_extension)
					extensions.push_back("A");

				switch (riscv_variant)
				{
					// RV32I
					case 0:
					{
						RV32I riscv(extensions);
						Logger::Instance().Log("[RISCV] RV32I core built");
						break;
					}
					// RV32E
					case 1:
					{
						RV32E riscv(extensions);
						Logger::Instance().Log("[RISCV] RV32E core built");
						break;
					}
					// RV64I
					case 2:
					{
						RV64I riscv(extensions);
						Logger::Instance().Log("[RISCV] RV64I core built");
						break;
					}
					default:
						break;
				}
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

		if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey")) {
			if (ImGuiFileDialog::Instance()->IsOk()) { // action if OK
				std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
				std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
				//bus->load_(filePathName);
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