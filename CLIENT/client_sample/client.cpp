#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <iostream>
#include <unordered_map>
#include <Windows.h>
#include <chrono>
#include <vector>

using namespace std;

#include "..\..\SERVER\SERVER\protocol.h"

sf::TcpSocket s_socket;

constexpr auto SCREEN_WIDTH = 14;
constexpr auto SCREEN_HEIGHT = 14;

constexpr auto TILE_WIDTH = 65;
constexpr auto WINDOW_WIDTH = SCREEN_WIDTH * TILE_WIDTH;   // size of window
constexpr auto WINDOW_HEIGHT = SCREEN_WIDTH * TILE_WIDTH;

int g_left_x;
int g_top_y;
int g_myid;
bool g_is_chat_mode;

sf::RenderWindow* g_window;
sf::Font g_font;

class OBJECT {
private:
	bool m_showing;
	sf::Sprite m_sprite;

	sf::Text m_name;
	sf::Text m_chat;
	chrono::system_clock::time_point m_mess_end_time;
public:
	int id;
	int m_x, m_y;
	char name[NAME_SIZE];
	int m_hp = 0; int m_exp = 0; int m_attack = 0; int m_level = 0;
	int max_hp;
	OBJECT(sf::Texture& t, int x, int y, int x2, int y2) {
		m_showing = false;
		m_sprite.setTexture(t);
		m_sprite.setTextureRect(sf::IntRect(x, y, x2, y2));
		set_name("NONAME");
		m_mess_end_time = chrono::system_clock::now();
	}
	OBJECT() {
		m_showing = false;
	}
	void show()
	{
		m_showing = true;
	}
	void hide()
	{
		m_showing = false;
	}

	void a_move(int x, int y) {
		m_sprite.setPosition((float)x, (float)y);
	}

	void a_draw() {
		g_window->draw(m_sprite);
	}

	void move(int x, int y) {
		m_x = x;
		m_y = y;
	}
	void draw() {
		if (false == m_showing) return;
		float rx = (m_x - g_left_x) * 65.0f + 1;
		float ry = (m_y - g_top_y) * 65.0f + 1;
		m_sprite.setPosition(rx, ry);
		g_window->draw(m_sprite);
		auto size = m_name.getGlobalBounds();
		if (m_mess_end_time < chrono::system_clock::now()) {
			m_name.setPosition(rx + 32 - size.width / 2, ry - 10);
			g_window->draw(m_name);
		}
		else {
			m_chat.setPosition(rx + 32 - size.width / 2, ry - 10);
			g_window->draw(m_chat);
		}
	}
	void set_name(const char str[]) {
		m_name.setFont(g_font);
		m_name.setString(str);
		if (id < MAX_USER) m_name.setFillColor(sf::Color(255, 255, 255));
		else m_name.setFillColor(sf::Color(255, 255, 0));
		m_name.setStyle(sf::Text::Bold);
	}

	void set_chat(const char str[]) {
		m_chat.setFont(g_font);
		m_chat.setString(str);
		m_chat.setFillColor(sf::Color(255, 255, 255));
		m_chat.setStyle(sf::Text::Bold);
		m_mess_end_time = chrono::system_clock::now() + chrono::seconds(3);
	}
};

class OBSTACLE {
private:
	sf::Texture* texture;
	sf::Sprite sprite;
	short x, y;
	int id;
public:
	OBSTACLE(short x, short y, int id, sf::Texture* texture) : id{ id }, x{ x }, y{ y }, texture{ texture } {
		sprite.setTexture(*texture);
		sprite.setTextureRect(sf::IntRect(0, 0, 64, 64));
	}

	void draw(sf::RenderWindow* w) {
		float rx = (x - g_left_x) * 65.0f + 1;
		float ry = (y - g_top_y) * 65.0f + 1;
		sprite.setPosition(rx, ry);
		w->draw(sprite);
	}
	void CollisionCheck()
	{

	}
};

class Inventory {
public:

	Inventory() : isOn{ false }, attackItem{ 0 }, hpItem{ 0 } {
		attackTexture.loadFromFile("item.png", sf::IntRect(64, 0, 64, 64));
		hpTexture.loadFromFile("item.png", sf::IntRect(0, 0, 64, 64));
		attackSprite.setTexture(attackTexture);
		hpSprite.setTexture(hpTexture);

	}

