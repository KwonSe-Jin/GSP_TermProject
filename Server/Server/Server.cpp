#include "pch.h"
#include "GameServer.h"
#include "LuaManager.h"
int main()
{
   // LuaManager::GetInstance(); // LuaManager �ʱ�ȭ
    GameServer server;
    server.Run();
}
