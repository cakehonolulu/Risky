#include <risky.h>
#include <cstdlib>

Exit* Risky::exitSystem = nullptr;
bool Risky::requested_exit = false;
bool Risky::abort = false;

void Risky::setExitSystem(Exit* exit_) {
    Risky::exitSystem = exit_;
}

bool Risky::requestedExit() {
    return requested_exit;
}

bool Risky::aborted() {
    return abort;
}

void Risky::exit() {
    if (exitSystem) {
        exitSystem->exit();
    }
}