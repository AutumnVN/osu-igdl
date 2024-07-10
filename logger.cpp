#include "logger.h"
#include <ctime>

using namespace std;

logger::logger() {}
logger::~logger() {}

void logger::WriteLogFormat(const char *format, ...) {
    va_list arglist;
    string strArgData;
    char szBuffer[0x1024];
    ZeroMemory(szBuffer, 0x1024);
    va_start(arglist, format);
    vsprintf_s(szBuffer, format, arglist);
    va_end(arglist);
    strArgData = szBuffer;
    fstream of("igdlLog.txt", ios::app);

    if (!of.is_open()) return;

    of << GetSystemTimes() << ": " << strArgData << endl;
    of.close();
}

string logger::GetSystemTimes() {
    time_t Time;
    tm t;
    CHAR strTime[MAX_PATH];
    ZeroMemory(strTime, MAX_PATH);
    time(&Time);
    localtime_s(&t, &Time);
    strftime(strTime, 100, "%m-%d %H:%M:%S ", &t);
    string strTimes = strTime;
    return strTimes;
}
