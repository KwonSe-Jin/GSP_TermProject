#include "pch.h"
#include "ClientManager.h"
#include "NPCManager.h"
#include "LuaManager.h"
#include "DBManager.h"

void ClientManager::ProcessIO(int client_id, OVER_EXP* ex_over, DWORD num_bytes)
{

	switch (ex_over->_comp_type) {
	case OP_RECV:

		Recv(client_id, ex_over, num_bytes);
		break;

	case OP_ACCEPT:

		Accept(ex_over);
		break;

	case OP_NPC_MOVE: {
		bool keep_alive = false;
		for (int j = 0; j < MAX_USER; ++j) {
			if (clients[j]._state != ST_INGAME) continue;
			if (can_see(client_id, j)) {
				keep_alive = true;
				break;
			}
		}

		if (keep_alive) {
			NPCManager::GetInstance()->DoNPCMove(client_id, ex_over->_ai_target_obj);
		}
		else {
			clients[client_id]._is_active = false;
		}

		delete ex_over;
		break;
	}

	case OP_NPC_ATTACK: {
		bool keep_alive = false;
		for (int j = 0; j < MAX_USER; ++j) {
			if (clients[j]._state != ST_INGAME) continue;
			if (can_see(client_id, j)) {
				keep_alive = true;
				break;
			}
		}


		if (keep_alive) {
			NPCManager::GetInstance()->DoNPCAttack(client_id, ex_over->_ai_target_obj);
		}
		else {
			clients[client_id]._is_active = false;
		}

		delete ex_over;
		break;
	}

	case OP_NPC_WAKEUP: {
		clients[client_id]._ll.lock();
		auto L = clients[client_id]._L;

		if (!clients[client_id]._bNpcMove) {
			lua_getglobal(L, "event_player_move");
			lua_pushnumber(L, ex_over->_ai_target_obj);
			lua_pcall(L, 1, 0, 0);
		}

		//lua_pop(L, 1);
		clients[client_id]._ll.unlock();
		delete ex_over;
		break;
	}

	case OP_PLAYER_ATTACK: {
		clients[client_id]._ll.lock();
		auto L = clients[client_id]._L;

		lua_getglobal(L, "event_player_attack");
		lua_pushnumber(L, ex_over->_ai_target_obj);
		lua_pcall(L, 1, 0, 0);


		//lua_pop(L, 1);
		clients[client_id]._ll.unlock();
		delete ex_over;
		break;
	}

	case OP_LOGIN_SUCC: {
		// std::lock_guard<std::mutex> lock(clients[client_id]._s_lock);
		HandleLoginSuccess(client_id);
		delete ex_over;
		break;
	}

	case OP_LOGIN_FAIL: {

		{
			lock_guard<mutex> ll{ clients[client_id]._s_lock };

			clients[client_id].x = rand() % W_WIDTH;
			clients[client_id].y = rand() % W_HEIGHT;
			clients[client_id]._level = 1;
			clients[client_id]._atk = 10;
			clients[client_id]._exp = 0;
			clients[client_id]._max_hp = 100;
			clients[client_id]._hp = 100;
			clients[client_id]._max_exp = 100;

			g_x = clients[client_id].x;
			g_y = clients[client_id].y;

			// 섹터 등록
			clients[client_id]._sector_x = clients[client_id].x / SECTOR_WIDTH;
			clients[client_id]._sector_y = clients[client_id].y / SECTOR_HEIGHT;
			sector_locks[clients[client_id]._sector_x][clients[client_id]._sector_y].lock();
			g_sectors[clients[client_id]._sector_x][clients[client_id]._sector_y].insert(client_id);
			sector_locks[clients[client_id]._sector_x][clients[client_id]._sector_y].unlock();
		}

		clients[client_id].send_login_info_packet();

		//  주변 객체 알림
		for (int y = max(clients[client_id]._sector_y - 1, 0); y <= min(clients[client_id]._sector_y + 1, SECTOR_ROWS - 1); ++y) {
			for (int x = max(clients[client_id]._sector_x - 1, 0); x <= min(clients[client_id]._sector_x + 1, SECTOR_COLS - 1); ++x) {
				lock_guard<mutex> lock(sector_locks[x][y]);
				for (int p_id : g_sectors[x][y]) {
					{
						lock_guard<mutex> ll(clients[p_id]._s_lock);
						if (ST_INGAME != clients[p_id]._state) continue;
					}
					if (p_id == client_id) continue;
					if (!can_see(client_id, p_id)) continue;

					if (is_pc(p_id)) clients[p_id].send_add_player_packet(client_id);
					else if (is_npc(p_id)) NPCManager::GetInstance()->WakeUpNPC(p_id, client_id);

					clients[client_id].send_add_player_packet(p_id);
				}
			}
		}
		//  delete ex_over;
		break;
	}

	case OP_RESPAWN: {
		std::lock_guard<std::mutex> lock(clients[client_id]._s_lock);

		// HP 및 EXP 초기화
		clients[client_id]._hp = clients[client_id]._max_hp;
		clients[client_id]._exp = clients[client_id]._exp / 2;

		// 초기 위치로 복귀
		clients[client_id].x = g_x;
		clients[client_id].y = g_y;

		// 클라이언트에게 상태 업데이트 전송
		clients[client_id].send_stat_change_packet(client_id);

		delete ex_over;
		break;
	}
	}
}


