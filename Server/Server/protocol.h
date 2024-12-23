constexpr int PORT_NUM = 4000;
constexpr int BUF_SIZE = 512;
constexpr int NAME_SIZE = 20;
constexpr int CHAT_SIZE = 300;

constexpr int MAX_USER = 200000;
constexpr int MAX_NPC = 30000;
constexpr int MAX_OBSTACLE = 50000;



constexpr int VIEW_RANGE = 7;
constexpr int W_WIDTH = 2000;
constexpr int W_HEIGHT = 2000;
constexpr int SECTOR_WIDTH = 25;
constexpr int SECTOR_HEIGHT = 25;
constexpr int SECTOR_COLS = W_WIDTH / SECTOR_WIDTH;
constexpr int SECTOR_ROWS = W_HEIGHT / SECTOR_HEIGHT;
// Packet ID
constexpr char CS_LOGIN = 0;
constexpr char CS_MOVE = 1;
constexpr char CS_CHAT = 2;
constexpr char CS_ATTACK = 3;			// 4 방향 공격
constexpr char CS_TELEPORT = 4;			// RANDOM한 위치로 Teleport, Stress Test할 때 Hot Spot현상을 피하기 위해 구현
constexpr char CS_LOGOUT = 5;			// 클라이언트에서 정상적으로 접속을 종료하는 패킷
constexpr char CS_MESSAGE = 6;
constexpr char CS_ROUND_ATK = 7;
constexpr char CS_GET_ITEM= 8;
constexpr char CS_ITEM_USE = 9;

constexpr char SC_LOGIN_INFO = 2;
constexpr char SC_LOGIN_FAIL = 3;
constexpr char SC_ADD_OBJECT = 4;
constexpr char SC_REMOVE_OBJECT = 5;
constexpr char SC_MOVE_OBJECT = 6;
constexpr char SC_CHAT = 7;
constexpr char SC_STAT_CHANGE = 8;
constexpr char SC_MESSAGE = 9;
constexpr char SC_ITEM_DROP = 10;
constexpr char SC_IN_INVENTORY = 11;
constexpr char SC_MESSAGE2 = 12;

#pragma pack (push, 1)
struct CS_LOGIN_PACKET {
	unsigned short size;
	char	type;
	char	name[NAME_SIZE];
};

struct CS_MOVE_PACKET {
	unsigned short size;
	char	type;
	char	direction;  // 0 : UP, 1 : DOWN, 2 : LEFT, 3 : RIGHT
	unsigned	move_time;
};

struct CS_CHAT_PACKET {
	unsigned short size;			// 크기가 가변이다, mess가 작으면 size도 줄이자.
	char	type;
	char	mess[CHAT_SIZE];
};

struct CS_TELEPORT_PACKET {			// 랜덤으로 텔레포트 하는 패킷, 동접 테스트에 필요
	unsigned short size;
	char	type;
};

struct CS_LOGOUT_PACKET {
	unsigned short size;
	char	type;
};

struct SC_LOGIN_INFO_PACKET {
	unsigned short size;
	char	type;
	int		visual;				// 종족, 성별등을 구분할 때 사용
	int		id;
	int		hp;
	int		max_hp;
	int		exp;
	int		level;
	int		atk;
	short	x, y;
};

struct SC_ADD_OBJECT_PACKET {
	unsigned short size;
	char	type;
	int		id;
	int		visual;				// 어떻게 생긴 OBJECT인가를 지시
	short	x, y;
	char	name[NAME_SIZE];
};

struct SC_REMOVE_OBJECT_PACKET {
	unsigned short size;
	char	type;
	int		id;
};

struct SC_MOVE_OBJECT_PACKET {
	unsigned short size;
	char	type;
	int		id;
	short	x, y;
	unsigned int move_time;
};

struct SC_CHAT_PACKET {
	unsigned short size;
	char	type;
	int		id;
	char	mess[CHAT_SIZE];
};

struct SC_LOGIN_FAIL_PACKET {
	unsigned short size;
	char	type;
};

struct SC_STAT_CHANGE_PACKET {
	unsigned short size;
	char	type;
	int		hp;
	int		max_hp;
	int		exp;
	int		level;
	int		atk;
	short	x, y;
};
struct CS_MESSAGE_PACKET {
	unsigned short size;
	char	type;
	char	name[NAME_SIZE];
	char	mess[50];
};
struct SC_MESSAGE_PACKET {
	unsigned short size;
	char	type;
	char	name[NAME_SIZE];
	int		recvid;
	char	mess[50];
};
struct SC_MESSAGE2_PACKET {
	unsigned short size;
	char	type;
	char	name[NAME_SIZE];
	int		recvid;
	char	mess[50];
};
struct CS_ATTACK_PACKET {
	unsigned short size;
	char	type;
	int		id;
	int		attack; // 공격력
};
struct CS_ROUND_ATTACK_PACKET {
	unsigned short size;
	char	type;
	int		id;
	int		attack; // 공격력
};
struct SC_ITEM_DROP_PACKET {
	unsigned short size;
	char	type;
	int		itemType; // 0은 무기, 1는 물약
	short	x, y;
};
struct CS_GET_ITEM_PACKET {
	unsigned short size;
	char	type;
	int		id;
	int		itemtype; 
};
struct SC_IN_INVENTORY_PACKET {
	unsigned short size;
	char	type;
	int		hpitemtype;
	int		wepitemtype;
};
struct CS_ITEM_USE_PACKET {
	unsigned short size;
	char	type;
	int		itemType;
};
#pragma pack (pop)