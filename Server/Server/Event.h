#pragma once
#include "pch.h"

struct TIMER_EVENT {
	int obj_id;
	chrono::system_clock::time_point wakeup_time;
	EVENT_TYPE event_id;
	int target_id;
	constexpr bool operator < (const TIMER_EVENT& L) const
	{
		return (wakeup_time > L.wakeup_time);
	}
};

struct DB_EVENT {
	int client_id;                           // �۾� ��� Ŭ���̾�Ʈ ID
	DB_TYPE db_type;                         // �۾� Ÿ�� (DB_LOAD, DB_SAVE)
	std::string name;                        // �۾��� �ʿ��� �߰� ������ (��: ���� �̸�)
};