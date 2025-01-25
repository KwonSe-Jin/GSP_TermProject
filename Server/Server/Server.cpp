#include "pch.h"


void WakeUpNPC(int npc_id, int waker)
{
	OVER_EXP* exover = new OVER_EXP;
	exover->_comp_type = OP_NPC_WAKEUP;
	exover->_ai_target_obj = waker;
	PostQueuedCompletionStatus(h_iocp, 1, npc_id, &exover->_over);

	if (clients[npc_id]._is_active) return;
	bool old_state = false;
	if (false == atomic_compare_exchange_strong(&clients[npc_id]._is_active, &old_state, true))
		return;

}

void process_packet(int c_id, char* packet)
{
	switch (packet[2]) {
	case CS_LOGOUT: {
		DB_EVENT db_event{ c_id, DB_SAVE, "" };
		db_queue.push(db_event);
		cout << "클라이언트 " << c_id << " 로그아웃 및 데이터 저장 요청" << endl;
		break;
	}
	case CS_ITEM_USE: {
		CS_ITEM_USE_PACKET* p = reinterpret_cast<CS_ITEM_USE_PACKET*>(packet);
		if (p->itemType == 0)
		{
			if (clients[c_id].WepitemCount >= 1) {
				clients[c_id].WepitemCount--;
				clients[c_id]._atk += 5;
				clients[c_id].send_stat_change_packet(c_id);
			}
		}
		else if (p->itemType == 1)
		{
			if (clients[c_id].hpitemcount >= 1) {
				clients[c_id].hpitemcount--;
				if (clients[c_id]._hp < clients[c_id]._max_hp)
					clients[c_id]._hp += 10;
				else break;
				clients[c_id].send_stat_change_packet(c_id);
			}
		}
		break;
	}
	case CS_GET_ITEM: {
		CS_GET_ITEM_PACKET* p = reinterpret_cast<CS_GET_ITEM_PACKET*>(packet);
		if (p->itemtype == 0) clients[c_id].WepitemCount++;
		else if (p->itemtype == 1) clients[c_id].hpitemcount++;
		cout << "itemcount : " << clients[c_id].WepitemCount << ", " << clients[c_id].hpitemcount << endl;
		clients[c_id].send_inventory_packet(c_id);
		break;
	}
	case CS_MESSAGE: {
		CS_MESSAGE_PACKET* p = reinterpret_cast<CS_MESSAGE_PACKET*>(packet);
		std::cout << p->mess << std::endl;
		int size = sizeof(p->mess);
		for (auto& pl : clients)
		{
			{
				lock_guard<mutex> ll(pl._s_lock);
				if (ST_INGAME != pl._state) continue;
			}
			if (is_pc(pl._id)) pl.send_message_packet(p->mess, size, c_id);
		}
		break;
	}
	case CS_ATTACK: {
		CS_ATTACK_PACKET* p = reinterpret_cast<CS_ATTACK_PACKET*>(packet);
		// cout << p->attack << "asd" <<p->id;
		clients[p->id]._atk = p->attack;
		clients[p->id]._range = 1;
		for (auto& pl : clients) {
			if (pl._id == c_id)
				continue;
			if (ST_INGAME != pl._state)
				continue;
			if (false == is_adjacent(1, c_id, pl._id))
				continue;
			if (true == is_npc(pl._id)) {
				TIMER_EVENT ev1{ pl._id,  chrono::system_clock::now() + 1s , EV_PLAYER_ATTACK, c_id };
				timer_queue.push(ev1);
			}
		}
		break;
	}
	case CS_ROUND_ATK: {
		CS_ROUND_ATTACK_PACKET* p = reinterpret_cast<CS_ROUND_ATTACK_PACKET*>(packet);
		// cout << p->attack << "asd" <<p->id;
		clients[p->id]._atk = p->attack;
		clients[p->id]._range = 4;
		for (auto& pl : clients) {
			if (pl._id == c_id)
				continue;
			if (ST_INGAME != pl._state)
				continue;
			if (false == is_range(2, c_id, pl._id))
				continue;
			if (true == is_npc(pl._id)) {

				TIMER_EVENT ev1{ pl._id,  chrono::system_clock::now() + 1s , EV_PLAYER_ATTACK, c_id };
				timer_queue.push(ev1);
			}
		}
		break;
	}
	case CS_LOGIN: {
		CS_LOGIN_PACKET* p = reinterpret_cast<CS_LOGIN_PACKET*>(packet);
		{
			lock_guard<mutex> ll{ clients[c_id]._s_lock };

			clients[c_id]._state = ST_INGAME;
			strcpy_s(clients[c_id]._name, p->name);
		}
		// DB 작업 큐에 추가
		DB_EVENT db_event{ c_id, DB_LOGIN, p->name };
		db_queue.push(db_event);

		break;
	}
	case CS_MOVE: {
		CS_MOVE_PACKET* p = reinterpret_cast<CS_MOVE_PACKET*>(packet);
		SESSION& client = clients[c_id];

		clients[c_id].last_move_time = p->move_time;

		short x = clients[c_id].x;
		short y = clients[c_id].y;

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

		clients[c_id].x = x;
		clients[c_id].y = y;

		int old_sector_x = clients[c_id]._sector_x;
		int old_sector_y = clients[c_id]._sector_y;
		int new_sector_x = x / SECTOR_WIDTH;
		int new_sector_y = y / SECTOR_HEIGHT;

		if (old_sector_x != new_sector_x || old_sector_y != new_sector_y) {
			sector_locks[old_sector_x][old_sector_y].lock();
			g_sectors[old_sector_x][old_sector_y].erase(c_id);
			sector_locks[old_sector_x][old_sector_y].unlock();

			sector_locks[new_sector_x][new_sector_y].lock();
			g_sectors[new_sector_x][new_sector_y].insert(c_id);
			sector_locks[new_sector_x][new_sector_y].unlock();

			clients[c_id]._sector_x = new_sector_x;
			clients[c_id]._sector_y = new_sector_y;
		}

		unordered_set<int> near_list;
		clients[c_id]._vl.lock();
		unordered_set<int> old_vlist = clients[c_id]._view_list;
		clients[c_id]._vl.unlock();

		for (int y = max(new_sector_y - 1, 0); y <= min(new_sector_y + 1, SECTOR_ROWS - 1); ++y) {
			for (int x = max(new_sector_x - 1, 0); x <= min(new_sector_x + 1, SECTOR_COLS - 1); ++x) {
				lock_guard<mutex> lock(sector_locks[x][y]);
				for (int cl_id : g_sectors[x][y]) {
					if (clients[cl_id]._state != ST_INGAME) continue;
					if (cl_id == c_id) continue;
					if (can_see(c_id, cl_id))
						near_list.insert(cl_id);
				}
			}
		}

		clients[c_id].send_move_packet(c_id);

		for (auto& pl : near_list) {
			auto& cpl = clients[pl];
			if (is_pc(pl)) {
				cpl._vl.lock();
				if (clients[pl]._view_list.count(c_id)) {
					cpl._vl.unlock();
					clients[pl].send_move_packet(c_id);
				}
				else {
					cpl._vl.unlock();
					clients[pl].send_add_player_packet(c_id);
				}
			}
			else if (is_npc(pl)) {
				WakeUpNPC(pl, c_id); // NPC만 호출
			}

			if (old_vlist.count(pl) == 0)
				clients[c_id].send_add_player_packet(pl);
		}

		for (auto& pl : old_vlist) {
			if (0 == near_list.count(pl)) {
				clients[c_id].send_remove_player_packet(pl);
				if (is_pc(pl))
					clients[pl].send_remove_player_packet(c_id);
			}
		}

		clients[c_id]._vl.lock();
		clients[c_id]._view_list = near_list;
		clients[c_id]._vl.unlock();
		break;
	}


	}
}


