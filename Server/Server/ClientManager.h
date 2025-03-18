#pragma once
#include "pch.h"
#include "LuaManager.h"

class ClientManager {
private:
    ClientManager() {}

public:
    static ClientManager* GetInstance() {
        static ClientManager instance;
        return &instance;
    }

    void ProcessIO(int client_id, OVER_EXP* ex_over, DWORD num_bytes);
    void Disconnect(int client_id);
    void Recv(int client_id, OVER_EXP* exp_over, DWORD num_bytes);
    void Accept(OVER_EXP* exp_over);
    // 패킷 처리 함수 추가
    void ProcessPacket(int client_id, char* packet);

    void HandleLoginSuccess(int client_id);
};