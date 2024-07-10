#pragma once
#ifndef __CLOG__
#define __CLOG__
#include "rw_lock.h"
#include <fstream>
#include <string>
#include <tchar.h>

using namespace std;

class logger {
  public:
    logger();
    ~logger();
    template <class T>
    static void WriteLog(T x);
    static void WriteLogFormat(const char *format, ...);
    static string GetSystemTimes();
};

template <class T>
void logger::WriteLog(T x) {
    fstream of("igdlLog.txt", ios::app);

    if (!of.is_open()) return;

    of.seekp(ios::end);
    of << GetSystemTimes() << ": " << x << endl;
    of.close();
}
#endif
