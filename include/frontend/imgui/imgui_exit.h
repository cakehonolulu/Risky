#pragma once

#include <utils/exit.h>

class ImGuiExitSystem : public Exit {
public:
    void exitApplication() override;
};
