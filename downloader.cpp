#include "Downloader.h"
#include "logger.h"
#include "utils.h"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <time.h>

using namespace rapidjson;

const char *DL::DlTypeName[3] = {"Full Version", "No Video", "Mini"};
Lock DL::taskLock;
int DL::downloadType = MINI;
bool DL::dontDownload = false;
map<string, DlInfo> DL::tasks;
int DL::manualDlType = 0;
char DL::manualDlId[0x10] = "";

// xferinfo callback function
// write out real time information to struct
int xferinfoCB(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    MyProgress *myp = (struct MyProgress *)clientp;
    DL::SetTaskReadLock();

    if (DL::tasks.count(myp->taskKey) <= 0) {
        DL::UnsetTaskLock();
        return 1;
    }

    DL::UnsetTaskLock();
    DL::SetTaskWriteLock();
    DL::tasks[myp->taskKey].fileSize = (double)dltotal;
    DL::tasks[myp->taskKey].downloaded = (double)dlnow;

    if (!dltotal && !dlnow) {
        DL::tasks[myp->taskKey].percent = 0;
    } else {
        DL::tasks[myp->taskKey].percent = (float)((double)dlnow / (double)dltotal);
    }

    DL::UnsetTaskLock();
    return 0;
}

// GET requests, return string
CURLcode DL::CurlGetReq(const string url, string &response, const vector<string> extendHeader) {
    CURL *curl = curl_easy_init();
    CURLcode res = CURL_LAST;
    if (curl) {
        struct curl_slist *header_list = NULL;

        for (int i = 0; i < extendHeader.size(); i++) {
            header_list = curl_slist_append(header_list, extendHeader[i].c_str());
        }

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, stringWriter);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 8000);

        res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            long response_code;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            if (response_code != 200) {
                logger::WriteLogFormat("[-] CurlGetReq: response_code %ld", response_code);
                res = CURL_LAST;
            }
        }
    }
    curl_easy_cleanup(curl);
    return res;
}

// GET requests, write output to file
CURLcode DL::CurlDownload(const string url, const string fileName, MyProgress *prog, const vector<string> extendHeader) {
    CURL *curl = curl_easy_init();
    CURLcode res = CURL_LAST;
    FILE *fp;
    if (curl) {
        struct curl_slist *header_list = NULL;

        for (int i = 0; i < extendHeader.size(); i++) {
            header_list = curl_slist_append(header_list, extendHeader[i].c_str());
        }

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fileWriter);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, xferinfoCB);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, prog);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 8000);

        auto err = fopen_s(&fp, fileName.c_str(), "wb");

        if (fp) {
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
            res = curl_easy_perform(curl);
            fclose(fp);
        }

        if (res == CURLE_OK) {
            long response_code;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            if (response_code != 200) {
                logger::WriteLogFormat("[-] CurlDownload: response_code %ld", response_code);
                res = CURL_LAST;
            }
        }
    }
    curl_easy_cleanup(curl);
    return res;
}

// use sayobot api to parse sid,song name,category by osu beatmap url
int DL::SayobotParseInfo(string url, UINT32 &sid, string &songName, int &category) {
    string parseApiUrl = "https://api.sayobot.cn/v2/beatmapinfo?0=" + url;
    string content;
    Document jContent;
    int status;

    auto res = CurlGetReq(parseApiUrl, content);
    if (res) {
        logger::WriteLogFormat("[-] SayobotParseInfo: can't get json content, %s", curl_easy_strerror(res));
        return 1;
    }

    string utf8Content = GB2312toUTF8(content.c_str());
    jContent.Parse(utf8Content.c_str());

    if (jContent.HasParseError()) {
        logger::WriteLogFormat("[-] SayobotParseInfo: unknown parsing error");
        return 6;
    }

    if (!jContent.HasMember("status")) {
        logger::WriteLogFormat("[-] SayobotParseInfo: Wrong json format: doesn't contain member 'status'");
        return 2;
    }

    status = jContent["status"].GetInt();
    if (status) {
        logger::WriteLogFormat("[-] SayobotParseInfo: Sayobot err-code: %d", status);
        return 3;
    }

    if (!jContent.HasMember("data")) {
        logger::WriteLogFormat("[-] SayobotParseInfo: Wrong json format: doesn't contain member 'data'");
        return 4;
    }

    if (!jContent["data"].HasMember("sid") || !jContent["data"].HasMember("title") || !jContent["data"].HasMember("approved")) {
        logger::WriteLogFormat("[-] SayobotParseInfo: Wrong json format: doesn't contain member 'sid' or 'title' or 'approved'");
        return 5;
    }

    sid = jContent["data"]["sid"].GetUint();
    songName = jContent["data"]["title"].GetString();
    category = jContent["data"]["approved"].GetInt();
    return 0;
}

int DL::ParseInfo(string url, UINT32 &sid, string &songName, int &category) {
    logger::WriteLogFormat("[*] parsing %s", url.c_str());
    return SayobotParseInfo(url, sid, songName, category);
}

// download beatmap from sayobot server to file by using sid
int DL::SayobotDownload(string fileName, UINT32 sid, string taskKey) {
    string downloadApiUrl;

    switch (DL::downloadType) {
        case FULL:
            downloadApiUrl = "https://txy1.sayobot.cn/beatmaps/download/full/" + to_string(sid);
            break;
        case NOVIDEO:
            downloadApiUrl = "https://txy1.sayobot.cn/beatmaps/download/novideo/" + to_string(sid);
            break;
        case MINI:
        default:
            downloadApiUrl = "https://txy1.sayobot.cn/beatmaps/download/mini/" + to_string(sid);
            break;
    }

    MyProgress *myp = new MyProgress();
    myp->taskKey = taskKey;
    auto res = DL::CurlDownload(downloadApiUrl, fileName, myp);
    if (res) {
        logger::WriteLogFormat("[-] SayobotDownload: err while downloading, %s", curl_easy_strerror(res));
        delete myp;
        return 2;
    }

    delete myp;
    return 0;
}

int DL::Download(string fileName, UINT32 sid, string taskKey) {
    logger::WriteLogFormat("[*] downloading sid %lu", sid);
    return SayobotDownload(fileName, sid, taskKey);
}

int DL::ManualDownload(string id, int idType) {
    if (id.empty()) return 0;

    string url = idType ? "https://osu.ppy.sh/b/" + id : "https://osu.ppy.sh/s/" + id;
    LPCWSTR w_url = char2wchar(url.c_str());
    ShellExecute(0, 0, w_url, 0, 0, SW_HIDE);
    delete w_url;
    return 0;
}

int DL::RemoveTaskInfo(string url) {
    SetTaskWriteLock();
    tasks.erase(url);
    UnsetTaskLock();
    return 0;
}

void DL::StopAllTask() {
    SetTaskWriteLock();
    Sleep(1);
    tasks.clear();
    Sleep(2);
    UnsetTaskLock();
}

void DL::SetTaskReadLock() {
    taskLock.ReadLock();
}

void DL::SetTaskWriteLock() {
    taskLock.WriteLock();
}

void DL::UnsetTaskLock() {
    taskLock.Unlock();
}
