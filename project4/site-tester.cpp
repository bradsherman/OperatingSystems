#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "ConfigurationProcessor.h"
#include "CurlSite.h"
#include "ConcurrentQueue.h"
#include "HTML.h"

using namespace std;

vector<string> fileToVector(string);
void printVec(vector<string>);
void start_work(int);
void * fetch_site(void *);
void * parse_site(void *);
int countTerms(string content, string term);
string dateAndTime();
void exit_func(int);
bool file_exists(const char *);
void create_fetch_threads();
void create_parse_threads();

struct site_info {
    string site_name;
    string site_content;
};

struct results_item {
    string site_name;
    string search_term;
    int count;
};

// Globals
ConfigurationProcessor CONFIG;
vector<string> sites;
vector<string> searches;
ConcurrentQueue<string> fetch_queue;
int active_fetch_threads;
ConcurrentQueue<struct site_info> parse_queue;
int active_parse_threads;
ConcurrentQueue<struct results_item> results;
char num_cycles = '1';
CurlSite CURL;
vector<pthread_t> fetch_threads;
vector<pthread_t> parse_threads;
int results_expected;
int results_created;


int main(int argc, char *argv[])
{

    // set up exit signal handler
    signal(SIGHUP, exit_func);
    signal(SIGINT, exit_func);

    // load configuration file
    CONFIG = ConfigurationProcessor();
    string configFile;
    // get config file
    if(argc > 1) {
        configFile = string(argv[1]);
        if(!file_exists(configFile.c_str())) {
            cout << "ERROR: Specified configuration file: " << configFile << " does not exist... exiting now\n";
            exit(1);
        }
        CONFIG.loadConfig(configFile);
        cout << "INFO: Using specified configuration file: " << configFile << "\n";
    } else {
        cout << "INFO: Using default configuration settings\n";
    }
    if(!file_exists(CONFIG.getSiteFile().c_str())) {
        cout << "ERROR: Site file: " << CONFIG.getSiteFile() << " not found... exiting now\n";
        exit(1);
    }
    if(!file_exists(CONFIG.getSearchFile().c_str())) {
        cout << "ERROR: Search file: " << CONFIG.getSearchFile() << " not found... exiting now\n";
        exit(1);
    }
    cout << "\nINFO: Current Configuration\n";
    CONFIG.printConfig();
    cout << "\n";

    // populate vectors of sites/words
    sites = fileToVector(CONFIG.getSiteFile());
    searches = fileToVector(CONFIG.getSearchFile());
    cout << "Websites:\n";
    printVec(sites);
    cout << "\n";
    cout << "Search Terms:\n";
    printVec(searches);
    cout << "\n";
    CURL = CurlSite();
    create_fetch_threads();
    create_parse_threads();

    // setup handler and start alarm
    signal(SIGALRM, start_work);
    start_work(1);

    while(1) {}

    return 0;
}

void start_work(int x) {

    cout << "INFO: Starting round " << num_cycles << "\n";
    // start results_expected at the max and we will decrement
    // it if an error occurs in the fetch thread
    results_expected = sites.size() * searches.size();
    results_created = 0;

    // get time for the output file
    string time = dateAndTime();

    // queue all the sites to be fetched
    for(size_t i = 0; i < sites.size(); i++) {
        fetch_queue.enqueue(sites[i]);
    }

    // wait for all results to be in

    // setup output file
    string cycle;
    cycle.push_back(num_cycles);
    string outfile = cycle + ".csv";
    ofstream of(outfile.c_str(), ofstream::out);
    of << "Time,Phrase,Site,Count\n";
    while(results_created != results_expected) {
        // get struct item and write to file
        struct results_item r = results.dequeue();
        of << time << "," << r.search_term << "," << r.site_name << "," << r.count << "\n";
    }
    of.close();
    cout << "INFO: Writing results to \"" << outfile << "\"\n";
    num_cycles++;
    alarm(CONFIG.getFetchPeriod());
    cout << "\n";
}

void * fetch_site(void * arg) {
    while(!fetch_queue.stop) {
        string s = fetch_queue.dequeue();
        // queue returns empty string on exit
        if(s == "") continue;
        // curl a site
        cout << "fetching " << s << "\n";
        CURL.getSiteContent(s);
        struct site_info new_site;
        new_site.site_name = s;
        new_site.site_content = CURL.getContent();
        // only add result if we got one
        // content is "" on error
        if(new_site.site_content != "")
            parse_queue.enqueue(new_site);
        else {
            // decrement the number of expected results
            // if we don't queue a site
            results_expected -= searches.size();
        }
    }
    pthread_exit(NULL);
}

void * parse_site(void * args) {
    while(!parse_queue.stop) {
        struct site_info site = parse_queue.dequeue();
        // queue returns empty struct on exit
        if(site.site_name == "") continue;
        cout << "parsing " << site.site_name << "\n";
        for(size_t i = 0; i < searches.size(); i++) {
            int count;
            count = countTerms(site.site_content, searches[i]);
            struct results_item tmp;
            tmp.site_name = site.site_name;
            tmp.search_term = searches[i];
            tmp.count = count;
            results.enqueue(tmp);
            results_created++;
        }
    }
    pthread_exit(NULL);
}

vector<string> fileToVector(string filename) {
    vector<string> ret;

    ifstream infile(filename.c_str());
    string line;
    while(getline(infile, line)) {
        ret.push_back(line);
    }
    return ret;
}

void printVec(vector<string> vec) {
    for(size_t i = 0; i < vec.size(); i++) {
        cout << vec[i] << endl;
    }
}

string getVisibleText(string html) {
    // return html aware portion of the string
    size_t pos = 0;
    size_t pos2 = 0;
    pos = html.find("<body", pos);
    if(pos == string::npos) return html;
    pos2 = html.find("</body", pos);
    string visibleText = html.substr(pos, pos2);
    return visibleText;
}

int countTerms(string content, string term) {
    // counts the number of occurrences of term
    // in content
    int count = 0;
    content = getVisibleText(content);
    if (!content.empty())
    {
        size_t pos = 0;
        while((pos = content.find(term, pos)) != string::npos) {
            pos = pos + term.size();
            count++;
        }
    }

    return count;
}

string dateAndTime() {
    time_t timer;
    struct tm * t;
    time(&timer);
    t = localtime(&timer);
    char buf[80];
    strftime(buf, 80, "%m-%d-%C-%T", t);
    return string(buf);
}

void exit_func(int x) {
    // cancel alarm
    alarm(0);
    cout << "\nexiting...\n";
    fetch_queue.stopQueue();
    parse_queue.stopQueue();
    results.stopQueue();
    HTML html = HTML();
    html.writeHTMLPage(sites, num_cycles);
    exit(0);
}

bool file_exists(const char* filename) {
    struct stat buf;
    if(stat(filename, &buf) == 0) return true;
    return false;
}

void create_fetch_threads() {
    int i, r;
    fetch_threads.resize(CONFIG.getNumFetch());
    for(i = 0; i < CONFIG.getNumFetch(); i++) {
        r = pthread_create(&fetch_threads[i], NULL, fetch_site, NULL);
        if(r < 0) {
            cout << "ERROR: Could not create fetch thread\n";
        }
    }
}

void create_parse_threads() {
    int i, r;
    parse_threads.resize(CONFIG.getNumParse());
    for(i = 0; i < CONFIG.getNumParse(); i++) {
        r = pthread_create(&parse_threads[active_parse_threads], NULL, parse_site, NULL);
        if(r < 0) {
            cout << "ERROR: Could not create parse thread\n";
        }
    }
}