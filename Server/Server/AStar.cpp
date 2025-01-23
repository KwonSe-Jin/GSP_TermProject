#include "AStar.h"


// �޸���ƽ �Լ�: ����ź �Ÿ� ��� (x1, y1���� x2, y2������ �Ÿ�)
int heuristic(int x1, int y1, int x2, int y2) {
	return abs(x1 - x2) + abs(y1 - y2);
}


// A* �˰����� ����Ͽ� ��θ� ã�� �Լ�
vector<POINT> AStarFindPath(int startX, int startY, int goalX, int goalY) {
	priority_queue<Node> openList;
	unordered_map<pair<int, int>, pair<int, int>, pair_hash> previousNode;// �� ����� ���� ��� ���� ����
	unordered_map<pair<int, int>, int, pair_hash> gCost;// �� �������� ���� ��� ����

	openList.push(Node(startX, startY, 0, heuristic(startX, startY, goalX, goalY)));
	previousNode[{startX, startY}] = { startX, startY };
	gCost[{startX, startY}] = 0;

	// 4����(��, ��, ��, ��) �̵��� ���� ���� ����
	static const int dx[] = { 0, 0, -1, 1 };
	static const int dy[] = { -1, 1, 0, 0 };

	while (!openList.empty()) {
		Node current = openList.top(); // f ���� ���� ���� ��带 ����
		openList.pop();

		if (current.x == goalX && current.y == goalY) {
			vector<POINT> path;
			while (!(current.x == startX && current.y == startY)) {
				path.push_back({ current.x, current.y });
				auto prev = previousNode[{current.x, current.y}];
				current.x = prev.first;
				current.y = prev.second;
			}
			reverse(path.begin(), path.end());
			return path;
		}

		for (int i = 0; i < 4; ++i) {
			int nx = current.x + dx[i], ny = current.y + dy[i];
			if (nx < 0 || ny < 0 || nx >= W_WIDTH || ny >= W_HEIGHT) continue;
			if (is_obstacle(nx, ny)) continue;

			// ���ο� �������� ��� ���
			int newCost = gCost[{current.x, current.y}] + 1;
			if (!gCost.count({ nx, ny }) || newCost < gCost[{nx, ny}]) {
				gCost[{nx, ny}] = newCost;
				int priority = newCost + heuristic(nx, ny, goalX, goalY);
				openList.push(Node(nx, ny, newCost, heuristic(nx, ny, goalX, goalY)));
				previousNode[{nx, ny}] = { current.x, current.y }; // �ش� ����� ���� ��� ����
			}
		}
	}

	return {};  // No path found
}
