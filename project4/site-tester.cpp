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


int main(int argc, char *argv[])
{

    // set up exit signal handler
    signal(SIGHUP, exit_func);
    signal(SIGINT, exit_func);

    // load configuration file
    CONFIG = ConfigurationProcessor();
    // will turn into argv[1] eventually
    string configFile;
    if(argc > 1) {
        configFile = string(argv[1]);
        if(!file_exists(configFile.c_str())) {
            cout << "ERROR: Specified configuration file " << configFile << " does not exist... exiting now\n";
            exit(1);
        }
        CONFIG.loadConfig(configFile);
        cout << "INFO: Using specified configuration file " << configFile << "\n";
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

    // setup handler and start alarm
    signal(SIGALRM, start_work);
    alarm(CONFIG.getFetchPeriod());

    while(1) {}

    return 0;
}

void start_work(int x) {

    cout << "INFO: Starting round " << num_cycles << "\n";

    // get time for the output file
    string time = dateAndTime();

    // queue all the sites to be fetched
    for(size_t i = 0; i < sites.size(); i++) {
        fetch_queue.enqueue(sites[i]);
    }

    fetch_threads.resize(CONFIG.getNumFetch());
    active_fetch_threads = 0;

    // have different threads fetch each site
    while(!fetch_queue.empty()) {
        // if we reach max threads, wait for them to finish
        // and start making more
        if(active_fetch_threads == CONFIG.getNumFetch()) {
            while(active_fetch_threads > 0) {
                pthread_join(fetch_threads[active_fetch_threads-1], NULL);
                active_fetch_threads--;
            }
        }
        pthread_create(&fetch_threads[active_fetch_threads], NULL, fetch_site, NULL);

        active_fetch_threads++;
    }

    parse_threads.resize(CONFIG.getNumParse());
    active_parse_threads = 0;

    while(!parse_queue.empty()) {
        // if we reach max threads, wait for them to finish
        // and start making more
        if(active_parse_threads == CONFIG.getNumParse()) {
            while(active_parse_threads > 0) {
                pthread_join(parse_threads[active_parse_threads-1], NULL);
                active_parse_threads--;
            }
        }

        pthread_create(&parse_threads[active_parse_threads], NULL, parse_site, NULL);
        active_parse_threads++;
    }

    // wait till all results are in
    while(results.getSize() != sites.size() * searches.size()) {}

    //string outfile = CONFIG.getOutfileDir() + time;
    string cycle;
    cycle.push_back(num_cycles);
    string outfile = cycle + ".csv";
    ofstream of(outfile.c_str(), ofstream::app);
    of << "Time,Phrase,Site,Count\n";
    while(!results.empty()) {
        // get struct item and output to file
        struct results_item r = results.dequeue();
        of << time << "," << r.search_term << "," << r.site_name << "," << r.count << "\n";
    }
    of.close();
    num_cycles++;

    // get results from results_queue and wait for fetch and parse threads
    // to finish
    alarm(CONFIG.getFetchPeriod());
}

void * fetch_site(void * arg) {
    string s = fetch_queue.dequeue();
    // curl a site
    CURL.getSiteContent(s);
    struct site_info new_site;
    new_site.site_name = s;
    new_site.site_content = CURL.getContent();
    parse_queue.enqueue(new_site);
    return NULL;
}

void * parse_site(void * args) {
    // possibly use Output class to write results upon finding them here
    struct site_info site = parse_queue.dequeue();
    for(size_t i = 0; i < searches.size(); i++) {
        int count;
        count = countTerms(site.site_content, searches[i]);
        struct results_item tmp;
        tmp.site_name = site.site_name;
        tmp.search_term = searches[i];
        tmp.count = count;
        results.enqueue(tmp);
    }
    return NULL;
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

int countTerms(string content, string term) {
    // counts the number of occurrences of term
    // in content
    int count = 0;
    if (!content.empty())
    {
        // do stuff
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
    cout << "\nexiting...\n";
    int i;
    for(i = 0; i < CONFIG.getNumFetch(); i++) {
        pthread_join(fetch_threads[i], NULL);
    }
    for(i = 0; i < CONFIG.getNumParse(); i++) {
        pthread_join(parse_threads[i], NULL);
    }
    exit(0);
}

bool file_exists(const char* filename) {
    struct stat buf;
    if(stat(filename, &buf) == 0) return true;
    return false;
}