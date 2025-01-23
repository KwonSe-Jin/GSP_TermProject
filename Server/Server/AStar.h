#pragma once
#include "pch.h"

struct Node {
	int x, y;
	int f, g, h;

	Node(int x, int y, int g, int h)
		: x(x), y(y), g(g), h(h), f(g + h) {}

	//  �켱���� ť������ f ���� ���� ������� ó��
	bool operator<(const Node& other) const {
		return f > other.f;
	}
};

// �޸���ƽ �Լ�: ����ź �Ÿ� ��� (x1, y1���� x2, y2������ �Ÿ�)
extern int heuristic(int x1, int y1, int x2, int y2);

// A* �˰����� ����Ͽ� ��θ� ã�� �Լ�
extern vector<POINT> AStarFindPath(int startX, int startY, int goalX, int goalY);

class AStar
{
};

