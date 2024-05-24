#include <fstream>
#include <sstream>
#include <log/log.h>
#include <utils/symbols.h>

std::unordered_map<std::uint32_t, Symbol> parse_symbols_map(const std::string& filename) {
    std::unordered_map<std::uint32_t, Symbol> symbols;
    std::ifstream file(filename);

    if (!file.is_open()) {
        Logger::Instance().Error("[SYMBOLS] parse_symbols_map: Failed to open " + filename);
        Risky::exit();
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

    Logger::Instance().Log("[SYMBOLS] parse_symbols_map: Symbols parsed");
    return symbols;
}
