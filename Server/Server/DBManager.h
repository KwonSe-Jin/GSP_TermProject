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

    void ProcessDB() {
        cout << "[DBManager] ProcessDB ����" << endl;

        while (true) {
            DB_EVENT db_event;
			DB& db = DB::getInstance();
			if (dbQueue.try_pop(db_event)) {
				switch (db_event.db_type) {
				case DB_LOGIN: {
					if (db.DB_odbc(db_event.client_id, db_event.name.c_str())) {
						cout << "����" << endl;
						OVER_EXP* ov = new OVER_EXP;
						ov->_comp_type = OP_LOGIN_SUCC;
						PostQueuedCompletionStatus(h_iocp, 1, db_event.client_id, &ov->_over);
					}
					else {
						// ���� �� �⺻ ������ ����
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

};

