#pragma once
#include "pch.h"

struct Node {
	int x, y;
	int f, g, h;

	Node(int x, int y, int g, int h)
		: x(x), y(y), g(g), h(h), f(g + h) {}

	//  우선순위 큐에서는 f 값이 작은 순서대로 처리
	bool operator<(const Node& other) const {
		return f > other.f;
	}
};

// 휴리스틱 함수: 맨하탄 거리 계산 (x1, y1에서 x2, y2까지의 거리)
extern int heuristic(int x1, int y1, int x2, int y2);

// A* 알고리즘을 사용하여 경로를 찾는 함수
extern vector<POINT> AStarFindPath(int startX, int startY, int goalX, int goalY);

class AStar
{
};

