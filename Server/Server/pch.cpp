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

// 좌표에 있는 object의 ID를 반환
bool is_obstacle(short x, short y) {
	int sector_x = x / SECTOR_WIDTH;
	int sector_y = y / SECTOR_HEIGHT;

	// 해당 섹터 내에서 장애물만 검색
	for (int id : g_sectors[sector_x][sector_y]) {
		if (clients[id].x == x && clients[id].y == y && id >= MAX_USER + MAX_NPC) {
			return true;
		}
	}
	return false; 
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

