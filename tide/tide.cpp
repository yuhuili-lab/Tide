#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <vector>
#include <set>
#include <map>
#include <deque>
#include <stack>
#include <queue>
#include <algorithm>
#include <regex>
#include <curl/curl.h>
using namespace std;

string getHighestResm3u8(ifstream &ifs);
vector<string> processm3u8(ifstream &ifs);
string random_string(size_t length);
string createtmp();
bool download(string remotePath, string localPath);
bool is_file_exist(string fileName);

int main(int argc, char *argv[]) {
    srand (time(NULL));
    
    string masterm3u8Remote = argv[1];
    string masterm3u8Local = createtmp();
    download(masterm3u8Remote, masterm3u8Local);
    string relativeRemotePath = masterm3u8Remote.substr(0, masterm3u8Remote.find_last_of("\\/")+1);
    
    ifstream masterm3u8File(masterm3u8Local);
    string highestResRemote = getHighestResm3u8(masterm3u8File);
    string highestResLocal = createtmp();
    download(relativeRemotePath+highestResRemote, highestResLocal);
    
    return 0;
    
    cout << highestResRemote << endl;
    
    
    /*
    vector<string> filePaths = processm3u8(m3u8f);
    for (auto &a : filePaths) {
        cout << a << endl;
    }*/
}

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

bool download(string remotePath, string localPath) {
    CURL *curl;
    FILE *fp;
    CURLcode res;
    const char *url = remotePath.c_str();
    char outfilename[1024];//as 1 char space for null is also required
    strcpy(outfilename, localPath.c_str());
    curl = curl_easy_init();
    if (curl) {
        fp = fopen(outfilename,"wb");
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        res = curl_easy_perform(curl);
        /* always cleanup */
        curl_easy_cleanup(curl);
        fclose(fp);
    }
    return true;
}

bool is_file_exist(string fileName) {
    std::ifstream infile(fileName);
    return infile.good();
}

string createtmp() {
    while (true) {
        string randStr = random_string(10);
        if (!is_file_exist(randStr)) return randStr;
    }
    return "";
}

string getHighestResm3u8(ifstream &ifs) {
    
    string line, mediaFilePath = "";
    bool hasPlaylist = false;
    int averageBandwidth = -1;
    
    while (ifs >> line) {
        regex r("\\#EXT\\-X\\-STREAM\\-INF\\:AVERAGE\\-BANDWIDTH\\=(\\d+)");
        smatch m;
        regex_search(line, m, r);
        if (m.size() > 0) {
            int newBandwidthStr = stoi(m[1]);
            if (newBandwidthStr > averageBandwidth) {
                averageBandwidth = newBandwidthStr;
                ifs >> mediaFilePath;
            }
        }
    }
    
    return mediaFilePath;
}

vector<string> processm3u8(ifstream &ifs) {
    vector<string> res;
    string line, mediaFilePath;
    
    while (ifs >> line) {
        if (line.find("#EXTINF:") != 0) continue;
        ifs >> mediaFilePath;
        res.push_back(mediaFilePath);
    }
    
    return res;
}

string random_string(size_t length) {
    auto randchar = []() -> char
    {
        const char charset[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
        const size_t max_index = (sizeof(charset) - 1);
        return charset[rand() % max_index];
    };
    string str(length,0);
    generate_n(str.begin(), length, randchar);
    return str;
}
