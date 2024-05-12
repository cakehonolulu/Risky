target_sources(risky
                PRIVATE
                risky.cpp
                bus/bus.cpp
                cpu/core/rv32e.cpp
                cpu/core/rv32i.cpp
                cpu/core/rv64i.cpp
                cpu/disassembler.cpp)
