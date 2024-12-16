#include <fstream>
#include <sstream>
#include <log/log.hh>
#include <utils/symbols.h>
#include "risky.h"

std::unordered_map<std::uint32_t, Symbol> parse_symbols_map(const std::string& filename) {
    std::unordered_map<std::uint32_t, Symbol> symbols;
    std::ifstream file(filename);

    if (!file.is_open()) {
        Logger::error("parse_symbols_map: Failed to open " + filename);
        Risky::exit(1, Risky::Subsystem::Core);
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::uint32_t address;
        std::string type;
        std::string name;

        if (!(iss >> std::hex >> address >> type >> name)) {
            continue;
        }

        symbols[address] = { address, type, name };
    }

    Logger::info("parse_symbols_map: Symbols parsed");
    return symbols;
}
