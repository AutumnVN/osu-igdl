#include "config.h"
#include "downloader.h"
#include "overlay.h"
#include <fstream>
#include <iostream>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <sstream>
#include <windows.h>

using namespace std;
using namespace rapidjson;

char Config::tosuPath[MAX_PATH] = "";

void Config::LoadConfig() {
    ifstream ifs("igdl.cfg");
    string result;
    Document d;

    if (!ifs.is_open()) return;

    ifs >> result;
    ifs.close();
    d.Parse(result.c_str());

    if (d.HasMember("downloadType")) {
        DL::downloadType = d["downloadType"].GetInt();
        if (DL::downloadType > 1 || DL::downloadType < 0) DL::downloadType = 1;
    }

    if (d.HasMember("dontDownload")) DL::dontDownload = d["dontDownload"].GetBool();

    if (d.HasMember("tosuPath")) strcpy_s(tosuPath, d["tosuPath"].GetString());
}

void Config::SaveConfig() {
    ofstream ofs("igdl.cfg");
    StringBuffer sb;
    Writer<StringBuffer> writer(sb);
    string result;

    if (!ofs.is_open()) return;

    writer.StartObject();
    writer.Key("downloadType");
    writer.Int(DL::downloadType);
    writer.Key("dontDownload");
    writer.Bool(DL::dontDownload);
    writer.Key("tosuPath");
    writer.String(tosuPath);
    writer.EndObject();
    result = sb.GetString();
    ofs << result << endl;
    ofs.close();
}