void Recv(OVER_EXP* exp_over, int c_id, DWORD num_bytes)
{
	int remain_data = num_bytes + clients[c_id]._prev_remain;
	char* packet_start = exp_over->_send_buf;
	int packet_size = *reinterpret_cast<unsigned short*>(packet_start);
	while (packet_size <= remain_data) {
		process_packet(c_id, packet_start);
		packet_start += packet_size;
		remain_data -= packet_size;
		if (0 < remain_data) {
			packet_size = *reinterpret_cast<unsigned short*>(packet_start);
		}
		else break;
	}

	if (remain_data > 0) {
		clients[c_id]._prev_remain = remain_data;
		memcpy(exp_over->_send_buf, packet_start, remain_data);
	}
	clients[c_id].do_recv();
}
void Accept(OVER_EXP* exp_over)
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
void disconnect(int c_id)
{
	clients[c_id]._vl.lock();
	unordered_set <int> vl = clients[c_id]._view_list;
	clients[c_id]._vl.unlock();
	for (auto& p_id : vl) {
		if (is_npc(p_id)) continue;
		auto& pl = clients[p_id];
		{
			lock_guard<mutex> ll(pl._s_lock);
			if (ST_INGAME != pl._state) continue;
		}
		if (pl._id == c_id) continue;
		pl.send_remove_player_packet(c_id);
	}
	closesocket(clients[c_id]._socket);

	lock_guard<mutex> ll(clients[c_id]._s_lock);
	clients[c_id]._state = ST_FREE;
}



