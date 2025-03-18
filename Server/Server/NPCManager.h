#pragma once
#include "pch.h"

class NPCManager
{
private:
    NPCManager() {}

public:
    static NPCManager* GetInstance() {
        static NPCManager instance;
        return &instance;
    }

    void InitializeNPC();
    void DoNPCMove(int npc_id, int player_id);
    void DoNPCAttack(int npc_id, int user_id);
    void WakeUpNPC(int npc_id, int waker);
};

