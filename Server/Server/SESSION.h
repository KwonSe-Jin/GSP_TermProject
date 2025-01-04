#pragma once
#include "pch.h"

class Obstacle {
public:
	short x, y;
	int id;
};

class SESSION {

public:
	OVER_EXP _recv_over;
	mutex _s_lock;
	S_STATE _state;
	atomic_bool   _is_active;
	SOCKET _socket;
	//    
	int _id;
	short   x, y;
	char   _name[NAME_SIZE];
	int      _max_exp;
	int      _exp;
	int      _max_hp;
	int      _hp;
	int      _level;
	int      _atk;
	int      _prev_remain;
	int      _range;
	int      _type; // 0 p, lv1, lv2;
	int    _count = 0;
	bool   _bNpcMove = false;
	int     WepitemCount;
	int     hpitemcount;
	bool    _isattack = false;
	bool    _isplayerattack = false;
	int		_sector_x;
	int		_sector_y;
	unordered_set <int> _view_list;
	mutex   _vl;
	int      last_move_time;
	lua_State* _L;
	mutex   _ll;
public:
	SESSION()
	{
		_id = -1;
		_socket = 0;
		x = y = 0;
		WepitemCount = 0;
		hpitemcount = 0;
		_name[0] = 0;
		_state = ST_FREE;
		_prev_remain = 0;
	}

	~SESSION() {}

	void do_recv();
	void do_send(void* packet);
	void send_login_info_packet();
	void send_move_packet(int c_id);
	void send_inventory_packet(int c_id);
	void send_add_player_packet(int c_id);
	void send_message_packet(char* mess, int size, int id);
	void send_message2_packet(char* mess, int size, int id);
	void send_chat_packet(int c_id, const char* mess);
	void send_stat_change_packet(int c_id);
	void send_remove_player_packet(int c_id);
	void send_item_drop_packet(int type, int npc_id);
	

};

