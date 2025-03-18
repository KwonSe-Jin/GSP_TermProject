#pragma once
#include "pch.h"

class TimerManager {
private:
    TimerManager() {}

public:
    static TimerManager* GetInstance() {
        static TimerManager instance;
        return &instance;
    }

    void ProcessTimer() {
		cout << " 타이머 실행";
		while (true) {
			while (!timer_queue.empty())
			{
				TIMER_EVENT ev;
				if (true == timer_queue.try_pop(ev)) {
					if (ev.wakeup_time <= chrono::system_clock::now()) {
						if (ev.event_id == EV_NPC_MOVE) {
							OVER_EXP* ov = new OVER_EXP;
							ov->_comp_type = OP_NPC_MOVE;
							ov->_ai_target_obj = ev.target_id;
							PostQueuedCompletionStatus(h_iocp, 1, ev.obj_id, &ov->_over);
							break;
						}
						else if (ev.event_id == EV_NPC_ATTACK) {
							if (clients[ev.obj_id]._hp > 0) {
								OVER_EXP* ov = new OVER_EXP;
								ov->_comp_type = OP_NPC_ATTACK;
								ov->_ai_target_obj = ev.target_id;
								PostQueuedCompletionStatus(h_iocp, 1, ev.obj_id, &ov->_over);
							}
							else  clients[ev.obj_id]._is_active = false;
							break;
						}
						else if (ev.event_id == EV_RESPAWN) {
							OVER_EXP* ov = new OVER_EXP;
							ov->_comp_type = OP_RESPAWN;
							ov->_ai_target_obj = ev.target_id;
							PostQueuedCompletionStatus(h_iocp, 1, ev.obj_id, &ov->_over);
							break;
						}
						else if (ev.event_id == EV_PLAYER_ATTACK) {
							OVER_EXP* ov = new OVER_EXP;
							ov->_comp_type = OP_PLAYER_ATTACK;
							ov->_ai_target_obj = ev.target_id;
							PostQueuedCompletionStatus(h_iocp, 1, ev.obj_id, &ov->_over);
							break;
						}
						continue;
					}
					else {
						timer_queue.push(ev);
						this_thread::sleep_for(1ms);
						continue;
					}
				}
			}
		}
    }
};
