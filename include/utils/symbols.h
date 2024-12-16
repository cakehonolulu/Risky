#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include "risky.h"

struct Symbol {
    std::uint32_t address;
    std::string type;
    std::string name;
};

std::unordered_map<std::uint32_t, Symbol> parse_symbols_map(const std::string& filename);
