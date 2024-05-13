#pragma once


class Exit {
public:
    virtual ~Exit() = default;
    virtual void exitApplication() = 0;
};
