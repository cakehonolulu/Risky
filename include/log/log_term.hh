#pragma once
#include <log/log.hh>
#include <iostream>

class TerminalLogBackend : public LogBackend
{
  public:
    void log(const std::string &message, LogLevel level) override
    {
        std::string color;

        switch (level)
        {
        case LogLevel::Info:
            color = _BOLDWHITE;
            break;
        case LogLevel::Warn:
            color = _BOLDYELLOW;
            break;
        case LogLevel::Debug:
            color = _BOLDCYAN;
            break;
        case LogLevel::Error:
            color = _BOLDRED;
            break;
        default:
            color = _WHITE;
        }

        if (level != LogLevel::Raw)
		{
			std::cout << color << message << _RESET << std::endl;
		}
		else
		{
			std::cout << message;
		}
    }
};
