#pragma once

#include <utils/exit.h>

class Risky {
public:
	static void setExitSystem(Exit* exit_);
	static bool shouldExit();
	static void exit();

	virtual void init() = 0;
	virtual void run() = 0;
private:
	static Exit* exitSystem;
};
