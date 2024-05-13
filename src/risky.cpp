#include <risky.h>
#include <cstdlib>
#include <cstdint>

Exit* Risky::exitSystem = nullptr;
bool Risky::requested_exit = false;
bool Risky::abort = false;

void Risky::setExitSystem(Exit* exitSystem) {
    Risky::exitSystem = exitSystem;
}

bool Risky::requestedExit() {
    return requested_exit;
}

bool Risky::aborted() {
    return abort;
}

void Risky::exit() {
    if (exitSystem) {
        exitSystem->exitApplication();
    }
}
