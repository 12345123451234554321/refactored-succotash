// libs: -lraylib -lm -ldl -lrt -lX11 -lenet -fsanitize=address,leak,undefined -fno-omit-frame-pointer -g3
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOKERNEL
#define NOUSER
#endif
#include "raylib.h"
#include "raymath.h"
#include <cstring>
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include <cmath>
#include <cstdint>
#include <enet/enet.h>
#include <vector>

constexpr int WIDTH = 800;
constexpr int HEIGHT = 600;
bool isServer = false;
double offset = 0;

enum packetTypes { TYPE_POSITIONS, TYPE_TARGET, TYPE_TIME, TYPE_BUY };

inline double dmgFLin(double t) {
	return t / 20.0 + 1;
}

inline double dmgFSqr(double t) {
	return pow(t / 20.0, 2);
}

inline double dmgFExp(double t) {
	return exp(t / 20.0);
}

inline double rangeFLog(double t) {
	return log2(t + 1) * 5 + 40;
}

inline double fNull(double t) {
	return 0;
}

double dmgFTest(double t) {
	return t / 10.0;
}

double rangeFTest(double t) {
	return 50.0;
}

double getGameTime() {
	if (isServer) {
		return GetTime();
	} else {
		return GetTime() + offset;
	}
}

Color getTeamColor(std::uint8_t team) {
	switch (team) {
		case 0: return RED;
		case 1: return BLUE;
		case 2: return GREEN;
		case 3: return YELLOW;
		case 4: return VIOLET;
		case 5: return ORANGE;
		case 6: return SKYBLUE;
		case 7: return PINK;
		default: return WHITE;
	}
}

double (*dmgTable[])(double) = {fNull, dmgFTest, dmgFLin, dmgFSqr, dmgFExp};
double (*rangeTable[])(double) = {fNull, dmgFLin, rangeFLog};

struct Unit {
	std::uint64_t id;
	std::uint8_t owner;
	std::int16_t type;
	Vector2 pos;
	Vector2 vel;
	float velb;
	Vector2 tgt;
	bool tTgt;
	double hp;
	double maxHp;
	int dmgI;
	double (*dmgF)(double);
	int rangeI;
	double (*rangeF)(double);
	double timeCreat;
};

struct Wall {
	std::uint64_t id;
	std::uint8_t owner;
	std::int16_t type;
	Vector2 pos;
	Vector2 size;
	Vector2 vel;
	float velb;
	Vector2 tgt;
	bool tTgt;
	double hp;
	double maxHp;
	int dmgI;
	double (*dmgF)(double);
	int rangeI;
	double (*rangeF)(double);
	double timeCreat;
};

