#pragma once

class Exit {
public:
	virtual bool shouldExit() const = 0;
	virtual void exit() const = 0;
};
