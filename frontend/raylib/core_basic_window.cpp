#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

int main()
{
	int screenWidth = 1280;
	int screenHeight = 720;
	bool coreEmptyAlert = false;
	bool coreInvalidAlert = false;
	bool logDebugWindow = true;
	bool coreConfigWindow = true;
	bool cpuRegDebugWindow = false;
	bool disassemblyWindow = false;
	bool builtCore = false;
	bool symbolsLoaded = false;
	bool requestedExit = false;

	bool hasMExtension = false;
	bool hasAExtension = false;
	bool hasCExtension = false;
	bool hasZicsrExtension = false;
	bool hasZifenceExtension = false;
	bool nommu = false;
	int selectedRiscvVariant = 0;

	const char *riscvVariantsText = "RV32I;RV32E;RV64I";

	InitWindow(screenWidth, screenHeight, "Risky - RISC-V Emulator");

#if defined(PLATFORM_WEB)
	emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
	SetTargetFPS(60);
	while (!WindowShouldClose())
	{
		if (requestedExit)
		{
			// Handle exit logic
			// For now, just reset the flag
			requestedExit = false;
		}

		BeginDrawing();
		ClearBackground(RAYWHITE);

		// Draw main menu
		if (GuiButton((Rectangle){ 10, 10, 100, 30 }, "Config"))
		{
			coreConfigWindow = !coreConfigWindow;
		}

		if (GuiButton((Rectangle){ 120, 10, 100, 30 }, "Log"))
		{
			logDebugWindow = !logDebugWindow;
		}

		if (GuiButton((Rectangle){ 230, 10, 150, 30 }, "CPU Registers") && builtCore)
		{
			cpuRegDebugWindow = !cpuRegDebugWindow;
		}

		if (GuiButton((Rectangle){ 390, 10, 150, 30 }, "Disassembler") && builtCore)
		{
			disassemblyWindow = !disassemblyWindow;
		}

		if (coreConfigWindow)
		{
			GuiToggleGroup((Rectangle){ 10, 110, 160, 20 }, riscvVariantsText, &selectedRiscvVariant);

			GuiCheckBox((Rectangle){ 10, 180, 20, 20 }, "M (Multiplication & Division)", &hasMExtension);
			GuiCheckBox((Rectangle){ 10, 210, 20, 20 }, "A (Atomic)", &hasAExtension);
			GuiCheckBox((Rectangle){ 10, 240, 20, 20 }, "C (Compressed)", &hasCExtension);
			GuiCheckBox((Rectangle){ 10, 270, 20, 20 }, "Zicsr", &hasZicsrExtension);
			GuiCheckBox((Rectangle){ 10, 300, 20, 20 }, "Zifence", &hasZifenceExtension);
			GuiCheckBox((Rectangle){ 10, 370, 20, 20 }, "None (nommu)", &nommu);

			if (GuiButton((Rectangle){ 10, 410, 100, 30 }, "Build Core")) {
				builtCore = true;
			}

			if (builtCore)
			{
				DrawText("Core built successfully!", 10, 450, 20, GREEN);
			}
		}

		if (logDebugWindow)
		{
			// Draw log debug window
			DrawText("Log Messages:", 500, 50, 20, BLACK);
		}

		if (cpuRegDebugWindow)
		{
			// Draw CPU register debug window
			DrawText("CPU Registers Debug Window", 500, 150, 20, BLACK);
		}

		if (disassemblyWindow)
		{
			// Draw disassembly window
			DrawText("Disassembly Window", 500, 250, 20, BLACK);
		}

		EndDrawing();
	}
#endif

	CloseWindow();
	return 0;
}