	void toggle() {
		isOn = !isOn;
	}

	void draw(sf::RenderWindow* w) {
		if (isOn) {
			const float size = 64.f;
			const float space = 6.f;

			const int bgWidth = size * 2 + space * 3;
			const int bgHeight = size + space * 2;

			const int posX = 50.f;
			const int posY = 100.f;

			sf::RectangleShape bg(sf::Vector2f(bgWidth, bgHeight));
			bg.setPosition(posX, posY);
			bg.setOutlineColor(sf::Color::Black);
			bg.setOutlineThickness(2.f);
			bg.setFillColor(sf::Color::White);
			w->draw(bg);

			sf::RectangleShape item1(sf::Vector2f(size, size));
			item1.setPosition(posX + space, posY + space);
			item1.setOutlineColor(sf::Color::Black);
			item1.setOutlineThickness(2.f);
			item1.setFillColor(sf::Color::White);
			w->draw(item1);

			attackSprite.setPosition(posX + space, posY + space);
			w->draw(attackSprite);

			sf::Text text1;
			text1.setFont(g_font);
			text1.setString(to_string(attackItem));
			text1.setPosition(posX + space + 45, posY + space + 30);
			text1.setFillColor(sf::Color::Red);
			text1.setStyle(sf::Text::Bold);
			g_window->draw(text1);

			sf::RectangleShape item2(sf::Vector2f(size, size));
			item2.setPosition(posX + size + space * 2, posY + space);
			item2.setOutlineColor(sf::Color::Black);
			item2.setOutlineThickness(2.f);
			item2.setFillColor(sf::Color::White);
			w->draw(item2);

			sf::Text text2;
			text2.setFont(g_font);
			text2.setString(to_string(hpItem));
			text2.setPosition(posX + size + space * 2 + 45, posY + space + 30);
			text2.setFillColor(sf::Color::Red);
			text2.setStyle(sf::Text::Bold);
			g_window->draw(text2);

			hpSprite.setPosition(posX + size + space * 2, posY + space);
			w->draw(hpSprite);

		}
	}
	void updateItems(int hp, int atk) {
		hpItem = hp;
		attackItem = atk;
	}
	bool UseItem(int type)
	{
		if (type == 0 && attackItem >= 1)
			attackItem--;
		else if (type == 1 && hpItem >= 1)
			hpItem--;
		else return false;
		return true;
	}
	inline bool GetIsOn()
	{
		return isOn;
	}

private:
	bool isOn;
	int attackItem;
	int hpItem;
	sf::Texture attackTexture;
	sf::Texture hpTexture;
	sf::Sprite attackSprite;
	sf::Sprite hpSprite;
};

class Item {
public:
	Item(int type, int x, int y, sf::Texture* texture) : type{ type }, x{ x }, y{ y }, texture{ texture } {
		sprite.setTexture(*texture);
		if (type == 0) sprite.setTextureRect(sf::IntRect(64, 0, 64, 64));
		else if (type == 1) sprite.setTextureRect(sf::IntRect(0, 0, 64, 64));
	}

	void draw(sf::RenderWindow* w) {
		float rx = (x - g_left_x) * 65.0f + 1;
		float ry = (y - g_top_y) * 65.0f + 1;
		sprite.setPosition(rx, ry);
		w->draw(sprite);
	}
	inline int GetposX()
	{
		return x;
	}
	inline int GetposY()
	{
		return y;
	}
	inline int GetType()
	{
		return type;
	}
private:
	int type;
	int x;
	int y;
	sf::Texture* texture;
	sf::Sprite sprite;
};


OBJECT avatar;
unordered_map <int, OBJECT> players;
vector<Item> items;
vector<OBSTACLE> obstacles;
OBJECT white_tile;
OBJECT black_tile;

sf::Texture* board;
sf::Texture* pieces;
sf::Texture* item;

sf::Texture* obstacle;

std::vector<std::string> v;
std::string inputMessage;

Inventory* inventory;

