#include "pch.h"
#include "SESSION.h"

void SESSION::do_recv()
{
	DWORD recv_flag = 0;
	memset(&_recv_over._over, 0, sizeof(_recv_over._over));
	_recv_over._wsabuf.len = BUF_SIZE - _prev_remain;
	_recv_over._wsabuf.buf = _recv_over._send_buf + _prev_remain;
	WSARecv(_socket, &_recv_over._wsabuf, 1, 0, &recv_flag,
		&_recv_over._over, 0);
}

void SESSION::do_send(void* packet)
{

	OVER_EXP* sdata = new OVER_EXP{ reinterpret_cast<char*>(packet) };
	WSASend(_socket, &sdata->_wsabuf, 1, 0, 0, &sdata->_over, 0);
}


void SESSION::send_inventory_packet(int c_id)
{
	SC_IN_INVENTORY_PACKET p;
	p.size = sizeof(SC_IN_INVENTORY_PACKET);
	p.type = SC_IN_INVENTORY;
	p.wepitemtype = clients[c_id].WepitemCount;
	p.hpitemtype = clients[c_id].hpitemcount;
	do_send(&p);
}



void SESSION::send_login_info_packet()
{

	SC_LOGIN_INFO_PACKET p;
	p.id = _id;
	p.size = sizeof(SC_LOGIN_INFO_PACKET);
	p.type = SC_LOGIN_INFO;
	p.x = x;
	p.y = y;
	p.max_hp = _max_hp;
	p.exp = _exp;
	p.hp = _hp;
	p.level = _level;
	p.atk = _atk;
	do_send(&p);

}

void SESSION::send_move_packet(int c_id)
{
	SC_MOVE_OBJECT_PACKET p;
	p.id = c_id;
	p.size = sizeof(SC_MOVE_OBJECT_PACKET);
	p.type = SC_MOVE_OBJECT;
	p.x = clients[c_id].x;
	p.y = clients[c_id].y;
	p.move_time = clients[c_id].last_move_time;
	do_send(&p);
}

void SESSION::send_add_player_packet(int c_id)
{
	SC_ADD_OBJECT_PACKET add_packet;
	add_packet.id = c_id;
	strcpy_s(add_packet.name, clients[c_id]._name);
	add_packet.size = sizeof(add_packet);
	add_packet.type = SC_ADD_OBJECT;
	add_packet.x = clients[c_id].x;
	add_packet.y = clients[c_id].y;
	_vl.lock();
	_view_list.insert(c_id);
	_vl.unlock();
	do_send(&add_packet);
}

void SESSION::send_chat_packet(int p_id, const char* mess)
{
	std::string str(mess);

	SC_CHAT_PACKET packet;
	packet.id = p_id;
	packet.type = SC_CHAT;
	strcpy_s(packet.mess, str.c_str());
	std::cout << packet.mess << std::endl;
	packet.size = sizeof(packet.id) + sizeof(packet.type) + sizeof(packet.size) + str.length();
	do_send(&packet);
}

// 플레이어 간의 채팅
void SESSION::send_message_packet(char* mess, int size, int id)
{
	SC_MESSAGE_PACKET packet;
	packet.size = sizeof(SC_MESSAGE_PACKET);
	strcpy_s(packet.name, clients[id]._name);
	packet.recvid = _id;
	packet.type = SC_MESSAGE;
	strcpy_s(packet.mess, size, mess);
	do_send(&packet);
}

// 채팅창에 공격 상태 뜨는 메세지
void SESSION::send_message2_packet(char* mess, int size, int id)
{
	SC_MESSAGE2_PACKET packet;
	packet.size = sizeof(SC_MESSAGE2_PACKET);
	strcpy_s(packet.name, clients[id]._name);
	packet.recvid = _id;
	packet.type = SC_MESSAGE2;
	strcpy_s(packet.mess, size, mess);
	do_send(&packet);
}
void SESSION::send_stat_change_packet(int c_id)
{
	// attack 넣기 
	SC_STAT_CHANGE_PACKET p;
	p.size = sizeof(p);
	p.type = SC_STAT_CHANGE;
	p.hp = clients[c_id]._hp;
	p.max_hp = clients[c_id]._max_hp;
	p.level = clients[c_id]._level;
	p.exp = clients[c_id]._exp;
	p.atk = clients[c_id]._atk;
	p.x = clients[c_id].x;
	p.y = clients[c_id].y;
	clients[c_id].do_send(&p);
}

void SESSION::send_remove_player_packet(int c_id)
{
	_vl.lock();
	if (_view_list.count(c_id))
		_view_list.erase(c_id);
	else {
		_vl.unlock();
		return;
	}
	_vl.unlock();
	SC_REMOVE_OBJECT_PACKET p;
	p.id = c_id;
	p.size = sizeof(p);
	p.type = SC_REMOVE_OBJECT;
	do_send(&p);
}

void SESSION::send_item_drop_packet(int type, int npc_id)
{
	SC_ITEM_DROP_PACKET p;
	p.size = sizeof(p);
	p.type = SC_ITEM_DROP;
	p.itemType = type;
	p.x = clients[npc_id].x;
	p.y = clients[npc_id].y;
	for (auto& pl : clients) {
		{
			lock_guard<mutex> ll(pl._s_lock);
			if (ST_INGAME != pl._state) continue;
		}
		if (false == can_see(npc_id, pl._id)) continue;
		if (is_pc(pl._id)) clients[pl._id].do_send(&p);
	}
}
