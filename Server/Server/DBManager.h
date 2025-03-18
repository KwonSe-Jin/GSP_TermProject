#pragma once
#include "pch.h"


class DBManager {
private:
    concurrency::concurrent_queue<DB_EVENT> dbQueue;
    mutex queueMutex;
    DBManager() {}

public:
    static DBManager* GetInstance() {
        static DBManager instance;
        return &instance;
    }

    void PushDBEvent(int client_id, DB_TYPE type, const string& name = "") {
        dbQueue.push({ client_id, type, name });
    }

    void ProcessDB();

};