void ClientManager::Disconnect(int client_id)
{
	clients[client_id]._vl.lock();
	unordered_set<int> vl = clients[client_id]._view_list;
	clients[client_id]._vl.unlock();

	for (int p_id : vl) {
		if (is_npc(p_id)) continue;
		auto& pl = clients[p_id];

		{
			lock_guard<mutex> ll(pl._s_lock);
			if (ST_INGAME != pl._state) continue;
		}

		pl.send_remove_player_packet(client_id);
	}

	closesocket(clients[client_id]._socket);

	lock_guard<mutex> ll(clients[client_id]._s_lock);
	clients[client_id]._state = ST_FREE;
}

void ClientManager::Recv(int client_id, OVER_EXP* exp_over, DWORD num_bytes)
{

	int remain_data = num_bytes + clients[client_id]._prev_remain;
	char* packet_start = exp_over->_send_buf;
	int packet_size = *reinterpret_cast<unsigned short*>(packet_start);
	while (packet_size <= remain_data) {
		ProcessPacket(client_id, packet_start);
		packet_start += packet_size;
		remain_data -= packet_size;
		if (0 < remain_data) {
			packet_size = *reinterpret_cast<unsigned short*>(packet_start);
		}
		else break;
	}

	if (remain_data > 0) {
		clients[client_id]._prev_remain = remain_data;
		memcpy(exp_over->_send_buf, packet_start, remain_data);
	}
	if (clients[client_id]._socket == INVALID_SOCKET) {
		cout << "[ERROR] Recv() 중 연결이 끊어짐 (client_id: " << client_id << ")" << endl;
		Disconnect(client_id);
		return;
	}
	clients[client_id].do_recv();
}

void ClientManager::Accept(OVER_EXP* exp_over)
{
	static std::atomic<unsigned int> id_generator = 0;
	int client_id = id_generator++;



	if (client_id != -1) {
		{
			lock_guard<mutex> ll(clients[client_id]._s_lock);
			clients[client_id]._state = ST_ALLOC;
		}

		clients[client_id].x = 0;
		clients[client_id].y = 0;
		clients[client_id]._id = client_id;
		clients[client_id]._name[0] = 0;
		clients[client_id]._prev_remain = 0;
		clients[client_id]._socket = g_c_socket;

		CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_c_socket),
			h_iocp, client_id, 0);
		clients[client_id].do_recv();
		ZeroMemory(&g_a_over._over, sizeof(g_a_over._over));
		g_c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		int addr_size = sizeof(SOCKADDR_IN);
		AcceptEx(g_s_socket, g_c_socket, g_a_over._send_buf, 0, addr_size + 16, addr_size + 16, 0, &g_a_over._over);
	}
	else cout << "Max user exceeded.\n";
}

