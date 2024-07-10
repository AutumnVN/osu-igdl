#pragma once
#include "rw_lock.h"
#include <Windows.h>
#include <iostream>
#include <set>

using namespace std;

#define fsRead(fs, obj) fs.read((char *)&obj, sizeof(obj));
#define fsPass(fs, size) fs.seekg(size, ios::cur);

namespace DB {
    extern set<UINT32> sidDB;
    extern set<UINT32> bidDB;
    extern Lock databaseLock;

    void InitDataBase(string osuDB);
    bool mapExistFast(string url);
    bool mapExist(UINT32 sid);
    void insertSid(UINT32 sid);
};
