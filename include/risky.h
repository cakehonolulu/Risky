#pragma once

#include <log/log.hh>

class Risky {
public:
    enum class Subsystem {
        None,
        Core,
        Bus,
        Disassembler
    };

    Risky(std::shared_ptr<LogBackend> logger = nullptr);
    virtual ~Risky();

    virtual void init() = 0;
    virtual void run() = 0;

    static int exit(int code, Subsystem subsystem);
    static bool is_aborted() { return aborted; }
    static void reset_aborted() {
        aborted = false;
        guilty_subsystem = Subsystem::None;
    }

    static Subsystem get_guilty_subsystem() { return guilty_subsystem; }

private:
    static inline bool aborted = false;
    static inline Subsystem guilty_subsystem = Subsystem::None;
};

#define _RESET "\033[0m"
#define _BLACK "\033[30m"              /* Black */
#define _RED "\033[31m"                /* Red */
#define _GREEN "\033[32m"              /* Green */
#define _YELLOW "\033[33m"             /* Yellow */
#define _BLUE "\033[34m"               /* Blue */
#define _MAGENTA "\033[35m"            /* Magenta */
#define _CYAN "\033[36m"               /* Cyan */
#define _WHITE "\033[37m"              /* White */
#define _BOLDBLACK "\033[1m\033[30m"   /* Bold Black */
#define _BOLDRED "\033[1m\033[31m"     /* Bold Red */
#define _BOLDGREEN "\033[1m\033[32m"   /* Bold Green */
#define _BOLDYELLOW "\033[1m\033[33m"  /* Bold Yellow */
#define _BOLDBLUE "\033[1m\033[34m"    /* Bold Blue */
#define _BOLDMAGENTA "\033[1m\033[35m" /* Bold Magenta */
#define _BOLDCYAN "\033[1m\033[36m"    /* Bold Cyan */
#define _BOLDWHITE "\033[1m\033[37m"   /* Bold White */
