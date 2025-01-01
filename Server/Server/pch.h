#pragma once
#include <iostream>
#include <array>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <thread>
#include <vector>
#include <mutex>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <queue>
#include <string>
#include <concurrent_priority_queue.h>
#include <concurrent_queue.h>
#include <random>
#include <atomic>
#include <limits>
#include <shared_mutex>
#include <windows.h>
#include <sqlext.h>
#include "include/lua.hpp"


#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")
#pragma comment(lib, "lua54.lib")
using namespace std;
enum COMP_TYPE { OP_ACCEPT, OP_RECV, OP_SEND, OP_NPC_MOVE, OP_NPC_ATTACK, OP_NPC_WAKEUP, OP_PLAYER_ATTACK, OP_RESPAWN , OP_LOGIN_SUCC, OP_LOGIN_FAIL, OP_LOGOUT };
enum EVENT_TYPE { EV_NPC_MOVE, EV_NPC_ATTACK, EV_RESPAWN, EV_PLAYER_ATTACK };
enum DB_TYPE {DB_LOGIN, DB_SAVE};
enum S_STATE { ST_FREE, ST_ALLOC, ST_INGAME };

#include "protocol.h"
#include "OVERLAP.h"
#include "SESSION.h"



extern HANDLE h_iocp;
extern std::array<SESSION, MAX_USER + MAX_NPC + MAX_OBSTACLE> clients; 
extern SOCKET g_s_socket;
extern SOCKET g_c_socket;
extern OVER_EXP g_a_over;
extern array<array<unordered_set<int>, SECTOR_ROWS>, SECTOR_COLS> g_sectors;
extern array<array<mutex, SECTOR_ROWS>, SECTOR_COLS> sector_locks;
extern array<Obstacle, MAX_OBSTACLE> obstacles;


bool is_pc(int object_id);

bool is_npc(int object_id);
bool is_obstacle(short x, short y);
bool can_see(int from, int to);
bool is_range(int range, int from, int to);
bool is_adjacent(int range, int from, int to);

