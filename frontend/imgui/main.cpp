#include <frontends/imgui/imgui_risky.h>

int main(int argc, char **argv)
{
	ImGui_Risky risky;
	risky.init();
	risky.run();
	return 0;
}