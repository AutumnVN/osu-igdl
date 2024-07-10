#pragma once
#include "rw_lock.h"
#include <curl/curl.h>
#include <iostream>
#include <map>
#include <vector>

using namespace std;

enum DlType {
    FULL,
    NOVIDEO,
    MINI
};

enum DlStatus {
    NONE,
    PARSE,
    DOWNLOAD
};

enum Category {
    UNKNOWN = -3,
    GRAVEYARD = -2,
    WIP = -1,
    PENDING = 0,
    RANKED = 1,
    APPROVED = 2,
    QUALIFIED = 3,
    LOVED = 4
};

struct DlInfo {
    DlStatus dlStatus = NONE;
    UINT32 sid = 0;
    string songName = "";
    Category category = UNKNOWN;
    double fileSize = 0;
    double downloaded = 0;
    float percent = 0;
};

struct MyProgress {
    string taskKey = "";
};

namespace DL {
    extern const char *DlTypeName[3];
    extern Lock taskLock;
    extern int downloadType;
    extern bool dontDownload;
    extern map<string, DlInfo> tasks;
    extern int manualDlType;
    extern char manualDlId[0x10];

    CURLcode CurlGetReq(const string url, string &response, const vector<string> extendHeader = vector<string>());
    CURLcode CurlDownload(const string url, const string fileName, MyProgress *prog, const vector<string> extendHeader = vector<string>());
    int SayobotParseInfo(string url, UINT32 &sid, string &songName, int &approved);
    int ParseInfo(string url, UINT32 &sid, string &songName, int &category);
    int SayobotDownload(string fileName, UINT32 sid, string taskKey);
    int Download(string fileName, UINT32 sid, string taskKey);
    int ManualDownload(string id, int idType);
    int RemoveTaskInfo(string url);
    void SetTaskReadLock();
    void SetTaskWriteLock();
    void UnsetTaskLock();
    void StopAllTask();
};
