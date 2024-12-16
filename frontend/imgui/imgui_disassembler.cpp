#include <frontends/imgui/imgui_risky.h>
#include <cpu/registers.h>
#include <cpu/disassembler.h>

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

void ImGui_Risky::imgui_disassembly_window_32(Core *core) {
	Disassembler disassembler;

	static int selectedRegisterNames = 1;
	const char* registerNameSets[] = { "RISC Naming", "ABI Naming" };

	ImGui::Text("PC:");

	ImGui::SameLine();

	static char jumpToAddressBuffer[9] = "00000000";
	ImGui::InputText("->", jumpToAddressBuffer,
	                 sizeof(jumpToAddressBuffer),
	                 ImGuiInputTextFlags_CharsHexadecimal);

	std::uint32_t jumpToAddress =
			std::strtoul(jumpToAddressBuffer, nullptr, 16);

	ImGui::SameLine();

	static std::uint32_t startPC;

	if (ImGui::Button("Jump")) {
		core->set_pc_32(jumpToAddress);
		startPC = std::any_cast<std::uint32_t>(core->pc());
	}

	ImGui::Separator();

	if (Risky::is_aborted()) {
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
		ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 1.0f);
		ImGui::ArrowButton("##PlayButton", ImGuiDir_Right);
		ImGui::PopStyleVar(2);
	} else {
		// Check if the core stepping thread is running
		if (core->thread_running()) {
			if (ImGui::Button("Stop")) {
				// Stop the step thread
				core->stop_();
			}
		} else {
			if (ImGui::ArrowButton("##PlayButton", ImGuiDir_Right)) {
				// Start the step thread
				core->start_();
			}
		}
	}

	// Check if the core stepping thread is running
	if (core->check_thread()) {
		startPC = std::any_cast<std::uint32_t>(core->pc());
		ImGui::SetScrollY(0);
	}

	ImGui::SameLine();

	if (Risky::is_aborted()) {
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha,
		                    ImGui::GetStyle().Alpha * 0.5f);
		ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 1.0f);
		ImGui::Button("Step");
		ImGui::PopStyleVar(2);
	}
	else
	{
		if (ImGui::Button("Step")) {
			core->step();
			startPC = std::any_cast<std::uint32_t>(core->pc()) - 4;
		}
	}

	if (Risky::is_aborted()) {
		ImGui::SameLine();
		if (ImGui::Button("Reset")) {
			core->reset();
			Risky::reset_aborted();
		}
	}

	ImGui::SameLine();

	ImGui::Combo("Register Names", &selectedRegisterNames, registerNameSets, IM_ARRAYSIZE(registerNameSets));

	ImGui::BeginChild("Disassembly", ImVec2(0, 0), true);

	int numInstructions = ImGui::GetWindowHeight() / ImGui::GetTextLineHeight();
	static bool initialized = false;
	static float lastScrollY = 0.0f;

	// Initialize startPC with the current PC
	std::uint32_t currentPC = std::any_cast<std::uint32_t>(core->pc());
	if (!initialized) {
		startPC = currentPC;
		initialized = true;
	}

	// Adjust the startPC if scrolling past the current list
	float scrollY = ImGui::GetScrollY();
	float scrollMaxY = ImGui::GetScrollMaxY();

	// Use a threshold to adjust less aggressively
	const float scrollThreshold = 5.0f;
	const int scrollIncrement = 4 * 4;  // Adjust by 4 instructions (4 bytes each)

	if (scrollY >= (scrollMaxY - scrollThreshold) && scrollY > lastScrollY) {
		// Scrolling down past the bottom
		startPC += scrollIncrement;
		ImGui::SetScrollY(scrollMaxY - scrollThreshold - 1);  // Reset scroll to avoid continuous adjustment
	} else if (scrollY <= scrollThreshold && scrollY < lastScrollY) {
		// Scrolling up past the top
		startPC = (startPC > scrollIncrement) ? (startPC - scrollIncrement) : 0;
		ImGui::SetScrollY(scrollThreshold + 1);  // Reset scroll to avoid continuous adjustment
	}

	lastScrollY = scrollY;


	std::vector<DisplayItem> displayItems;

	for (int i = 0; i < numInstructions; ++i) {
		currentPC = startPC + i * 4;

		auto it = symbols.find(currentPC);
		if (it != symbols.end()) {
			displayItems.push_back({ format("{:08X}  <{}>:", currentPC, it->second.name), "", false, false, true, ImVec4(0.5f, 0.5f, 0.5f, 1.0f) });
		}

		// Disassemble the instruction at the current PC
		std::uint32_t opcode = core->bus_read32(currentPC);
		const std::vector<std::string> *registerNames = (selectedRegisterNames == 0) ? &cpu_register_names : &cpu_abi_register_names;
		std::string disassembly = disassembler.Disassemble(opcode, registerNames);

		char opcodeBuffer[12];
		snprintf(opcodeBuffer, sizeof(opcodeBuffer), "%02X %02X %02X %02X",
		         (opcode >> 24) & 0xFF,
		         (opcode >> 16) & 0xFF,
		         (opcode >> 8) & 0xFF,
		         (opcode >> 0) & 0xFF);

		bool isJumpInstruction = (disassembly.find("jal") != std::string::npos || disassembly.find("jalr") != std::string::npos);
		bool isCurrentInstruction = (currentPC == std::any_cast<std::uint32_t>(core->pc()));

		ImVec4 instructionColor = isCurrentInstruction ? ImVec4(1.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

		std::string displayText;

		if (isJumpInstruction) {
			std::istringstream iss(disassembly);
			std::string opcode_dis, rd, rs, immediate;
			iss >> opcode_dis;

			if (opcode_dis == "jal") {
				iss >> rd >> immediate;
				rd.erase(std::remove(rd.begin(), rd.end(), ','), rd.end());
			} else if (opcode_dis == "jalr") {
				iss >> rd >> rs >> immediate;
				rd.erase(std::remove(rd.begin(), rd.end(), ','), rd.end());
				rs.erase(std::remove(rs.begin(), rs.end(), ','), rs.end());
			}

			std::uint32_t jumpOffset;
			try {
				jumpOffset = std::stoul(immediate);
			} catch (const std::invalid_argument& e) {
				Logger::error("Invalid jump offset!");
				Risky::exit(1, Risky::Subsystem::Core);
			}

			std::uint32_t jumpAddress = currentPC + jumpOffset;

			auto jumpSymbol = symbols.find(jumpAddress);
			if (symbols_loaded && jumpSymbol != symbols.end()) {
				if (opcode_dis == "jal") {
					displayText = format("{} {}, <{}>", opcode_dis, rd, jumpSymbol->second.name);
				} else if (opcode_dis == "jalr") {
					displayText = format("{} {}, {}, <{}>", opcode_dis, rd, rs, jumpSymbol->second.name);
				}
			} else {
				displayText = format("{}", disassembly);
			}
		} else {
			displayText = disassembly;
		}

		displayItems.push_back({ format("{:08X}: {}", currentPC, displayText), opcodeBuffer, isCurrentInstruction, isJumpInstruction, it != symbols.end(), instructionColor });
	}

	ImGuiListClipper clipper;
	clipper.Begin(displayItems.size());

	while (clipper.Step()) {
		for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
			const DisplayItem& item = displayItems[i];

			if (item.isCurrentInstruction) {
				ImGui::PushStyleColor(ImGuiCol_Text, item.color);
			}

			ImGui::Text("%s", item.text.c_str());

			ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(item.opcodeBuffer.c_str()).x);

			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
			ImGui::Text("%s", item.opcodeBuffer.c_str());
			ImGui::PopStyleColor();

			if (item.isCurrentInstruction) {
				ImGui::PopStyleColor();
			}
		}
	}

	ImGui::EndChild();
}
