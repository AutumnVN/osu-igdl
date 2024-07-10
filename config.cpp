#include "config.h"
#include "downloader.h"
#include "overlay.h"
#include <fstream>
#include <iostream>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <sstream>

using namespace std;
using namespace rapidjson;

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
        if (DL::downloadType > 2 || DL::downloadType < 0) DL::downloadType = 2;
    }

    if (d.HasMember("dontDownload")) DL::dontDownload = d["dontDownload"].GetBool();
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
    writer.EndObject();
    result = sb.GetString();
    ofs << result << endl;
    ofs.close();
}