void client_initialize()
{
	board = new sf::Texture;
	pieces = new sf::Texture;
	item = new sf::Texture;
	obstacle = new sf::Texture;
	board->loadFromFile("chessmap.bmp");
	pieces->loadFromFile("chess2.png");
	item->loadFromFile("item.png");
	obstacle->loadFromFile("block.png");
	if (false == g_font.loadFromFile("cour.ttf")) {
		cout << "Font Loading Error!\n";
		exit(-1);
	}
	white_tile = OBJECT{ *board, 5, 5, TILE_WIDTH, TILE_WIDTH };
	black_tile = OBJECT{ *board, 5, 5, TILE_WIDTH, TILE_WIDTH };
	avatar = OBJECT{ *pieces, 192, 0, 64, 64 };
	//for (int i = MAX_USER; i < (MAX_USER + MAX_NPC) / 2; ++i) {
	//	players[i] = OBJECT{ *pieces, 0, 0, 64, 64 };
	//}
	//for (int i = (MAX_USER + MAX_NPC) / 2; i < MAX_USER + MAX_NPC; ++i) {
	//	players[i] = OBJECT{ *pieces, 64, 0, 64, 64 };
	//}
}

void client_finish()
{
	players.clear();
	delete board;
	delete pieces;
	delete inventory;
}

void ProcessPacket(char* ptr)
{
	static bool first_time = true;
	switch (ptr[2])
	{
	case SC_MESSAGE:
	{
		SC_MESSAGE_PACKET* packet = reinterpret_cast<SC_MESSAGE_PACKET*>(ptr);
		g_myid = packet->recvid;
		avatar.id = g_myid;
		v.push_back(std::string(packet->name) + " : " + std::string(packet->mess));
		break;
	}
	case SC_MESSAGE2:
	{
		SC_MESSAGE_PACKET* packet = reinterpret_cast<SC_MESSAGE_PACKET*>(ptr);
		g_myid = packet->recvid;
		avatar.id = g_myid;
		v.push_back(std::string(packet->mess));
		break;
	}
	case SC_LOGIN_INFO:
	{

		SC_LOGIN_INFO_PACKET* packet = reinterpret_cast<SC_LOGIN_INFO_PACKET*>(ptr);
		g_myid = packet->id;
		avatar.id = g_myid;
		avatar.move(packet->x, packet->y);
		g_left_x = packet->x - SCREEN_WIDTH / 2;
		g_top_y = packet->y - SCREEN_HEIGHT / 2;
		avatar.m_attack = packet->atk;
		avatar.m_exp = packet->exp;
		avatar.m_hp = packet->hp;
		avatar.m_level = packet->level;
		avatar.max_hp = packet->max_hp;
		avatar.show();
	}
	break;

	case SC_ADD_OBJECT:
	{
		SC_ADD_OBJECT_PACKET* my_packet = reinterpret_cast<SC_ADD_OBJECT_PACKET*>(ptr);
		int id = my_packet->id;

		if (id == g_myid) {
			avatar.move(my_packet->x, my_packet->y);
			g_left_x = my_packet->x - SCREEN_WIDTH / 2;
			g_top_y = my_packet->y - SCREEN_HEIGHT / 2;
			avatar.show();
		}
		else if (id < MAX_USER) {
			players[id] = OBJECT{ *pieces, 128, 0, 64, 64 };
			players[id].id = id;
			players[id].move(my_packet->x, my_packet->y);
			players[id].set_name(my_packet->name);
			players[id].show();
		}
		else if (MAX_USER <= id && id < (MAX_USER + MAX_NPC) / 2) {
			players[id] = OBJECT{ *pieces, 0, 0, 64, 64 };
			players[id].id = id;
			players[id].move(my_packet->x, my_packet->y);
			players[id].set_name(my_packet->name);
			players[id].show();
		}
		else if ((MAX_USER + MAX_NPC) / 2 <= id && id < (MAX_USER + MAX_NPC)) {
			players[id] = OBJECT{ *pieces, 64, 0, 64, 64 };
			players[id].id = id;
			players[id].move(my_packet->x, my_packet->y);
			players[id].set_name(my_packet->name);
			players[id].show();
		}
		break;
	}
	case SC_MOVE_OBJECT:
	{
		SC_MOVE_OBJECT_PACKET* my_packet = reinterpret_cast<SC_MOVE_OBJECT_PACKET*>(ptr);
		int other_id = my_packet->id;
		if (other_id == g_myid) {
			avatar.move(my_packet->x, my_packet->y);
			g_left_x = my_packet->x - SCREEN_WIDTH / 2;
			g_top_y = my_packet->y - SCREEN_HEIGHT / 2;
		}
		else {
			players[other_id].move(my_packet->x, my_packet->y);
		}
		break;
	}
	case SC_REMOVE_OBJECT:
	{
		SC_REMOVE_OBJECT_PACKET* my_packet = reinterpret_cast<SC_REMOVE_OBJECT_PACKET*>(ptr);
		int other_id = my_packet->id;
		if (other_id == g_myid) {
			avatar.hide();
		}
		else {
			players.erase(other_id);
		}
		break;
	}
	case SC_CHAT:
	{
		SC_CHAT_PACKET* my_packet = reinterpret_cast<SC_CHAT_PACKET*>(ptr);
		cout << my_packet->size;
		int other_id = my_packet->id;
		std::string str(my_packet->mess);
		const int len = my_packet->size - sizeof(my_packet->size) - sizeof(my_packet->id) - sizeof(my_packet->type);
		std::string chat(str.begin(), str.begin() + len);
		cout << chat << endl;
		if (other_id == g_myid) {
			avatar.set_chat(chat.c_str());
		}
		else {
			players[other_id].set_chat(chat.c_str());
		}

		break;
	}
	case SC_STAT_CHANGE: {
		SC_STAT_CHANGE_PACKET* packet = reinterpret_cast<SC_STAT_CHANGE_PACKET*>(ptr);
		avatar.m_hp = packet->hp;
		avatar.m_level = packet->level;
		avatar.max_hp = packet->max_hp;
		avatar.m_exp = packet->exp;
		avatar.m_attack = packet->atk;
		avatar.m_x = packet->x;
		avatar.m_y = packet->y;
		break;
	}
	case SC_ITEM_DROP: {
		SC_ITEM_DROP_PACKET* packet = reinterpret_cast<SC_ITEM_DROP_PACKET*>(ptr);
		cout << "packet->type" << packet->itemType << endl;
		items.push_back(Item(packet->itemType, packet->x, packet->y, item));

		break;
	}
	case SC_IN_INVENTORY: {
		SC_IN_INVENTORY_PACKET* packet = reinterpret_cast<SC_IN_INVENTORY_PACKET*>(ptr);
		cout << "adad : " << packet->hpitemtype << ", " << packet->wepitemtype << endl;
		inventory->updateItems(packet->hpitemtype, packet->wepitemtype);
		break;
	}
	default:
		printf("Unknown PACKET type [%d]\n", ptr[1]);
	}
}

