#pragma once

#include <iostream>
#include <vector>
#include <functional>
#include <cstdint>
#include <risky.h>

enum class LogLevel
{
	Info,
	Warning,
	Error,
	Debug,
    Uart
};

class Logger
{
public:
	static Logger& Instance()
	{
		static Logger instance;
		return instance;
	}

	void SetLogFunction(std::function<void(const std::string&)> customLogFunction)
	{
		log = std::move(customLogFunction);
	}

	void SetErrorFunction(std::function<void(const std::string&)> customErrorFunction)
	{
		error = std::move(customErrorFunction);
	}

	void SetWarnFunction(std::function<void(const std::string&)> customWarnFunction)
	{
		warn = std::move(customWarnFunction);
	}

    void SetUARTFunction(std::function<void(std::uint8_t)> customUARTFunction)
    {
        uart = std::move(customUARTFunction);
    }

	void Log(const std::string& message)
	{
		if (log)
		{
			log(message);
		}
		else
		{
			std::cout << message << std::endl;
		}
	}

	void Error(const std::string& message)
	{
		if (error)
		{
			error(message);
		}
		else
		{
			std::cout << BOLDRED <<  message << RESET << std::endl;
		}
	}

	void Warn(const std::string& message)
	{
		if (warn)
		{
			warn(message);
		}
		else
		{
			std::cout << BOLDYELLOW <<  message << RESET << std::endl;
		}
	}

    void Uart(std::uint8_t byte)
    {
        if (uart)
        {
            uart(byte);
        }
        else
        {
            uartBuffer.push_back(byte);
        }
    }

private:
	std::function<void(const std::string&)> log;
	std::function<void(const std::string&)> error;
	std::function<void(const std::string&)> warn;
    std::function<void(std::uint8_t)> uart;
    std::vector<std::uint8_t> uartBuffer;
};
