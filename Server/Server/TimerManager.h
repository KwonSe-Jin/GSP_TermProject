#pragma once
#include "pch.h"

class TimerManager {
private:
    TimerManager() {}

public:
    static TimerManager* GetInstance() {
        static TimerManager instance;
        return &instance;
    }

    void ProcessTimer();
};