void process_data(char* net_buf, size_t io_byte)
{
	char* ptr = net_buf;
	static size_t in_packet_size = 0;
	static size_t saved_packet_size = 0;
	static char packet_buffer[BUF_SIZE];

	while (0 != io_byte) {
		if (0 == in_packet_size) in_packet_size = ptr[0];
		if (io_byte + saved_packet_size >= in_packet_size) {
			memcpy(packet_buffer + saved_packet_size, ptr, in_packet_size - saved_packet_size);
			ProcessPacket(packet_buffer);
			ptr += in_packet_size - saved_packet_size;
			io_byte -= in_packet_size - saved_packet_size;
			in_packet_size = 0;
			saved_packet_size = 0;
		}
		else {
			memcpy(packet_buffer + saved_packet_size, ptr, io_byte);
			saved_packet_size += io_byte;
			io_byte = 0;
		}
	}
}

void client_main()
{
	char net_buf[BUF_SIZE];
	size_t	received;

	auto recv_result = s_socket.receive(net_buf, BUF_SIZE, received);
	if (recv_result == sf::Socket::Error)
	{
		wcout << L"Recv 에러!";
		exit(-1);
	}
	if (recv_result == sf::Socket::Disconnected) {
		wcout << L"Disconnected\n";
		exit(-1);
	}
	if (recv_result != sf::Socket::NotReady)
		if (received > 0) process_data(net_buf, received);

	for (int i = 0; i < SCREEN_WIDTH; ++i)
		for (int j = 0; j < SCREEN_HEIGHT; ++j)
		{
			int tile_x = i + g_left_x;
			int tile_y = j + g_top_y;
			if ((tile_x < 0) || (tile_y < 0)) continue;
			if (0 == (tile_x / 3 + tile_y / 3) % 2) {
				white_tile.a_move(TILE_WIDTH * i, TILE_WIDTH * j);
				white_tile.a_draw();
			}
			else
			{
				black_tile.a_move(TILE_WIDTH * i, TILE_WIDTH * j);
				black_tile.a_draw();
			}
		}
	avatar.draw();
	for (auto& pl : players) pl.second.draw();
	sf::Text text;
	text.setFont(g_font);
	char buf[100];
	sprintf_s(buf, "(%d, %d)", avatar.m_x, avatar.m_y);
	text.setString(buf);

	sf::Text text1;
	text1.setFont(g_font);
	char buf1[200];
	sprintf_s(buf1, "HP : %d, EXP : %d, LEVEL : %d, ATK : %d", avatar.m_hp, avatar.m_exp,
		avatar.m_level, avatar.m_attack);
	text1.setString(buf1);
	text1.setPosition(0.f, 50.f);
	text1.setFillColor(sf::Color::Red);
	g_window->draw(text);
	g_window->draw(text1);

	const int max_len = 42;
	if (g_is_chat_mode) {
		sf::RectangleShape inputBox;
		inputBox.setFillColor(sf::Color(0, 0, 0, 128));
		inputBox.setSize(sf::Vector2f(600.f, 30.f));
		inputBox.setPosition(5, g_window->getSize().y - 35.f);
		g_window->draw(inputBox);

		sf::Text inputText;
		inputText.setFont(g_font);
		inputText.setString(inputMessage.size() > max_len ? std::string(inputMessage.end() - max_len, inputMessage.end()) : inputMessage);
		inputText.setCharacterSize(24);
		inputText.setFillColor(sf::Color::White);
		inputText.setPosition(15, g_window->getSize().y - 35.f);
		g_window->draw(inputText);


	}

	float outputYOffset = g_is_chat_mode ? 400.f : 365.f;

	sf::RectangleShape outputBox;
	outputBox.setFillColor(sf::Color(0, 0, 0, 128));
	outputBox.setSize(sf::Vector2f(600.f, 360.f));
	outputBox.setPosition(5

		, g_window->getSize().y - outputYOffset);
	g_window->draw(outputBox);

	sf::Text outputText;
	outputText.setFont(g_font);
	outputText.setCharacterSize(24);
	outputText.setFillColor(sf::Color::White);
	outputText.setPosition(15, g_window->getSize().y - outputYOffset);
	outputText.setString("");
	if (v.size() > 13) {
		for (int i = v.size() - 13; i < v.size(); ++i) {
			outputText.setString(outputText.getString() + (v[i].size() > max_len ? std::string(v[i].begin(), v[i].begin() + max_len) : v[i]) + '\n');
		}
	}
	else {
		for (const std::string& m : v)
			outputText.setString(outputText.getString() + (m.size() > max_len ? std::string(m.begin(), m.begin() + max_len) : m) + '\n');
	}

	for (Item& item : items)
		item.draw(g_window);
	for (OBSTACLE& obs : obstacles)
		obs.draw(g_window);

	g_window->draw(outputText);

	inventory->draw(g_window);
}


