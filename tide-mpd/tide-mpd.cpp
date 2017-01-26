#include <iostream>
#include <string>
#include <fstream>
#include <regex>
#include <stdio.h>
#include <curl/curl.h>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>

using namespace std;
using namespace xercesc;

string getHighestResm3u8(ifstream &ifs);
vector<string> processm3u8(ifstream &ifs);
void processmpdFile(string filename, int &timescale, int &duration, string &videoMediaFile, string &videoInitFile, string &audioMediaFile, string &audioInitFile, int &totalseconds);

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
    
    string mpdRemote = argv[1];
    string mpdLocal = create_tmp();
    download(mpdRemote, mpdLocal);
    string relativeRemotePath = mpdRemote.substr(0, mpdRemote.find_last_of("\\/")+1);
    
    // Process mpd file
    int timescale, duration, totalseconds;
    string videoMediaFileRemote, videoInitFileRemote, audioMediaFileRemote, audioInitFileRemote;
    processmpdFile(mpdLocal, timescale, duration, videoMediaFileRemote, videoInitFileRemote, audioMediaFileRemote, audioInitFileRemote, totalseconds);
    
    float clipLength = (float)duration / (float)timescale;
    int numberOfClips = ceil((float)totalseconds / (float)clipLength);
    
    
    
    for (int i=0; i<numberOfClips; ++i) {
        cout << "                                                       \r";
        cout.flush();
        cout << "Downloading " << i+1 << "/" << numberOfClips << " video clips..." << endl;
        
        regex r("\\$Number\\$");
        string newRemoteURL = regex_replace(videoMediaFileRemote, r, to_string(i));
        download(relativeRemotePath+newRemoteURL, "video/"+to_string(i)+".m4s");
    }
    
    for (int i=0; i<numberOfClips; ++i) {
        cout << "                                                       \r";
        cout.flush();
        cout << "Downloading " << i+1 << "/" << numberOfClips << " audio clips..." << endl;
        
        regex r("\\$Number\\$");
        string newRemoteURL = regex_replace(audioMediaFileRemote, r, to_string(i));
        download(relativeRemotePath+newRemoteURL, "audio/"+to_string(i)+".m4s");
    }
    
    download(relativeRemotePath+videoInitFileRemote, "video/init.m4s");
    download(relativeRemotePath+audioInitFileRemote, "audio/init.m4s");
    
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
        fp = fopen(outfilename,"wb");
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

// http://stackoverflow.com/questions/3074776/how-to-convert-char-array-to-wchar-t-array
static const wchar_t* charToWChar(const char* text)
{
    const size_t size = strlen(text) + 1;
    wchar_t* wText = new wchar_t[size];
    mbstowcs(wText, text, size);
    return wText;
}

