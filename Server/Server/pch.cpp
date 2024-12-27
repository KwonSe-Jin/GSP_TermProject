#include "pch.h"


HANDLE h_iocp;
std::array<SESSION, MAX_USER + MAX_NPC + MAX_OBSTACLE> clients;
std::array<array<unordered_set<int>, SECTOR_ROWS>, SECTOR_COLS> g_sectors;
std::array<array<mutex, SECTOR_ROWS>, SECTOR_COLS> sector_locks;
std::array<Obstacle, MAX_OBSTACLE> obstacles;
SOCKET g_s_socket;
SOCKET g_c_socket;
OVER_EXP g_a_over;


bool is_pc(int object_id)
{
	return object_id < MAX_USER;
}

bool is_npc(int object_id)
{
	return (MAX_USER <= object_id && object_id < MAX_USER + MAX_NPC);
}
bool is_obstacle(int object_id)
{
	return (MAX_USER + MAX_NPC <= object_id && object_id < MAX_USER + MAX_NPC + MAX_OBSTACLE);
}

bool can_see(int from, int to)
{
	if (abs(clients[from].x - clients[to].x) > VIEW_RANGE) return false;
	return abs(clients[from].y - clients[to].y) <= VIEW_RANGE;
}
bool is_range(int range, int from, int to)
{
	if (range < abs(clients[from].x - clients[to].x)) return false;
	if (range < abs(clients[from].y - clients[to].y)) return false;
	return true;
}
bool is_adjacent(int range, int from, int to) {
	int dx = abs(clients[from].x - clients[to].x);
	int dy = abs(clients[from].y - clients[to].y);
	return (dx <= range && dy == 0) || (dx == 0 && dy <= range); // 상하좌우 범위 내

}