void send_packet(void* packet)
{
	unsigned char* p = reinterpret_cast<unsigned char*>(packet);
	size_t sent = 0;
	s_socket.send(packet, p[0], sent);
}

void send_attack_packet()
{
	CS_ATTACK_PACKET packet;
	packet.size = sizeof(packet);
	packet.type = CS_ATTACK;
	packet.attack = avatar.m_attack;
	packet.id = avatar.id;
	send_packet(&packet);
}
void send_round_attack_packet()
{
	CS_ROUND_ATTACK_PACKET packet;
	packet.size = sizeof(packet);
	packet.type = CS_ROUND_ATK;
	packet.attack = avatar.m_attack;
	packet.id = avatar.id;
	send_packet(&packet);
}
void send_get_item_packet(int type)
{
	CS_GET_ITEM_PACKET packet;
	packet.size = sizeof(packet);
	packet.type = CS_GET_ITEM;
	packet.itemtype = type;
	send_packet(&packet);
}
void send_use_item(int type)
{
	CS_ITEM_USE_PACKET packet;
	packet.size = sizeof(packet);
	packet.type = CS_ITEM_USE;
	packet.itemType = type;
	send_packet(&packet);
}
void send_logout_packet()
{
	CS_LOGOUT_PACKET packet;
	packet.size = sizeof(packet);
	packet.type = CS_LOGOUT;
	send_packet(&packet);
}
void send_login_packet()
{
	char name[20];
	cout << "이름을 입력 하시오 : ";
	cin >> name;
	avatar.set_name(name);
	CS_LOGIN_PACKET packet;
	packet.size = sizeof(packet);
	packet.type = CS_LOGIN;
	strcpy_s(packet.name, name);
	send_packet(&packet);
}
int main()
{
	wcout.imbue(locale("korean"));
	sf::Socket::Status status = s_socket.connect("127.0.0.1", PORT_NUM);
	s_socket.setBlocking(false);

	if (status != sf::Socket::Done) {
		wcout << L"서버와 연결할 수 없습니다.\n";
		exit(-1);
	}

	client_initialize();
	//CS_LOGIN_PACKET p;
	//p.size = sizeof(p);
	//p.type = CS_LOGIN;

	//string player_name{ "P" };
	//player_name += to_string(GetCurrentProcessId());

	//strcpy_s(p.name, player_name.c_str());
	send_login_packet();
	//

	sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "2D CLIENT");
	g_window = &window;

	g_is_chat_mode = false;

	inventory = new Inventory();

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();

			if (g_is_chat_mode) {
				if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
					g_is_chat_mode = false;
					inputMessage.clear();
				}
				else if (event.type == sf::Event::TextEntered) {
					if (event.text.unicode < 128 && event.text.unicode != 13) {
						if (event.text.unicode != 8 && inputMessage.size() < 45)
							inputMessage += event.text.unicode;
						else if (event.text.unicode == 8 && !inputMessage.empty())
							inputMessage.pop_back();
					}
					else if (event.text.unicode == 13 && !inputMessage.empty()) { // Enter key pressed

						CS_MESSAGE_PACKET p;
						p.size = sizeof(p);
						p.type = CS_MESSAGE;
						strcpy_s(p.name, avatar.name);
						strcpy_s(p.mess, inputMessage.c_str());
						send_packet(&p);
						inputMessage.clear();
					}
				}
			}
			else {
				if (event.type == sf::Event::KeyPressed) {
					int direction = -1;
					switch (event.key.code) {
					case sf::Keyboard::Left:
						direction = 2;
						break;
					case sf::Keyboard::Right:
						direction = 3;
						break;
					case sf::Keyboard::Up:
						direction = 0;
						break;
					case sf::Keyboard::Down:
						direction = 1;
						break;
					case sf::Keyboard::Escape:
						send_logout_packet();
						window.close();
						break;
					case sf::Keyboard::Enter:
						g_is_chat_mode = true;
						break;
					case sf::Keyboard::F:
						inventory->toggle();
						break;
					case sf::Keyboard::A:
						send_attack_packet();
						break;
					case sf::Keyboard::D:
						send_round_attack_packet();
						break;
					case sf::Keyboard::X:
						if (inventory->GetIsOn() && inventory->UseItem(0))
							send_use_item(0);
						break;
					case sf::Keyboard::C:
						if (inventory->GetIsOn() && inventory->UseItem(1))
							send_use_item(1);
						break;
					case sf::Keyboard::Z:
						auto it = std::remove_if(items.begin(), items.end(), [&](Item& item) {
							if (item.GetposX() == avatar.m_x && item.GetposY() == avatar.m_y) {
								send_get_item_packet(item.GetType());
								return true;
							}
							else return false;
							});
						items.erase(it, items.end());
						break;


					}

					if (-1 != direction) {
						CS_MOVE_PACKET p;
						p.size = sizeof(p);
						p.type = CS_MOVE;
						p.direction = direction;
						send_packet(&p);
					}

				}
			}
		}

		window.clear();
		client_main();
		window.display();
	}
	client_finish();

	return 0;
}