void processmpdFile(string filename, int &timescale, int &duration, string &videoMediaFile, string &videoInitFile, string &audioMediaFile, string &audioInitFile, int &totalseconds) {
    vector<string> v;
    try {
        XMLPlatformUtils::Initialize();
    } catch (const XMLException& toCatch) {
        char* message = XMLString::transcode(toCatch.getMessage());
        cout << "Error during initialization! :\n"
        << message << "\n";
        XMLString::release(&message);
        return;
    }
    
    XercesDOMParser *parser = new XercesDOMParser();
    //parser->setValidationScheme(XercesDOMParser::Val_Always);
    parser->setValidationScheme( XercesDOMParser::Val_Never );
    parser->setDoNamespaces( false );
    parser->setDoSchema( false );
    parser->setLoadExternalDTD( false );

    
    ErrorHandler *errHandler = (ErrorHandler *) new HandlerBase();
    parser->setErrorHandler(errHandler);
    
    const char *xmlFile = filename.c_str();
    
    try {
        parser->parse(xmlFile);
    }
    catch (const XMLException& toCatch) {
        char *message = XMLString::transcode(toCatch.getMessage());
        cout << "Exception message is: \n"
        << message << "\n";
        XMLString::release(&message);
        return;
    }
    catch (const DOMException& toCatch) {
        char *message = XMLString::transcode(toCatch.msg);
        cout << "Exception message is: \n"
        << message << "\n";
        XMLString::release(&message);
        return;
    }
    catch (...) {
        cout << "Unexpected Exception \n" ;
        return;
    }
    
    DOMNode* docRootNode;
    DOMDocument* doc;
    doc = parser->getDocument();
    docRootNode = doc->getDocumentElement();
    
    cout << XMLString::transcode(docRootNode->getAttributes()->getNamedItem(XMLString::transcode("mediaPresentationDuration"))->getNodeValue()) << endl;
    
    string rawTotalDuration = XMLString::transcode(docRootNode->getAttributes()->getNamedItem(XMLString::transcode("mediaPresentationDuration"))->getNodeValue());
    regex r("PT(\\d*)H?(\\d*)M?(\\d*)S");
    smatch m;
    regex_search(rawTotalDuration, m, r);
    
    totalseconds = 0;
    totalseconds += stoi(m[m.size()-1]);
    if (m.size()>2) {
        totalseconds += stoi(m[m.size()-2])*60;
    }
    if (m.size()>3) {
        totalseconds += stoi(m[m.size()-3])*3600;
    }
    
    
    // Get Video Segment tag
    DOMNode *videoSegmentTemplate = docRootNode->getChildNodes()->item(1)->getChildNodes()->item(1)->getChildNodes()->item(1)->getChildNodes()->item(1);
    
    DOMNamedNodeMap *attrr =videoSegmentTemplate->getAttributes();
    
    // Timescale
    timescale = XMLString::parseInt(attrr->getNamedItem(XMLString::transcode("timescale"))->getNodeValue());
    
    // Duration
    duration = XMLString::parseInt(attrr->getNamedItem(XMLString::transcode("duration"))->getNodeValue());
    
    // Media
    videoMediaFile = XMLString::transcode(attrr->getNamedItem(XMLString::transcode("media"))->getNodeValue());
    
    // Initialization
    videoInitFile = XMLString::transcode(attrr->getNamedItem(XMLString::transcode("initialization"))->getNodeValue());
    
    
    
    // Get Audio Segment tag
    DOMNode *audioSegmentTemplate = docRootNode->getChildNodes()->item(1)->getChildNodes()->item(3)->getChildNodes()->item(1)->getChildNodes()->item(1);
    
    DOMNamedNodeMap *attr =audioSegmentTemplate->getAttributes();
    
    // Media
    audioMediaFile = XMLString::transcode(attr->getNamedItem(XMLString::transcode("media"))->getNodeValue());
    
    // Initialization
    audioInitFile = XMLString::transcode(attr->getNamedItem(XMLString::transcode("initialization"))->getNodeValue());
    
     
     
    /*
    DOMNodeList *nodes = docRootNode->getChildNodes();
    DOMNode *n = nodes->item(1);
    
    cout << n->getChildNodes()->getLength() << endl;
    cout << XMLString::transcode(n->getNodeName()) << endl;
    
    //DOMNamedNodeMap
    
    //cout << XMLString::transcode(docRootNode->getTextContent()) << endl;
    //cout << docRootNode->getChildNodes()->getLength() << endl;
    
    //cout << XMLString::transcode(docRootNode->getNodeValue()) << endl;
    */
    /*
    tinyxml2::XMLDocument doc;
    doc.LoadFile("my.mpd");
    //if(doc.Error()) cout << "err";
    tinyxml2::XMLElement *element = doc.FirstChildElement("MPD");
    if (element == nullptr) {
        cout << "k" << endl;
        return vector<string>();
    }
    cout << doc.FirstChildElement("MPD")->GetText();
   // string urlFormat =  doc.FirstChild()->ToElement()->Name();
//    string urlFormat = doc.FirstChildElement("MPD")->FirstChildElement("Period")->FirstChildElement("AdaptationSet")->FirstChildElement("Representation")->FirstChildElement("SegmentTemplate")->Attribute("media");
    //cout << urlFormat;*/
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