// NPC 이동함수


void do_npc_move(int npc_id, int player_id) {
	SESSION& npc = clients[npc_id];
	SESSION& player = clients[player_id];

	unordered_set<int> old_vl;
	{
		std::unique_lock lock(npc._vl);
		old_vl = npc._view_list;
	}

	int px = player.x;
	int py = player.y;

	int x = npc.x;
	int y = npc.y;

	// A* 알고리즘을 사용하여 NPC와 플레이어 간의 경로를 찾음
	vector<POINT> path = AStarFindPath(x, y, px, py);

	// 경로가 없다면 이동하지 않음
	if (path.empty()) {
		return;
	}

	// 경로의 첫 번째 지점으로 NPC를 이동시킴
	POINT nextStep = path[0];
	int dx = nextStep.x - x;
	int dy = nextStep.y - y;

	if (is_obstacle(x + dx, y + dy)) {
		return; 
	}

	// 이동
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
	for (int y = max(new_sector_y - 1, 0); y <= min(new_sector_y + 1, SECTOR_ROWS - 1); ++y) {
		for (int x = max(new_sector_x - 1, 0); x <= min(new_sector_x + 1, SECTOR_COLS - 1); ++x) {
			lock_guard<mutex> lock(sector_locks[x][y]);
			for (int cl_id : g_sectors[x][y]) {
				if (clients[cl_id]._state != ST_INGAME) continue;
				if (true == is_npc(cl_id)) continue;
				if (can_see(npc_id, cl_id))
					new_vl.insert(cl_id);
			}
		}
	}

	for (auto pl : new_vl) {
		if (0 == old_vl.count(pl)) {
			clients[pl].send_add_player_packet(npc._id);
		}
		else {
			clients[pl].send_move_packet(npc._id);
		}
	}

	for (auto pl : old_vl) {
		if (0 == new_vl.count(pl)) {
			clients[pl]._vl.lock();
			if (0 != clients[pl]._view_list.count(npc._id)) {
				clients[pl]._vl.unlock();
				clients[pl].send_remove_player_packet(npc._id);
			}
			else {
				clients[pl]._vl.unlock();
			}
		}
	}

	std::unique_lock lock(npc._vl);
	npc._view_list = new_vl;
}



//npc -> player 공격
void do_npc_attack(int npc_id, int user_id)
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


// player의 공격이 발생했을 때
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