void ClientManager::ProcessPacket(int client_id, char* packet)
{
	switch (packet[2]) {
	case CS_LOGOUT: {
		DBManager::GetInstance()->PushDBEvent(client_id, DB_SAVE);
		cout << "클라이언트 " << client_id << " 로그아웃 및 데이터 저장 요청" << endl;
		break;
	}
	case CS_ITEM_USE: {
		CS_ITEM_USE_PACKET* p = reinterpret_cast<CS_ITEM_USE_PACKET*>(packet);
		if (p->itemType == 0 && clients[client_id].WepitemCount >= 1) {
			clients[client_id].WepitemCount--;
			clients[client_id]._atk += 5;
			clients[client_id].send_stat_change_packet(client_id);
		}
		else if (p->itemType == 1 && clients[client_id].hpitemcount >= 1) {
			clients[client_id].hpitemcount--;
			if (clients[client_id]._hp < clients[client_id]._max_hp) {
				clients[client_id]._hp += 10;
				clients[client_id].send_stat_change_packet(client_id);
			}
		}
		break;
	}
	case CS_GET_ITEM: {
		CS_GET_ITEM_PACKET* p = reinterpret_cast<CS_GET_ITEM_PACKET*>(packet);
		if (p->itemtype == 0) clients[client_id].WepitemCount++;
		else if (p->itemtype == 1) clients[client_id].hpitemcount++;
		cout << "itemcount : " << clients[client_id].WepitemCount << ", " << clients[client_id].hpitemcount << endl;
		clients[client_id].send_inventory_packet(client_id);
		break;
	}
	case CS_MESSAGE: {
		CS_MESSAGE_PACKET* p = reinterpret_cast<CS_MESSAGE_PACKET*>(packet);
		cout << p->mess << endl;
		for (auto& pl : clients) {
			lock_guard<mutex> ll(pl._s_lock);
			if (ST_INGAME != pl._state) continue;
			if (is_pc(pl._id)) pl.send_message_packet(p->mess, sizeof(p->mess), client_id);
		}
		break;
	}
	case CS_ATTACK: {
		CS_ATTACK_PACKET* p = reinterpret_cast<CS_ATTACK_PACKET*>(packet);
		clients[p->id]._atk = p->attack;
		clients[p->id]._range = 1;
		for (auto& pl : clients) {
			if (pl._id == client_id || ST_INGAME != pl._state || !is_adjacent(1, client_id, pl._id)) continue;
			if (is_npc(pl._id)) {
				TIMER_EVENT ev1{ pl._id, chrono::system_clock::now() + 1s, EV_PLAYER_ATTACK, client_id };
				timer_queue.push(ev1);
			}
		}
		break;
	}
	case CS_ROUND_ATK: {
		CS_ROUND_ATTACK_PACKET* p = reinterpret_cast<CS_ROUND_ATTACK_PACKET*>(packet);
		clients[p->id]._atk = p->attack;
		clients[p->id]._range = 4;
		for (auto& pl : clients) {
			if (pl._id == client_id || ST_INGAME != pl._state || !is_range(2, client_id, pl._id)) continue;
			if (is_npc(pl._id)) {
				TIMER_EVENT ev1{ pl._id, chrono::system_clock::now() + 1s, EV_PLAYER_ATTACK, client_id };
				timer_queue.push(ev1);
			}
		}
		break;
	}
	case CS_LOGIN: {
		CS_LOGIN_PACKET* p = reinterpret_cast<CS_LOGIN_PACKET*>(packet);
		{
			lock_guard<mutex> ll{ clients[client_id]._s_lock };
			clients[client_id]._state = ST_INGAME;
			strcpy_s(clients[client_id]._name, p->name);
		}
		DBManager::GetInstance()->PushDBEvent(client_id, DB_LOGIN);
		break;
	}
	case CS_MOVE: {
		CS_MOVE_PACKET* p = reinterpret_cast<CS_MOVE_PACKET*>(packet);
		SESSION& client = clients[client_id];

		clients[client_id].last_move_time = p->move_time;

		short x = clients[client_id].x;
		short y = clients[client_id].y;

		bool isAbleDirection[4] = { true, true, true, true }; // UP RIGHT DOWN LEFT

		// 이동 전에 장애물 확인
		switch (p->direction) {
		case 0: // UP
			if (y > 0 && isAbleDirection[0] && !is_obstacle(x, y - 1)) {
				y--;
			}
			break;
		case 1: // DOWN
			if (y < W_HEIGHT - 1 && isAbleDirection[2] && !is_obstacle(x, y + 1)) {
				y++;
			}
			break;
		case 2: // LEFT
			if (x > 0 && isAbleDirection[3] && !is_obstacle(x - 1, y)) {
				x--;
			}
			break;
		case 3: // RIGHT
			if (x < W_WIDTH - 1 && isAbleDirection[1] && !is_obstacle(x + 1, y)) {
				x++;
			}
			break;
		}

		clients[client_id].x = x;
		clients[client_id].y = y;

		int old_sector_x = clients[client_id]._sector_x;
		int old_sector_y = clients[client_id]._sector_y;
		int new_sector_x = x / SECTOR_WIDTH;
		int new_sector_y = y / SECTOR_HEIGHT;

		if (old_sector_x != new_sector_x || old_sector_y != new_sector_y) {
			sector_locks[old_sector_x][old_sector_y].lock();
			g_sectors[old_sector_x][old_sector_y].erase(client_id);
			sector_locks[old_sector_x][old_sector_y].unlock();

			sector_locks[new_sector_x][new_sector_y].lock();
			g_sectors[new_sector_x][new_sector_y].insert(client_id);
			sector_locks[new_sector_x][new_sector_y].unlock();

			clients[client_id]._sector_x = new_sector_x;
			clients[client_id]._sector_y = new_sector_y;
		}

		unordered_set<int> near_list;
		clients[client_id]._vl.lock();
		unordered_set<int> old_vlist = clients[client_id]._view_list;
		clients[client_id]._vl.unlock();

		for (int y = max(new_sector_y - 1, 0); y <= min(new_sector_y + 1, SECTOR_ROWS - 1); ++y) {
			for (int x = max(new_sector_x - 1, 0); x <= min(new_sector_x + 1, SECTOR_COLS - 1); ++x) {
				lock_guard<mutex> lock(sector_locks[x][y]);
				for (int cl_id : g_sectors[x][y]) {
					if (clients[cl_id]._state != ST_INGAME) continue;
					if (cl_id == client_id) continue;
					if (can_see(client_id, cl_id))
						near_list.insert(cl_id);
				}
			}
		}

		clients[client_id].send_move_packet(client_id);

		for (auto& pl : near_list) {
			auto& cpl = clients[pl];
			if (is_pc(pl)) {
				cpl._vl.lock();
				if (clients[pl]._view_list.count(client_id)) {
					cpl._vl.unlock();
					clients[pl].send_move_packet(client_id);
				}
				else {
					cpl._vl.unlock();
					clients[pl].send_add_player_packet(client_id);
				}
			}
			else if (is_npc(pl)) {
				NPCManager::GetInstance()->WakeUpNPC(pl, client_id);
			}

			if (old_vlist.count(pl) == 0)
				clients[client_id].send_add_player_packet(pl);
		}

		for (auto& pl : old_vlist) {
			if (0 == near_list.count(pl)) {
				clients[client_id].send_remove_player_packet(pl);
				if (is_pc(pl))
					clients[pl].send_remove_player_packet(client_id);
			}
		}

		clients[client_id]._vl.lock();
		clients[client_id]._view_list = near_list;
		clients[client_id]._vl.unlock();
		break;
	}
	}
}