int main(int argc, char** argv) {
	enet_initialize();
	ENetHost* host = nullptr;
	ENetPeer* peer = nullptr;
	if (argc == 1) {
		ENetAddress address;
		address.host = ENET_HOST_ANY;
		address.port = 52228;
		host = enet_host_create(&address, 32, 2, 0, 0);
		isServer = true;
	} else {
		ENetAddress address;
		enet_address_set_host(&address, argv[1]);
		address.port = 52228;
		host = enet_host_create(nullptr, 1, 2, 0, 0);
		peer = enet_host_connect(host, &address, 2, 0);
		ENetEvent event;
		if (enet_host_service(host, &event, 5000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT) {
			size_t size = sizeof(int) + 2 * sizeof(double);
			char* buf = new char[size];
			char* ptr = buf;
			((int*)ptr)[0] = TYPE_TIME;
			ptr += 4;
			double time = GetTime();
			memcpy(ptr, &time, sizeof(double));
			ENetPacket* packet = enet_packet_create(buf, size, ENET_PACKET_FLAG_RELIABLE);
			enet_peer_send(peer, 0, packet);
			delete[] buf;
		} else {
			enet_peer_reset(peer);
			return 1;
		}
	}
	// gui
	char* funcNames[] = {"t/20 + 1", "(t/20)^2", "exp(t/20)"};
	int selectedFuncIndex = 0;
	float uiAmp = 10.0f;
	float uiFreq = 1.0f;
	int uiCount = 1;

	std::vector<Unit> v;
	std::vector<Wall> w;
	long long drawSelId = 0;
	std::int8_t selSt = 0;
	double selLastCheckT = getGameTime();
	std::uint64_t lid = 3;
	std::vector<long long> scores({0, 0});
	std::vector<long long> money({1000, 1000});
	v.push_back(
	    {1, 1, 0, {300, 20}, {0, 0}, 0.0f, {300, 20}, false, 100000.0, 100000.0, 0, fNull, 0, fNull, getGameTime()});
//	v.push_back(
//	    {2, 0, 1, {300, 540}, {0, 0}, 20.0f, {300, 580}, false, 100.0, 100.0, 2, dmgFLin, 2, rangeFLog, getGameTime()});
	v.push_back(
	    {3, 0, 0, {300, 580}, {0, 0}, 0.0f, {300, 20}, false, 100000.0, 100000.0, 0, fNull, 0, fNull, getGameTime()});

	w.push_back(
	    {1, 0, 0, {10, 600-100}, {20, 200}, {0, 0}, 0.0f, {300, 20}, false, 10000.0, 10000.0, 0, fNull, 0, fNull, getGameTime()});
	w.push_back(
	    {2, 0, 0, {260, 410}, {480, 20}, {0, 0}, 0.0f, {300, 20}, false, 10000.0, 10000.0, 0, fNull, 0, fNull, getGameTime()});
	w.push_back(
	    {3, 1, 0, {600-10, 100}, {20, 200}, {0, 0}, 0.0f, {300, 20}, false, 10000.0, 10000.0, 0, fNull, 0, fNull, getGameTime()});
	w.push_back(
	    {4, 1, 0, {600-260, 600-410}, {480, 20}, {0, 0}, 0.0f, {300, 20}, false, 10000.0, 10000.0, 0, fNull, 0, fNull, getGameTime()});
	InitWindow(WIDTH, HEIGHT, "Test");
	GuiSetStyle(DEFAULT, TEXT_SIZE, 16);
	SetTargetFPS(60);
	while (!WindowShouldClose()) {
		if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
			if (selSt == 0 && (getGameTime() - selLastCheckT > 0.1 || getGameTime() - selLastCheckT < 0)) {
				drawSelId = 0;
				for (int i = 0; i < v.size(); i++) {
					auto& a = v[i];
					if (CheckCollisionPointTriangle(GetMousePosition(), Vector2Add(a.pos, Vector2{10.0f, 10.0f}),
					                                Vector2Add(a.pos, Vector2{0.0f, -10.0f}),
					                                Vector2Add(a.pos, Vector2{-10.0f, 10.0f}))) {
						drawSelId = a.id;
						break;
					}
				}
				if (drawSelId != 0) {
					selSt = 1;
				}
				selLastCheckT = getGameTime();
			} else if (getGameTime() - selLastCheckT > 0.1 || getGameTime() - selLastCheckT < 0) {
				for (int i = 0; i < v.size(); i++) {
					if (v[i].id == drawSelId) {
						v[i].tgt = GetMousePosition();
						v[i].tTgt = true;
						if (!isServer) {
							size_t size = sizeof(int) + sizeof(long long) + sizeof(Vector2);
							char* buf = new char[size];
							char* ptr = buf;
							((int*)ptr)[0] = TYPE_TARGET;
							ptr += 4;
							memcpy(ptr, &v[i].id, sizeof(long long));
							ptr += sizeof(long long);
							memcpy(ptr, &v[i].tgt, sizeof(Vector2));
							ptr += sizeof(Vector2);
							ENetPacket* packet = enet_packet_create(buf, size, ENET_PACKET_FLAG_RELIABLE);
							enet_peer_send(peer, 0, packet);
							delete[] buf;
						}
						break;
					}
				}
				selSt = 0;
				selLastCheckT = getGameTime();
			}
		}
		if (isServer) {
			for (int i = 0; i < v.size();) {
				auto& a = v[i];
				if (a.hp <= 0) {
					if (a.owner == 0) {
						scores[1] += 50 + (a.dmgF(getGameTime() - a.timeCreat) * 2 + (getGameTime() - a.timeCreat) * 5);
						money[1] += 50 + (a.dmgF(getGameTime() - a.timeCreat) * 2 + (getGameTime() - a.timeCreat) * 5);
					} else {
						scores[0] += 50 + (a.dmgF(getGameTime() - a.timeCreat) * 2 + (getGameTime() - a.timeCreat) * 5);
						money[0] += 50 + (a.dmgF(getGameTime() - a.timeCreat) * 2 + (getGameTime() - a.timeCreat) * 5);
					}
					v.erase(i + v.begin());
					continue;
				}
				if (a.tTgt) {
					a.vel = Vector2Scale(Vector2Normalize(Vector2Subtract(a.tgt, a.pos)), a.velb);
				} else {
					a.vel = Vector2Zero();
				}
				Vector2 fPos = Vector2Add(a.pos, Vector2Scale(a.vel, GetFrameTime()));
				for (int j = 0; j < w.size(); j++) {
					if (CheckCollisionPointRec(fPos, {w[j].pos.x-w[j].size.x/2.0f,w[j].pos.y-w[j].size.y/2.0f,w[j].size.x,w[j].size.y})) {
						a.tTgt = false;
					}
				}
				if (Vector2Distance(a.tgt, a.pos) < 1.0f) {
					a.tTgt = false;
				}
				a.pos = Vector2Add(a.pos, Vector2Scale(a.vel, GetFrameTime()));
				i++;
			}
			for (int i = 0; i < v.size(); i++) {
				for (int j = 0; j < v.size(); j++) {
					if (i == j || v[i].owner == v[j].owner)
						continue;
					if (Vector2Distance(v[i].pos, v[j].pos) <= v[i].rangeF(getGameTime() - v[i].timeCreat) &&
					    Vector2DotProduct(Vector2Normalize(Vector2Subtract(v[j].tgt, v[j].pos)),
					                      Vector2Normalize(Vector2Subtract(v[i].pos, v[j].pos))) > 0.0f) {
						v[j].hp -= v[i].dmgF(getGameTime() - v[i].timeCreat) * GetFrameTime();
						v[j].tTgt = false;
					}
				}
			}
		}
		BeginDrawing();
		ClearBackground({0, 0, 0, 255});

		// if (GuiButton((Rectangle){ 24, 24, 120, 30 }, "make red"))
		// v.push_back({++lid,0,0,{100,500},{0,0},20.0f,{100,0},false,100.0,100.0,dmgFTest,rangeFLog,getGameTime()});

		for (int i = 0; i < v.size(); i++) {
			auto& a = v[i];
			if (a.owner == 0) {
				switch (a.type) {
					case 0:	DrawRectangleV(Vector2Subtract(a.pos,{10.0f,10.0f}), {20.0f,20.0f}, RED); break;
					default: DrawTriangle(Vector2Add(a.pos, Vector2{10.0f, 10.0f}), Vector2Add(a.pos, Vector2{0.0f, -10.0f}),
				    	         Vector2Add(a.pos, Vector2{-10.0f, 10.0f}), RED); break;
				}
			} else if (a.owner == 1) {
				switch (a.type) {
					case 0:	DrawRectangleV(Vector2Subtract(a.pos,{10.0f,10.0f}), {20.0f,20.0f}, BLUE); break;
					default: DrawTriangle(Vector2Add(a.pos, Vector2{10.0f, 10.0f}), Vector2Add(a.pos, Vector2{0.0f, -10.0f}),
				    	         Vector2Add(a.pos, Vector2{-10.0f, 10.0f}), BLUE); break;
				}
			}
			if (drawSelId == v[i].id && selSt == 1) {
				DrawRectangleLines(a.pos.x - 10, a.pos.y - 10, 20, 20, {255, 255, 255, 255});
			}
			DrawText(TextFormat("%.4g hp", a.hp), a.pos.x - 20, a.pos.y - 20, 10, {255, 100, 100, 255});
			DrawText(TextFormat("%.3g dmg", a.dmgF(getGameTime() - a.timeCreat)), a.pos.x - 20, a.pos.y + 20, 10,
			         {100, 255, 100, 255});
		}
		for (int i = 0; i < w.size(); i++) {
			DrawRectangleV(Vector2Subtract(w[i].pos,Vector2Scale(w[i].size, 0.5f)),w[i].size, getTeamColor(w[i].owner));
		}
		DrawText(TextFormat("%.2f time, %d mode, money: %lld:%lld", getGameTime() - selLastCheckT, selSt, money[0],
		                    money[1]),
		         10, 10, 30, WHITE);
		GuiLabel((Rectangle){620, 20, 150, 20}, "UNIT CONSTRUCTOR");

		// 1. Выбор функции (ComboBox)
		GuiComboBox((Rectangle){620, 80, 150, 25}, TextJoin(funcNames, 3, ";"), &selectedFuncIndex);

		GuiSpinner((Rectangle){620, 200, 150, 25}, "Count:", &uiCount, 1, 10, false);
		if (GuiButton((Rectangle){620, 240, 120, 30}, "buy")) {
			if (selectedFuncIndex == 0 && money[0] >= 20 * uiCount) {
				if (isServer) {
					money[0] -= 20 * uiCount;
					for (int i = 0; i < uiCount; i++) {
						v.push_back({++lid,
						             0,
						             1,
						             {100, 500},
						             {0, 0},
						             20.0f,
						             {100, 0},
						             false,
						             100.0,
						             100.0,
						             2,
						             dmgFLin,
						             2,
						             rangeFLog,
						             getGameTime()});
					}
				} else {
					size_t size = 3 * sizeof(int);
					char* buf = new char[size];
					char* ptr = buf;
					((int*)ptr)[0] = TYPE_BUY;
					((int*)ptr)[1] = 1;
					((int*)ptr)[2] = uiCount;
					ENetPacket* packet = enet_packet_create(buf, size, ENET_PACKET_FLAG_RELIABLE);
					enet_peer_send(peer, 0, packet);
					delete[] buf;
				}
			} else if (selectedFuncIndex == 1 && money[0] >= 100 * uiCount) {
				if (isServer) {
					money[0] -= 100 * uiCount;
					for (int i = 0; i < uiCount; i++) {
						v.push_back({++lid,
						             0,
						             1,
						             {100, 500},
						             {0, 0},
						             20.0f,
						             {100, 0},
						             false,
						             100.0,
						             100.0,
						             3,
						             dmgFSqr,
						             2,
						             rangeFLog,
						             getGameTime()});
					}
				} else {
					size_t size = 3 * sizeof(int);
					char* buf = new char[size];
					char* ptr = buf;
					((int*)ptr)[0] = TYPE_BUY;
					((int*)ptr)[1] = 2;
					((int*)ptr)[2] = uiCount;
					ENetPacket* packet = enet_packet_create(buf, size, ENET_PACKET_FLAG_RELIABLE);
					enet_peer_send(peer, 0, packet);
					delete[] buf;
				}
			} else if (selectedFuncIndex == 2 && money[0] >= 200 * uiCount) {
				if (isServer) {
					money[0] -= 200 * uiCount;
					for (int i = 0; i < uiCount; i++) {
						v.push_back({++lid,
						             0,
						             1,
						             {100, 500},
						             {0, 0},
						             20.0f,
						             {100, 0},
						             false,
						             100.0,
						             100.0,
						             4,
						             dmgFExp,
						             2,
						             rangeFLog,
						             getGameTime()});
					}
				} else {
					size_t size = 3 * sizeof(int);
					char* buf = new char[size];
					char* ptr = buf;
					((int*)ptr)[0] = TYPE_BUY;
					((int*)ptr)[1] = 3;
					((int*)ptr)[2] = uiCount;
					ENetPacket* packet = enet_packet_create(buf, size, ENET_PACKET_FLAG_RELIABLE);
					enet_peer_send(peer, 0, packet);
					delete[] buf;
				}
			}
		}
		EndDrawing();
		if (isServer) {
			ENetEvent event;
			while (enet_host_service(host, &event, 0) > 0) {
				if (event.type == ENET_EVENT_TYPE_RECEIVE) {
					char* ptr = (char*)event.packet->data;
					switch (((int*)ptr)[0]) {
						case TYPE_TARGET: {
							ptr += 4;
							for (int i = 0; i < v.size(); i++) {
								if (v[i].id == *((long long*)(ptr)) && v[i].owner == 1) {
									v[i].tgt = *(Vector2*)(ptr + 8);
									v[i].tTgt = true;
								}
							}
							break;
						}
						case TYPE_TIME: {
							ptr += 4;
							size_t size = sizeof(int) + 2 * sizeof(double);
							char* buf = new char[size];
							char* ptro = buf;
							((int*)ptro)[0] = TYPE_TIME;
							ptro += 4;
							memcpy(ptro, ptr, sizeof(double));
							ptro += sizeof(double);
							double time = GetTime();
							memcpy(ptro, &time, sizeof(double));
							ptro += sizeof(double);
							ENetPacket* packet = enet_packet_create(buf, size, ENET_PACKET_FLAG_RELIABLE);
							enet_peer_send(event.peer, 0, packet);
							delete[] buf;
							break;
						}
						case TYPE_BUY: {
							ptr += 4;
							if (*(int*)ptr == 1 && money[1] >= 100 * *(int*)(ptr + 4)) {
								ptr += 4;
								for (int i = 0; i < *(int*)ptr; i++) {
									v.push_back({++lid,
									             0,
									             1,
									             {100, 500},
									             {0, 0},
									             20.0f,
									             {100, 0},
									             false,
									             100.0,
									             100.0,
									             2,
									             dmgFLin,
									             2,
									             rangeFLog,
									             getGameTime()});
								}
							} else if (*(int*)ptr == 2 && money[1] >= 100 * *(int*)(ptr + 4)) {
								ptr += 4;
								for (int i = 0; i < *(int*)ptr; i++) {
									v.push_back({++lid,
									             0,
									             1,
									             {100, 500},
									             {0, 0},
									             20.0f,
									             {100, 0},
									             false,
									             80.0,
									             80.0,
									             3,
									             dmgFSqr,
									             2,
									             rangeFLog,
									             getGameTime()});
								}
							} else if (*(int*)ptr == 3 && money[1] >= 200 * *(int*)(ptr + 4)) {
								ptr += 4;
								for (int i = 0; i < *(int*)ptr; i++) {
									v.push_back({++lid,
									             0,
									             1,
									             {100, 500},
									             {0, 0},
									             20.0f,
									             {100, 0},
									             false,
									             50.0,
									             50.0,
									             4,
									             dmgFExp,
									             2,
									             rangeFLog,
									             getGameTime()});
								}
							}
							break;
						}
					}
					enet_packet_destroy(event.packet);
				}
			}
			size_t unitSize = sizeof(uint64_t) + 1 + 2 + 8 + 8 + 4 + 4 + 8;
			size_t totalSize = 12 + (v.size() * unitSize) + (w.size() * (unitSize + 8));

			char* buffer = new char[totalSize];
			char* ptr = buffer;

			((int*)ptr)[0] = TYPE_POSITIONS;
			ptr += 4;

			int count = v.size();
			memcpy(ptr, &count, 4);
			ptr += 4;

			for (auto& u : v) {
				memcpy(ptr, &u.id, 8);
				ptr += 8;
				memcpy(ptr, &u.owner, 1);
				ptr += 1;
				memcpy(ptr, &u.type, 2);
				ptr += 2;
				memcpy(ptr, &u.pos.x, 4);
				ptr += 4;
				memcpy(ptr, &u.pos.y, 4);
				ptr += 4;
				memcpy(ptr, &u.hp, 8);
				ptr += 8;
				memcpy(ptr, &u.dmgI, 4);
				ptr += 4;
				memcpy(ptr, &u.rangeI, 4);
				ptr += 4;
				memcpy(ptr, &u.timeCreat, 8);
				ptr += 8;
			}
			
			int wcount = w.size();
			memcpy(ptr, &wcount, 4);
			ptr += 4;

			for (auto& u : w) {
				memcpy(ptr, &u.id, 8);
				ptr += 8;
				memcpy(ptr, &u.owner, 1);
				ptr += 1;
				memcpy(ptr, &u.type, 2);
				ptr += 2;
				memcpy(ptr, &u.pos.x, 4);
				ptr += 4;
				memcpy(ptr, &u.pos.y, 4);
				ptr += 4;
				memcpy(ptr, &u.size, 8);
				ptr += 8;
				memcpy(ptr, &u.hp, 8);
				ptr += 8;
				memcpy(ptr, &u.dmgI, 4);
				ptr += 4;
				memcpy(ptr, &u.rangeI, 4);
				ptr += 4;
				memcpy(ptr, &u.timeCreat, 8);
				ptr += 8;
			}

			ENetPacket* packet = enet_packet_create(buffer, totalSize, ENET_PACKET_FLAG_RELIABLE);
			enet_host_broadcast(host, 0, packet);
			delete[] buffer;
		} else {
			ENetEvent event;
			while (enet_host_service(host, &event, 0) > 0) {
				if (event.type == ENET_EVENT_TYPE_RECEIVE && !isServer) {
					char* ptr = (char*)event.packet->data;
					switch (((int*)ptr)[0]) {
						case TYPE_POSITIONS: {
							ptr += 4;
							int count;
							memcpy(&count, ptr, sizeof(int));
							ptr += sizeof(int);
							v.clear();
							for (int i = 0; i < count; i++) {
								Unit u;
								memcpy(&u.id, ptr, 8);
								ptr += 8;
								memcpy(&u.owner, ptr, 1);
								ptr += 1;
								memcpy(&u.type, ptr, 2);
								ptr += 2;
								memcpy(&u.pos.x, ptr, 4);
								ptr += 4;
								memcpy(&u.pos.y, ptr, 4);
								ptr += 4;
								memcpy(&u.hp, ptr, 8);
								ptr += 8;
								memcpy(&u.dmgI, ptr, 4);
								ptr += 4;
								memcpy(&u.rangeI, ptr, 4);
								ptr += 4;
								memcpy(&u.timeCreat, ptr, 8);
								ptr += 8;

								u.dmgF = dmgTable[u.dmgI];
								u.rangeF = rangeTable[u.rangeI];
								u.maxHp = 100;
								u.velb = 20;
								u.tTgt = false;

								v.push_back(u);
							}
							int wcount;
							memcpy(&wcount, ptr, sizeof(int));
							ptr += sizeof(int);
							w.clear();
							for (int i = 0; i < wcount; i++) {
								Wall u;
								memcpy(&u.id, ptr, 8);
								ptr += 8;
								memcpy(&u.owner, ptr, 1);
								ptr += 1;
								memcpy(&u.type, ptr, 2);
								ptr += 2;
								memcpy(&u.pos.x, ptr, 4);
								ptr += 4;
								memcpy(&u.pos.y, ptr, 4);
								ptr += 4;
								memcpy(&u.size, ptr, 8);
								ptr += 8;
								memcpy(&u.hp, ptr, 8);
								ptr += 8;
								memcpy(&u.dmgI, ptr, 4);
								ptr += 4;
								memcpy(&u.rangeI, ptr, 4);
								ptr += 4;
								memcpy(&u.timeCreat, ptr, 8);
								ptr += 8;

								u.dmgF = dmgTable[u.dmgI];
								u.rangeF = rangeTable[u.rangeI];
								u.maxHp = 10000;
								u.velb = 0;
								u.tTgt = false;

								w.push_back(u);
							}
							break;
						}
						case TYPE_TIME: {
							ptr += 4;
							double t1, t23;
							memcpy(&t1, ptr, sizeof(double));
							ptr += 8;
							memcpy(&t23, ptr, sizeof(double));
							offset = ((t23 - t1) + (t23 - GetTime())) / 2.0;
							break;
						}
					}
					enet_packet_destroy(event.packet);
				}
			}
		}
	}
	CloseWindow();
	enet_host_destroy(host);
	enet_deinitialize();
	return 0;
}
