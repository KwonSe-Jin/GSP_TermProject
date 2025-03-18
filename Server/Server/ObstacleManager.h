#pragma once
#include "pch.h"

class ObstacleManager {
private:
    ObstacleManager() {} // ½Ì±ÛÅæ ÆĞÅÏ Àû¿ë

public:
    static ObstacleManager* GetInstance() {
        static ObstacleManager instance;
        return &instance;
    }

    void InitializeObstacle();
};