void ClientManager::HandleLoginSuccess(int client_id)
{
	{
		lock_guard<mutex> ll{ clients[client_id]._s_lock };

		g_x = clients[client_id].x;
		g_y = clients[client_id].y;

		cout << clients[client_id]._name << " 로그인 성공" << endl;

		// 섹터 등록
		clients[client_id]._sector_x = clients[client_id].x / SECTOR_WIDTH;
		clients[client_id]._sector_y = clients[client_id].y / SECTOR_HEIGHT;
		sector_locks[clients[client_id]._sector_x][clients[client_id]._sector_y].lock();
		g_sectors[clients[client_id]._sector_x][clients[client_id]._sector_y].insert(client_id);
		sector_locks[clients[client_id]._sector_x][clients[client_id]._sector_y].unlock();
	}

	clients[client_id].send_login_info_packet();
	clients[client_id].send_inventory_packet(client_id);

	// 주변 객체 알림
	for (int y = max(clients[client_id]._sector_y - 1, 0); y <= min(clients[client_id]._sector_y + 1, SECTOR_ROWS - 1); ++y) {
		for (int x = max(clients[client_id]._sector_x - 1, 0); x <= min(clients[client_id]._sector_x + 1, SECTOR_COLS - 1); ++x) {
			lock_guard<mutex> lock(sector_locks[x][y]);
			for (int p_id : g_sectors[x][y]) {
				{
					lock_guard<mutex> ll(clients[p_id]._s_lock);
					if (ST_INGAME != clients[p_id]._state) continue;
				}
				if (p_id == client_id) continue;
				if (!can_see(client_id, p_id)) continue;

				if (is_pc(p_id)) clients[p_id].send_add_player_packet(client_id);
				else if (is_npc(p_id)) NPCManager::GetInstance()->WakeUpNPC(p_id, client_id);

				clients[client_id].send_add_player_packet(p_id);
			}
		}
	}
}
