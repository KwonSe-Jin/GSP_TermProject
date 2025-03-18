#pragma once
#include "pch.h"

// 플레이어가 NPC를 공격할 때 호출되는 함수
extern int API_attack_npc(lua_State* L);

// 객체 타입 가져오기 (플레이어 or NPC)
extern int API_get_type(lua_State* L);

// NPC가 범위 안에 있는지 확인
extern int API_LV2_range(lua_State* L);

// NPC가 일정 시간 동안 플레이어를 따라 움직이도록 설정
extern int API_move_npc(lua_State * L);

// 메시지를 클라이언트에 전송
extern int API_SendMessage(lua_State* L);

//class LuaManager {
//private:
//    lua_State* L;
//
//    LuaManager() {
//        L = luaL_newstate();
//        luaL_openlibs(L);
//        if (luaL_loadfile(L, "npc.lua") || lua_pcall(L, 0, 0, 0)) {
//            cout << "Failed to load npc.lua" << endl;
//        }
//        RegisterAPI();
//    }
//    void RegisterAPI() {
//        lua_register(L, "API_attack_npc", API_attack_npc);
//        lua_register(L, "API_get_type", API_get_type);
//        lua_register(L, "API_LV2_range", API_LV2_range);
//        lua_register(L, "API_move_npc", API_move_npc);
//        lua_register(L, "API_SendMessage", API_SendMessage);
//    }
//
//public:
//    static LuaManager* GetInstance() {
//        static LuaManager instance;
//        return &instance;
//    }
//
//    void CallFunction(const string& funcName, int arg) {
//        lua_getglobal(L, funcName.c_str());
//        lua_pushnumber(L, arg);
//        if (lua_pcall(L, 1, 0, 0) != 0) {
//            cout << "Error calling " << funcName << ": " << lua_tostring(L, -1) << endl;
//        }
//    }
//};
//
