#include "pch.h"
#include "NPCManager.h"
#include "LuaManager.h"
void NPCManager::InitializeNPC()
{
	cout << "NPC intialize begin" << endl;
	for (int i = MAX_USER; i < (MAX_USER + MAX_NPC) / 2; ++i) {

		clients[i].x = rand() % (W_WIDTH / 2); // 왼쪽 절반 영역에 위치
		clients[i].y = rand() % W_HEIGHT;
		clients[i]._id = i;
		sprintf_s(clients[i]._name, "LV1", i);
		clients[i]._state = ST_INGAME;
		clients[i]._type = 1;
		clients[i]._hp = 100;
		clients[i]._level = 1;
		clients[i]._atk = 10;
		clients[i]._range = 1;
		clients[i]._is_active = false;
		clients[i]._sector_x = clients[i].x / SECTOR_WIDTH;
		clients[i]._sector_y = clients[i].y / SECTOR_HEIGHT;
		sector_locks[clients[i]._sector_x][clients[i]._sector_y].lock();
		g_sectors[clients[i]._sector_x][clients[i]._sector_y].insert(i);
		sector_locks[clients[i]._sector_x][clients[i]._sector_y].unlock();
		auto L = clients[i]._L = luaL_newstate();
		luaL_openlibs(L);
		luaL_loadfile(L, "npc.lua");
		lua_pcall(L, 0, 0, 0);

		lua_getglobal(L, "set_uid");
		lua_pushnumber(L, i);
		lua_pcall(L, 1, 0, 0);
		// lua_pop(L, 1);// eliminate set_uid from stack after call

		lua_register(L, "API_SendMessage", API_SendMessage);
		lua_register(L, "API_LV2_range", API_LV2_range);
		lua_register(L, "API_get_type", API_get_type);
		lua_register(L, "API_move_npc", API_move_npc);
		lua_register(L, "API_attack_npc", API_attack_npc);
	}
	for (int i = (MAX_USER + MAX_NPC) / 2; i < MAX_USER + MAX_NPC; ++i) {
		clients[i].x = rand() % ((W_WIDTH / 2) + (W_WIDTH / 2));
		clients[i].y = rand() % W_HEIGHT;
		clients[i]._id = i;
		sprintf_s(clients[i]._name, "LV2", i);
		clients[i]._state = ST_INGAME;
		clients[i]._type = 2;
		clients[i]._hp = 150;
		clients[i]._level = 2;
		clients[i]._atk = 15;
		clients[i]._range = 2;
		clients[i]._is_active = false;
		clients[i]._sector_x = clients[i].x / SECTOR_WIDTH;
		clients[i]._sector_y = clients[i].y / SECTOR_HEIGHT;
		sector_locks[clients[i]._sector_x][clients[i]._sector_y].lock();
		g_sectors[clients[i]._sector_x][clients[i]._sector_y].insert(i);
		sector_locks[clients[i]._sector_x][clients[i]._sector_y].unlock();
		auto L = clients[i]._L = luaL_newstate();
		luaL_openlibs(L);
		luaL_loadfile(L, "npc.lua");
		lua_pcall(L, 0, 0, 0);

		lua_getglobal(L, "set_uid");
		lua_pushnumber(L, i);
		lua_pcall(L, 1, 0, 0);
		// lua_pop(L, 1);// eliminate set_uid from stack after call

		lua_register(L, "API_SendMessage", API_SendMessage);
		lua_register(L, "API_LV2_range", API_LV2_range);
		lua_register(L, "API_get_type", API_get_type);
		lua_register(L, "API_move_npc", API_move_npc);
		lua_register(L, "API_attack_npc", API_attack_npc);
	}
	cout << "NPC initialize end.\n";
}

