#include "ObstacleManager.h"

void ObstacleManager::InitializeObstacle()
{
	cout << "obstacle initialize begin.\n";

	// ������ ��ġ�� ����

	unordered_set<pair<short, short>, pair_hash> occupied;

	// NPC ��ġ ���
	for (int i = MAX_USER; i < MAX_USER + MAX_NPC; ++i) {
		occupied.emplace(clients[i].x, clients[i].y);
	}

	for (int i = MAX_USER + MAX_NPC; i < MAX_USER + MAX_NPC + MAX_OBSTACLE; ++i) {
		short x, y;

		// ��ֹ� ��ġ ����
		do {
			x = rand() % W_WIDTH;
			y = rand() % W_HEIGHT;
		} while (occupied.find({ x, y }) != occupied.end());

		// ������Ʈ
		occupied.emplace(x, y);

		// ��ֹ� ����
		clients[i].x = x;
		clients[i].y = y;
		clients[i]._id = i;
		clients[i]._state = ST_INGAME;
		clients[i]._sector_x = clients[i].x / SECTOR_WIDTH;
		clients[i]._sector_y = clients[i].y / SECTOR_HEIGHT;

		// ���Ϳ� ���
		sector_locks[clients[i]._sector_x][clients[i]._sector_y].lock();
		g_sectors[clients[i]._sector_x][clients[i]._sector_y].insert(i);
		sector_locks[clients[i]._sector_x][clients[i]._sector_y].unlock();
	}

	cout << "obstacle initialize end.\n";
}
