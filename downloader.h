#pragma once
#include "rw_lock.h"
#include <curl/curl.h>
#include <iostream>
#include <map>
#include <vector>

using namespace std;

enum DlType {
    FULL,
    NOVIDEO
};

enum DlStatus {
    NONE,
    PARSE,
    DOWNLOAD
};

struct DlInfo {
    DlStatus dlStatus = NONE;
    UINT32 sid = 0;
    string artist = "";
    string songName = "";
    string category = "";
    double fileSize = 0;
    double downloaded = 0;
    float percent = 0;
};

struct MyProgress {
    string taskKey = "";
};

namespace DL {
    extern const char *DlTypeName[2];
    extern Lock taskLock;
    extern int downloadType;
    extern bool dontDownload;
    extern map<string, DlInfo> tasks;
    extern int manualDlType;
    extern char manualDlId[0x10];

    CURLcode CurlGetReq(const string url, string &response, const vector<string> extendHeader = vector<string>());
    CURLcode CurlDownload(const string url, const string fileName, MyProgress *prog, const vector<string> extendHeader = vector<string>());
    int CatboyParseInfo(string url, UINT32 &sid, string &artist, string &songName, string &category);
    int ParseInfo(string url, UINT32 &sid, string &artist, string &songName, string &category);
    int CatboyDownload(string fileName, UINT32 sid, string taskKey);
    int Download(string fileName, UINT32 sid, string taskKey);
    int ManualDownload(string id, int idType);
    int RemoveTaskInfo(string url);
    void SetTaskReadLock();
    void SetTaskWriteLock();
    void UnsetTaskLock();
    void StopAllTask();
};
