#include "pch.h"
#include "GameServer.h"
#include "LuaManager.h"
int main()
{
   // LuaManager::GetInstance(); // LuaManager √ ±‚»≠
    GameServer server;
    server.Run();
}
