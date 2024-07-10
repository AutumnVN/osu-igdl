#include "Downloader.h"
#include "downloader.h"
#include "logger.h"
#include "utils.h"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <regex>
#include <time.h>

using namespace rapidjson;

const char *DL::DlTypeName[2] = {"Full", "No Video"};
Lock DL::taskLock;
int DL::downloadType = NOVIDEO;
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

// use catboy api to parse sid,song name,category by osu beatmap url
int DL::CatboyParseInfo(string url, UINT32 &sid, string &artist, string &songName, string &category) {
    string parseApiUrl;
    string content;
    Document jContent;
    int status;

    vector<regex> e;
    e.push_back(regex("osu.ppy.sh/b/(\\d{1,})"));
    e.push_back(regex("osu.ppy.sh/s/(\\d{1,})"));
    e.push_back(regex("osu.ppy.sh/beatmapsets/(\\d{1,})"));
    e.push_back(regex("osu.ppy.sh/beatmaps/(\\d{1,})"));
    smatch m;
    bool found = false;
    int setid = 0, bid = 0;

    for (int i = 0; i < 4; i++) {
        found = regex_search(url, m, e[i]);

        if (found) {
            if (i == 0 || i == 3) {
                bid = atoi(m.str(1).c_str());
            } else {
                setid = atoi(m.str(1).c_str());
            }
            break;
        }
    }

    if (bid != 0) {
        parseApiUrl = "https://catboy.best/api/v2/b/" + to_string(bid);
    }

    if (setid != 0) {
        parseApiUrl = "https://catboy.best/api/v2/s/" + to_string(setid);
    }

    auto res = CurlGetReq(parseApiUrl, content);
    if (res) {
        logger::WriteLogFormat("[-] CatboyParseInfo: can't get json content, %s", curl_easy_strerror(res));
        return 1;
    }

    string utf8Content = GB2312toUTF8(content.c_str());
    jContent.Parse(utf8Content.c_str());

    if (jContent.HasParseError()) {
        logger::WriteLogFormat("[-] CatboyParseInfo: unknown parsing error");
        return 6;
    }

    if (jContent.HasMember("set")) {
        jContent["set"].Swap(jContent);
    }

    if (!jContent.HasMember("id") || !jContent.HasMember("artist") || !jContent.HasMember("title") || !jContent.HasMember("status")) {
        logger::WriteLogFormat("[-] CatboyParseInfo: Wrong json format: doesn't contain member 'id' or 'artist' or 'title' or 'status'");
        return 2;
    }

    sid = jContent["id"].GetUint();
    artist = jContent["artist"].GetString();
    songName = jContent["title"].GetString();
    category = jContent["status"].GetString();
    category[0] = toupper(category[0]);
    return 0;
}

int DL::ParseInfo(string url, UINT32 &sid, string &artist, string &songName, string &category) {
    logger::WriteLogFormat("[*] parsing %s", url.c_str());
    return CatboyParseInfo(url, sid, artist, songName, category);
}

// download beatmap from catboy server to file by using sid
int DL::CatboyDownload(string fileName, UINT32 sid, string taskKey) {
    string downloadApiUrl;

    switch (DL::downloadType) {
        case FULL:
            downloadApiUrl = "https://catboy.best/d/" + to_string(sid);
            break;
        default:
        case NOVIDEO:
            downloadApiUrl = "https://catboy.best/d/" + to_string(sid) + "n";
            break;
    }

    MyProgress *myp = new MyProgress();
    myp->taskKey = taskKey;
    auto res = DL::CurlDownload(downloadApiUrl, fileName, myp);
    if (res) {
        logger::WriteLogFormat("[-] CatboyDownload: err while downloading, %s", curl_easy_strerror(res));
        delete myp;
        return 2;
    }

    delete myp;
    return 0;
}

int DL::Download(string fileName, UINT32 sid, string taskKey) {
    logger::WriteLogFormat("[*] downloading sid %lu", sid);
    return CatboyDownload(fileName, sid, taskKey);
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
