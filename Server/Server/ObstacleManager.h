#pragma once
#include "pch.h"

class ObstacleManager {
private:
    ObstacleManager() {} // �̱��� ���� ����

public:
    static ObstacleManager* GetInstance() {
        static ObstacleManager instance;
        return &instance;
    }

    void InitializeObstacle();
};
