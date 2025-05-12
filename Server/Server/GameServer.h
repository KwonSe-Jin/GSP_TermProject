#pragma once
#include "pch.h"
#include "DBManager.h"
#include "TimerManager.h"
#include "ClientManager.h"
#include "NPCManager.h"
#include "ObstacleManager.h"
class GameServer
{
private:
    vector<thread> worker_threads;
    thread db_thread, timer_thread;

public:
    GameServer() {
        WSAData WSAData;
        WSAStartup(MAKEWORD(2, 2), &WSAData);

        g_s_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
        SOCKADDR_IN server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(PORT_NUM);
        server_addr.sin_addr.S_un.S_addr = INADDR_ANY;
        bind(g_s_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
        listen(g_s_socket, SOMAXCONN);

        h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
        CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_s_socket), h_iocp, 9999, 0);

        g_c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
        g_a_over._comp_type = OP_ACCEPT;
        AcceptEx(g_s_socket, g_c_socket, g_a_over._send_buf, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, 0, &g_a_over._over);
    }

    ~GameServer() {
        closesocket(g_s_socket);
        WSACleanup();
    }

    void StartThreads() {
        int num_threads = std::thread::hardware_concurrency();
        cout << num_threads;
        for (int i = 0; i < num_threads; ++i)
            worker_threads.emplace_back([&]() { WorkerThread(); });

        db_thread = thread(&DBManager::ProcessDB, DBManager::GetInstance());
        timer_thread = thread(&TimerManager::ProcessTimer, TimerManager::GetInstance());
    }

    void Run() {
      
        NPCManager::GetInstance()->InitializeNPC();
        ObstacleManager::GetInstance()->InitializeObstacle();

        StartThreads();

        for (auto& th : worker_threads) th.join();
        db_thread.join();
        timer_thread.join();
    }

    void WorkerThread() {
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
                    ClientManager::GetInstance()->Disconnect(static_cast<int>(key));
                    if (ex_over->_comp_type == OP_SEND) delete ex_over;
                    continue;
                }
            }

            if ((num_bytes == 0) && ((ex_over->_comp_type == OP_RECV) || (ex_over->_comp_type == OP_SEND))) {
                ClientManager::GetInstance()->Disconnect(static_cast<int>(key));
                if (ex_over->_comp_type == OP_SEND) delete ex_over;
                continue;
            }

            ClientManager::GetInstance()->ProcessIO(static_cast<int>(key), ex_over, num_bytes);
        }
    }


};

