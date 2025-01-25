#pragma once
#include "pch.h"


class DB {
public:
    // �̱��� �ν��Ͻ� ��������
    static DB& getInstance() {
        static DB instance; // ���� �ν��Ͻ�
        return instance;
    }

	bool DB_odbc(int c_id, const char* name);

    bool DB_save(int c_id);



private:
    // ������ �� �Ҹ��� private ó��
    DB() {}
    ~DB() {}

    // ���� �� ���� ������ ����
    DB(const DB&) = delete;
    DB& operator=(const DB&) = delete;
};

