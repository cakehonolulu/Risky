#pragma once

#include <cstdint>

class CoreBackend {
public:
    virtual ~CoreBackend() = default;
    virtual void execute_opcode(std::uint32_t opcode) = 0;

    bool ready = false;
};
