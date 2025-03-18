#include "pch.h"
#include "LuaManager.h"

// 플레이어가 NPC를 공격할 때 호출되는 함수
int API_attack_npc(lua_State* L)
{

	cout << "attack_npc" << endl;
	// MY_ID(공격받은 NPC)
	int my_id = (int)lua_tointeger(L, -2);
	// USER_ID : 공격자(나 자신)
	int user_id = (int)lua_tointeger(L, -1);
	lua_pop(L, 3);

	if (clients[my_id]._hp > 0) {
		if (clients[my_id]._is_active != true) {
			clients[my_id]._is_active = true;
		}

		clients[my_id]._hp -= clients[user_id]._atk;


		char buf[100];
		sprintf_s(buf, "player ATTACK!!!! : %s -%d)", clients[my_id]._name, clients[user_id]._atk);
		clients[user_id].send_message2_packet(buf, sizeof(buf), user_id);

		if (clients[my_id]._hp <= 0) {
			clients[my_id]._is_active = false;
			clients[my_id]._state = ST_FREE;
			int item_type = rand() % 2;
			if (item_type == 0) {
				clients[user_id].send_item_drop_packet(0, my_id);
			}
			else {
				clients[user_id].send_item_drop_packet(1, my_id);
			}

			clients[user_id]._exp += (clients[my_id]._level * 10);
			if (clients[user_id]._exp >= clients[user_id]._max_exp)
			{
				clients[user_id]._max_hp = 100;
				clients[user_id]._hp = 100;
				clients[user_id]._max_exp = clients[user_id]._max_exp * 2;
				clients[user_id]._exp = 0;
				clients[user_id]._atk = clients[user_id]._atk * 2;
				clients[user_id]._level += 1;

				char buf[100];
				sprintf_s(buf, "LEVEL UP!!!!!)");
				clients[user_id].send_message2_packet(buf, sizeof(buf), user_id);
			}
			else {
				char buf[100];
				sprintf_s(buf, "EXP UP!!!!! -> %d)", clients[my_id]._level * 10);
				clients[user_id].send_message2_packet(buf, sizeof(buf), user_id);
			}
			clients[user_id].send_stat_change_packet(user_id);
			for (auto& other : clients) {
				if (true == is_npc(other._id)) continue;
				other._s_lock.lock();
				if (ST_INGAME != other._state) {
					other._s_lock.unlock();
					continue;
				}
				else other._s_lock.unlock();

				if (false == can_see(other._id, my_id))
					continue;
				clients[other._id].send_remove_player_packet(my_id);
			}
		}
		else {
			if (!clients[my_id]._isattack) {
				clients[my_id]._isattack = true;
				TIMER_EVENT ev1{ my_id,  chrono::system_clock::now() + 1s , EV_NPC_ATTACK, user_id };
				timer_queue.push(ev1);
			}
		}
	}
	else {
		clients[my_id]._is_active = false;
	}
	return 0;
}

// 객체 타입 가져오기 (플레이어 or NPC)
int API_get_type(lua_State* L) {
	int user_id =
		(int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	int type = clients[user_id]._type;
	lua_pushnumber(L, type);
	return 1;
}


// NPC가 범위 안에 있는지 확인
int API_LV2_range(lua_State* L) {
	int npc_id = (int)lua_tointeger(L, -2);
	int user_id = (int)lua_tointeger(L, -1);

	lua_pop(L, 3);
	bool rt = is_range(5, npc_id, user_id);
	lua_pushboolean(L, rt);
	return 1;
}

// NPC가 일정 시간 동안 플레이어를 따라 움직이도록 설정
int API_move_npc(lua_State* L) {
    int my_id = (int)lua_tointeger(L, -2);
    int user_id = (int)lua_tointeger(L, -1);
    lua_pop(L, 3);

    clients[my_id]._bNpcMove = true;
    auto now = chrono::system_clock::now();

    for (int i = 1; i <= 5; ++i) {
        TIMER_EVENT ev{ my_id, now + i * 1s, EV_NPC_MOVE, user_id };
		timer_queue.push(ev);
    }

    return 0;
}

// 메시지를 클라이언트에 전송
int API_SendMessage(lua_State* L) {
    int my_id = (int)lua_tointeger(L, -3);
    int user_id = (int)lua_tointeger(L, -2);
    const char* mess = lua_tostring(L, -1);
    lua_pop(L, 4);

    clients[my_id].send_chat_packet(user_id, mess);
    return 0;
}