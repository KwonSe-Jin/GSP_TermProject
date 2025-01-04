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
	int client_id;                           // 작업 대상 클라이언트 ID
	DB_TYPE db_type;                         // 작업 타입 (DB_LOAD, DB_SAVE)
	std::string name;                        // 작업에 필요한 추가 데이터 (예: 유저 이름)
};