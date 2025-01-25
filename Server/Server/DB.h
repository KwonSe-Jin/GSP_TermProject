#pragma once
#include "pch.h"


class DB {
public:
    // 싱글톤 인스턴스 가져오기
    static DB& getInstance() {
        static DB instance; // 정적 인스턴스
        return instance;
    }

	bool DB_odbc(int c_id, const char* name);

    bool DB_save(int c_id);



private:
    // 생성자 및 소멸자 private 처리
    DB() {}
    ~DB() {}

    // 복사 및 대입 연산자 삭제
    DB(const DB&) = delete;
    DB& operator=(const DB&) = delete;
};

