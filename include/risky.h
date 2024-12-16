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

#define RESET "\033[0m"
#define BLACK "\033[30m"              /* Black */
#define RED "\033[31m"                /* Red */
#define GREEN "\033[32m"              /* Green */
#define YELLOW "\033[33m"             /* Yellow */
#define BLUE "\033[34m"               /* Blue */
#define MAGENTA "\033[35m"            /* Magenta */
#define CYAN "\033[36m"               /* Cyan */
#define WHITE "\033[37m"              /* White */
#define BOLDBLACK "\033[1m\033[30m"   /* Bold Black */
#define BOLDRED "\033[1m\033[31m"     /* Bold Red */
#define BOLDGREEN "\033[1m\033[32m"   /* Bold Green */
#define BOLDYELLOW "\033[1m\033[33m"  /* Bold Yellow */
#define BOLDBLUE "\033[1m\033[34m"    /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m" /* Bold Magenta */
#define BOLDCYAN "\033[1m\033[36m"    /* Bold Cyan */
#define BOLDWHITE "\033[1m\033[37m"   /* Bold White */