void NPCManager::DoNPCMove(int npc_id, int player_id)
{
	SESSION& npc = clients[npc_id];
	SESSION& player = clients[player_id];

	unordered_set<int> old_vl;
	{
		unique_lock lock(npc._vl);
		old_vl = npc._view_list;
	}

	int px = player.x, py = player.y;
	int x = npc.x, y = npc.y;

	vector<POINT> path = AStarFindPath(x, y, px, py);
	if (path.empty()) return;

	POINT nextStep = path[0];
	int dx = nextStep.x - x;
	int dy = nextStep.y - y;

	if (is_obstacle(x + dx, y + dy)) return;

	npc.x += dx;
	npc.y += dy;
	npc._count++;

	if (npc._count > 4) {
		npc._bNpcMove = false;
		npc._count = 0;
	}

	int old_sector_x = npc._sector_x;
	int old_sector_y = npc._sector_y;
	int new_sector_x = npc.x / SECTOR_WIDTH;
	int new_sector_y = npc.y / SECTOR_HEIGHT;

	if (old_sector_x != new_sector_x || old_sector_y != new_sector_y) {
		sector_locks[old_sector_x][old_sector_y].lock();
		g_sectors[old_sector_x][old_sector_y].erase(npc_id);
		sector_locks[old_sector_x][old_sector_y].unlock();

		sector_locks[new_sector_x][new_sector_y].lock();
		g_sectors[new_sector_x][new_sector_y].insert(npc_id);
		sector_locks[new_sector_x][new_sector_y].unlock();

		npc._sector_x = new_sector_x;
		npc._sector_y = new_sector_y;
	}

	unordered_set<int> new_vl;
	for (int y = max(new_sector_y - 1, 0); y <= min(new_sector_y + 1, SECTOR_ROWS - 1); y++) {
		for (int x = max(new_sector_x - 1, 0); x <= min(new_sector_x + 1, SECTOR_COLS - 1); x++) {
			lock_guard<mutex> lock(sector_locks[x][y]);
			for (int cl_id : g_sectors[x][y]) {
				if (clients[cl_id]._state != ST_INGAME) continue;
				if (is_npc(cl_id)) continue;
				if (can_see(npc_id, cl_id)) new_vl.insert(cl_id);
			}
		}
	}

	for (auto pl : new_vl) {
		if (old_vl.count(pl) == 0) clients[pl].send_add_player_packet(npc._id);
		else clients[pl].send_move_packet(npc._id);
	}

	for (auto pl : old_vl) {
		if (new_vl.count(pl) == 0) {
			clients[pl]._vl.lock();
			if (clients[pl]._view_list.count(npc._id)) {
				clients[pl]._vl.unlock();
				clients[pl].send_remove_player_packet(npc._id);
			}
			else {
				clients[pl]._vl.unlock();
			}
		}
	}

	unique_lock lock(npc._vl);
	npc._view_list = new_vl;
}

void NPCManager::DoNPCAttack(int npc_id, int user_id)
{
	unordered_set <int> old_vl;
	unordered_set <int> new_vl;

	cout << "do_npc_attack" << endl;

	for (auto& obj : clients) {
		if (obj._state != ST_INGAME)
			continue;
		if (false == is_pc(obj._id))
			continue;
		// npc의 공격 범위 체크
		if (true == is_range(clients[npc_id]._range, npc_id, obj._id))
			old_vl.insert(obj._id);
	}

	if (old_vl.empty())
	{
		clients[npc_id]._is_active = false;
	}
	else
	{
		if (clients[npc_id]._hp > 0)
		{

			for (auto pl : old_vl) {
				clients[pl]._vl.lock();
				clients[pl]._hp -= clients[npc_id]._atk;
				clients[npc_id]._isattack = false;
				clients[pl]._hp = clients[pl]._hp < 0 ? 0 : clients[pl]._hp;
				cout << "atk" << clients[npc_id]._atk << endl;
				clients[pl]._vl.unlock();
				string s = to_string(clients[npc_id]._atk * -1);
				const char* mess = s.c_str();

				char buf[100];
				sprintf_s(buf, "%s ATTACK!!!!PLAYER HP -%d)", clients[npc_id]._name, clients[npc_id]._atk);

				clients[pl].send_message2_packet(buf, sizeof(buf), pl);
				clients[pl].send_stat_change_packet(pl);
				if (clients[pl]._hp <= 0)
				{
					clients[pl]._is_active = false;
					TIMER_EVENT ev1{ pl,  chrono::system_clock::now() + 3s , EV_RESPAWN, pl };
					timer_queue.push(ev1);
				}
			}
			/*  if (!clients[npc_id]._isattack) {
				  clients[npc_id]._isattack = true;
				  TIMER_EVENT ev1{ npc_id,  chrono::system_clock::now() + 1s , EV_ATTACK, user_id };
				  timer_queue.Push(ev1);
			  }*/
		}
		else
			clients[npc_id]._is_active = false;

	}
	return;
}

void NPCManager::WakeUpNPC(int npc_id, int waker)
{
	OVER_EXP* exover = new OVER_EXP;
	exover->_comp_type = OP_NPC_WAKEUP;
	exover->_ai_target_obj = waker;
	PostQueuedCompletionStatus(h_iocp, 1, npc_id, &exover->_over);

	if (clients[npc_id]._is_active) return;
	bool old_state = false;
	if (!atomic_compare_exchange_strong(&clients[npc_id]._is_active, &old_state, true)) return;
}