int API_get_type(lua_State* L)
{
	int user_id =
		(int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	int type = clients[user_id]._type;
	lua_pushnumber(L, type);
	return 1;
}

// npc 범위 체크 (범위 안에 있는지)
int API_LV2_range(lua_State* L)
{
	//cout << "is_at_near" << endl;
	int npc_id = (int)lua_tointeger(L, -2);
	int user_id = (int)lua_tointeger(L, -1);

	lua_pop(L, 3);
	bool rt = is_range(5, npc_id, user_id);
	lua_pushboolean(L, rt);
	return 1;
}
// 범위 안에 있을 시 1초에 한번씩 5번 타이머 스레드에 이동 요청(lua)
int API_move_npc(lua_State* L)
{
	int my_id = (int)lua_tointeger(L, -2);
	int user_id = (int)lua_tointeger(L, -1);
	lua_pop(L, 3);

	clients[my_id]._bNpcMove = true;

	auto now = chrono::system_clock::now();
	TIMER_EVENT ev{ my_id, now + 1s, EV_NPC_MOVE, user_id };
	timer_queue.push(ev);
	TIMER_EVENT ev1{ my_id, now + 2s, EV_NPC_MOVE, user_id };
	timer_queue.push(ev1);
	TIMER_EVENT ev2{ my_id, now + 3s, EV_NPC_MOVE, user_id };
	timer_queue.push(ev2);
	TIMER_EVENT ev3{ my_id, now + 4s, EV_NPC_MOVE, user_id };
	timer_queue.push(ev3);
	TIMER_EVENT ev4{ my_id, now + 5s, EV_NPC_MOVE, user_id };
	timer_queue.push(ev4);

	return 0;
}

//---------------------------------------/
int API_SendMessage(lua_State* L)
{
	int my_id = (int)lua_tointeger(L, -3);
	int user_id = (int)lua_tointeger(L, -2);
	char* mess = (char*)lua_tostring(L, -1);

	lua_pop(L, 4);

	clients[my_id].send_chat_packet(user_id, mess);
	return 0;
}

void InitializeNPC()
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


void InitializeObstacle()
{
	cout << "obstacle initialize begin.\n";

	// 점유된 위치를 추적

	unordered_set<pair<short, short>, pair_hash> occupied;

	// NPC 위치 기록
	for (int i = MAX_USER; i < MAX_USER + MAX_NPC; ++i) {
		occupied.emplace(clients[i].x, clients[i].y);
	}

	for (int i = MAX_USER + MAX_NPC; i < MAX_USER + MAX_NPC + MAX_OBSTACLE; ++i) {
		short x, y;

		// 장애물 위치 선정
		do {
			x = rand() % W_WIDTH;
			y = rand() % W_HEIGHT;
		} while (occupied.find({ x, y }) != occupied.end());

		// 업데이트
		occupied.emplace(x, y);

		// 장애물 설정
		clients[i].x = x;
		clients[i].y = y;
		clients[i]._id = i;
		clients[i]._state = ST_INGAME;
		clients[i]._sector_x = clients[i].x / SECTOR_WIDTH;
		clients[i]._sector_y = clients[i].y / SECTOR_HEIGHT;

		// 섹터에 등록
		sector_locks[clients[i]._sector_x][clients[i]._sector_y].lock();
		g_sectors[clients[i]._sector_x][clients[i]._sector_y].insert(i);
		sector_locks[clients[i]._sector_x][clients[i]._sector_y].unlock();
	}

	cout << "obstacle initialize end.\n";
}
void do_timer()
{
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
void do_DB() {
	while (true) {
		DB_EVENT db_event;
		DB& db = DB::getInstance();
		if (db_queue.try_pop(db_event)) {
			switch (db_event.db_type) {
			case DB_LOGIN: {
				if (db.DB_odbc(db_event.client_id, db_event.name.c_str())) {
					cout << "성공" << endl;
					OVER_EXP* ov = new OVER_EXP;
					ov->_comp_type = OP_LOGIN_SUCC;
					PostQueuedCompletionStatus(h_iocp, 1, db_event.client_id, &ov->_over);
				}
				else {
					// 실패 시 기본 데이터 설정
					OVER_EXP* ov = new OVER_EXP;
					ov->_comp_type = OP_LOGIN_FAIL;
					PostQueuedCompletionStatus(h_iocp, 1, db_event.client_id, &ov->_over);
				}
				break;
			}
			case DB_SAVE: {
				db.DB_save(db_event.client_id);
				break;
			}
			}
		}
		else {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
}


void worker_thread(HANDLE h_iocp)
{
	while (true) {
		DWORD num_bytes;
		ULONG_PTR key;
		WSAOVERLAPPED* over = nullptr;
		BOOL ret = GetQueuedCompletionStatus(h_iocp, &num_bytes, &key, &over, INFINITE);
		OVER_EXP* ex_over = reinterpret_cast<OVER_EXP*>(over);
		if (FALSE == ret) {
			if (ex_over->_comp_type == OP_ACCEPT) cout << "Accept Error";
			else {
				cout << "GQCS Error on client[" << key << "]\n";
				disconnect(static_cast<int>(key));
				if (ex_over->_comp_type == OP_SEND) delete ex_over;
				continue;
			}
		}

		if ((0 == num_bytes) && ((ex_over->_comp_type == OP_RECV) || (ex_over->_comp_type == OP_SEND))) {
			disconnect(static_cast<int>(key));
			if (ex_over->_comp_type == OP_SEND) delete ex_over;
			continue;
		}

		switch (ex_over->_comp_type) {
		case OP_ACCEPT: {
			Accept(ex_over);
			break;
		}
		case OP_RECV: {
			Recv(ex_over, static_cast<int>(key), num_bytes);
			break;
		}
		case OP_SEND:
			delete ex_over;
			break;
		case OP_NPC_MOVE: {
			bool keep_alive = false;
			for (int j = 0; j < MAX_USER; ++j) {
				if (clients[j]._state != ST_INGAME) continue;
				if (can_see(static_cast<int>(key), j)) {
					keep_alive = true;
					break;
				}
			}
			if (true == keep_alive) {
				do_npc_move(static_cast<int>(key), ex_over->_ai_target_obj);
			}
			else {
				clients[key]._is_active = false;
			}
			delete ex_over;
		}
						break;
		case OP_NPC_ATTACK: {
			bool keep_alive = false;
			for (int j = 0; j < MAX_USER; ++j) {
				if (clients[j]._state != ST_INGAME) continue;
				if (can_see(static_cast<int>(key), j)) {
					keep_alive = true;
					break;
				}
			}
			if (true == keep_alive) {
				do_npc_attack(static_cast<int>(key), ex_over->_ai_target_obj);
			}
			else {
				clients[key]._is_active = false;
			}
			delete ex_over;
		}
						  break;
		case OP_NPC_WAKEUP: {
			clients[key]._ll.lock();
			auto L = clients[key]._L;

			if (!clients[key]._bNpcMove) {
				lua_getglobal(L, "event_player_move");
				lua_pushnumber(L, ex_over->_ai_target_obj);
				lua_pcall(L, 1, 0, 0);
			}

			//lua_pop(L, 1);
			clients[key]._ll.unlock();
			delete ex_over;
		}
						  break;
		case OP_PLAYER_ATTACK: {
			clients[key]._ll.lock();
			auto L = clients[key]._L;

			lua_getglobal(L, "event_player_attack");
			lua_pushnumber(L, ex_over->_ai_target_obj);
			lua_pcall(L, 1, 0, 0);


			//lua_pop(L, 1);
			clients[key]._ll.unlock();
			delete ex_over;
		}
							 break;
		case OP_RESPAWN: {
			clients[key]._hp = clients[key]._max_hp;
			clients[key]._exp = clients[key]._exp / 2;
			clients[key].x = g_x;
			clients[key].y = g_y;
			clients[key].send_stat_change_packet(key);
			delete ex_over;
		}
					   break;
		case OP_LOGIN_SUCC: {
			{
				lock_guard<mutex> ll{ clients[key]._s_lock };

				g_x = clients[key].x;
				g_y = clients[key].y;


				cout << clients[key]._name << " 로그인 성공" << endl;

				// 섹터 등록 및 주변 객체 알림 처리
				clients[key]._sector_x = clients[key].x / SECTOR_WIDTH;
				clients[key]._sector_y = clients[key].y / SECTOR_HEIGHT;
				sector_locks[clients[key]._sector_x][clients[key]._sector_y].lock();
				g_sectors[clients[key]._sector_x][clients[key]._sector_y].insert(key);
				sector_locks[clients[key]._sector_x][clients[key]._sector_y].unlock();
			}
			clients[key].send_login_info_packet();
			clients[key].send_inventory_packet(key);
			for (int y = max(clients[key]._sector_y - 1, 0); y <= min(clients[key]._sector_y + 1, SECTOR_ROWS - 1); ++y) {
				for (int x = max(clients[key]._sector_x - 1, 0); x <= min(clients[key]._sector_x + 1, SECTOR_COLS - 1); ++x) {
					lock_guard<mutex> lock(sector_locks[x][y]);
					for (int p_id : g_sectors[x][y]) {
						{
							lock_guard<mutex> ll(clients[p_id]._s_lock);
							if (ST_INGAME != clients[p_id]._state) continue;
						}
						if (p_id == key) continue;
						if (false == can_see(key, p_id)) continue;
						if (is_pc(p_id)) clients[p_id].send_add_player_packet(key);
						else if (is_npc(p_id))
							WakeUpNPC(p_id, key);
						clients[key].send_add_player_packet(p_id);
					}
				}
			}
		}
						  break;
		case OP_LOGIN_FAIL: {
			{
				lock_guard<mutex> ll{ clients[key]._s_lock };

				clients[key].x = rand() % W_WIDTH;
				clients[key].y = rand() % W_HEIGHT;

				clients[key]._level = 1;
				clients[key]._atk = 10;
				clients[key]._exp = 0;
				clients[key]._max_hp = 100;
				clients[key]._hp = 100;
				clients[key]._max_exp = 100;
				g_x = clients[key].x;
				g_y = clients[key].y;

				// 섹터 등록 및 주변 객체 알림 처리 (위와 동일)
				clients[key]._sector_x = clients[key].x / SECTOR_WIDTH;
				clients[key]._sector_y = clients[key].y / SECTOR_HEIGHT;
				sector_locks[clients[key]._sector_x][clients[key]._sector_y].lock();
				g_sectors[clients[key]._sector_x][clients[key]._sector_y].insert(key);
				sector_locks[clients[key]._sector_x][clients[key]._sector_y].unlock();

			}
			clients[key].send_login_info_packet();
			for (int y = max(clients[key]._sector_y - 1, 0); y <= min(clients[key]._sector_y + 1, SECTOR_ROWS - 1); ++y) {
				for (int x = max(clients[key]._sector_x - 1, 0); x <= min(clients[key]._sector_x + 1, SECTOR_COLS - 1); ++x) {
					lock_guard<mutex> lock(sector_locks[x][y]);
					for (int p_id : g_sectors[x][y]) {
						{
							lock_guard<mutex> ll(clients[p_id]._s_lock);
							if (ST_INGAME != clients[p_id]._state) continue;
						}
						if (p_id == key) continue;
						if (false == can_see(key, p_id)) continue;
						if (is_pc(p_id)) clients[p_id].send_add_player_packet(key);
						else if (is_npc(p_id))
							WakeUpNPC(p_id, key);
						clients[key].send_add_player_packet(p_id);
					}
				}
			}

			break;
		}
		}
	}
}

int main()
{
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);
	g_s_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT_NUM);
	server_addr.sin_addr.S_un.S_addr = INADDR_ANY;
	bind(g_s_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
	listen(g_s_socket, SOMAXCONN);
	SOCKADDR_IN cl_addr;
	int addr_size = sizeof(cl_addr);

	InitializeNPC();
	InitializeObstacle();
	h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_s_socket), h_iocp, 9999, 0);
	g_c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	g_a_over._comp_type = OP_ACCEPT;
	AcceptEx(g_s_socket, g_c_socket, g_a_over._send_buf, 0, addr_size + 16, addr_size + 16, 0, &g_a_over._over);

	vector <thread> worker_threads;
	int num_threads = std::thread::hardware_concurrency();
	for (int i = 0; i < num_threads; ++i)
		worker_threads.emplace_back(worker_thread, h_iocp);
	thread db_thread(do_DB);
	thread timer_thread{ do_timer };

	for (auto& th : worker_threads)
		th.join();
	db_thread.join();
	timer_thread.join();
	closesocket(g_s_socket);
	WSACleanup();
}
