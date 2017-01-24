#include <iostream>
#include <string>
#include <fstream>
#include <regex>
#include <curl/curl.h>
using namespace std;

string getHighestResm3u8(ifstream &ifs);
vector<string> processm3u8(ifstream &ifs);

string random_string(size_t length);
string create_tmp();
bool download(string remotePath, string localPath);
bool is_file_exist(string fileName);
int progress_func(void* ptr, double TotalToDownload, double NowDownloaded,
                  double TotalToUpload, double NowUploaded);
int round_int(double r);

int main(int argc, char *argv[]) {
    srand (time(NULL));
    
    if (argc!=2 && argc!=3) {
        cerr << "Usage: ./tide url [output_name]" << endl;
        return 1;
    }
    
    if (argc == 3) {
        if (is_file_exist(argv[2])) {
            cerr << "File exists!" << endl;
            return 2;
        }
    }
    
    // Get master file supplied by command line argument
    string masterRemote = argv[1];
    string masterLocal = create_tmp();
    download(masterRemote, masterLocal);
    string relativeRemotePath = masterRemote.substr(0, masterRemote.find_last_of("\\/")+1);
    ifstream masterFile(masterLocal);
    
    // Get highest resolution file
    string highestResRemote = relativeRemotePath+getHighestResm3u8(masterFile);
    string highestResLocal = create_tmp();
    download(highestResRemote, highestResLocal);
    relativeRemotePath = highestResRemote.substr(0, highestResRemote.find_last_of("\\/")+1);
    ifstream highestResFile(highestResLocal);
    
    // Process video segments
    string videoLocal = argc==3 ? string(argv[2])+".ts" : create_tmp()+".ts";
    vector<string> videoSegmentsRemote = processm3u8(highestResFile);
    
    int index = 1;
    int total = videoSegmentsRemote.size();
    
    // Sequentially download all video segments
    for (auto &a : videoSegmentsRemote) {
        cout << "                                                       \r";
        cout.flush();
        cout << "Downloading " << index << "/" << total << " clips..." << endl;
        download(relativeRemotePath+a, videoLocal);
        ++index;
    }
    
    // Remove temp files
    remove(masterLocal.c_str());
    remove(highestResLocal.c_str());
}

// cURL download
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
        fp = fopen(outfilename,"ab");
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, false);
        curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_func);
        res = curl_easy_perform(curl);
        /* always cleanup */
        curl_easy_cleanup(curl);
        fclose(fp);
    }
    return true;
}

// Obtain the version of m3u8 playlist with the highest resolution.
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

// Obtain each individual file presented by m3u8 playlist.
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

// File manipulations
bool is_file_exist(string fileName) {
    std::ifstream infile(fileName);
    return infile.good();
}

string create_tmp() {
    while (true) {
        string randStr = random_string(10);
        if (!is_file_exist(randStr)) return randStr;
    }
    return "";
}

// Generate random string for tmp file
// http://stackoverflow.com/questions/440133/how-do-i-create-a-random-alpha-numeric-string-in-c
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

// http://stackoverflow.com/questions/1637587/c-libcurl-console-progress-bar
int progress_func(void* ptr, double TotalToDownload, double NowDownloaded,
                  double TotalToUpload, double NowUploaded) {
    // ensure that the file to be downloaded is not empty
    // because that would cause a division by zero error later on
    if (TotalToDownload <= 0.0) {
        return 0;
    }
    
    // how wide you want the progress meter to be
    int totaldotz=40;
    double fractiondownloaded = NowDownloaded / TotalToDownload;
    // part of the progressmeter that's already "full"
    int dotz = round_int(fractiondownloaded * totaldotz);
    
    // create the "meter"
    int ii=0;
    printf("%3.0f%% [",fractiondownloaded*100);
    // part  that's full already
    for ( ; ii < dotz;ii++) {
        printf("=");
    }
    // remaining part (spaces)
    for ( ; ii < totaldotz;ii++) {
        printf(" ");
    }
    // and back to line begin - do not forget the fflush to avoid output buffering problems!
    printf("]\r");
    fflush(stdout);
    // if you don't return 0, the transfer will be aborted - see the documentation
    return 0;
}

// http://stackoverflow.com/questions/485525/round-for-float-in-c
int round_int(double r) {
    return (r > 0.0) ? (r + 0.5) : (r - 0.5);
}
