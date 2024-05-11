#include <risky.h>
#include <cstdlib>

Exit* Risky::exitSystem = nullptr;

void Risky::setExitSystem(Exit* exitSystem_) {
	Risky::exitSystem = exitSystem_;
}

bool Risky::shouldExit() {
	if (exitSystem) {
		return exitSystem->shouldExit();
	}

	return false;
}

void Risky::exit() {
	if (exitSystem) {
		exitSystem->exit();
	} else {
		std::exit(0);
	}
}
