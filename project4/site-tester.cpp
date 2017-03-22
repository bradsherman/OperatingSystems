#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <cstdlib>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
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
string currentTime();
void exit_func(int) {
    cout << "exiting...\n";
    exit(0);
}

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

int main(int argc, char *argv[])
{

    // set up exit signal handler
    signal(SIGHUP, exit_func);
    signal(SIGINT, exit_func);

    // load configuration file
    CONFIG = ConfigurationProcessor();
    // will turn into argv[1] eventually
    CONFIG.loadConfig("files/Config.txt");
    CONFIG.printConfig();

    // populate vectors of sites/words
    sites = fileToVector(CONFIG.getSiteFile());
    searches = fileToVector(CONFIG.getSearchFile());
    printVec(sites);
    //printVec(searches);

    // setup handler and start alarm
    signal(SIGALRM, start_work);
    alarm(CONFIG.getFetchPeriod());

    while(1) {}

    return 0;
}

void start_work(int x) {

    // get time for the output file
    //string time = currentTime();
    string time = "blah";

    // queue all the sites to be fetched
    for(size_t i = 0; i < sites.size(); i++) {
        fetch_queue.enqueue(sites[i]);
    }

    vector<pthread_t> fetch_threads(CONFIG.getNumFetch());
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

    vector<pthread_t> parse_threads(CONFIG.getNumParse());
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
    ofstream of(outfile);
    of << "Time,Phrase,Site,Count\n";
    while(!results.empty()) {
        // get struct item and output to file
        struct results_item r = results.dequeue();
        of << "DATE," << r.search_term << "," << r.site_name << "," << r.count << "\n";
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
    CurlSite site = CurlSite();
    site.getSiteContent(s);
    struct site_info new_site;
    new_site.site_name = s;
    new_site.site_content = site.getContent();
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

/*
string currentTime() {
    time_t timer;
    struct tm * t;
    time(&timer);
    t = localtime(&timer);
    char* hour = itoa(t->tm_hour);
    char* minutes = itoa(t->tm_min);
    char* secs = itoa(t->tm_sec);
    time = hour + ":" + minutes + ":" + secs;
    return time;
}

string dateAndTime() {
    time_t timer;
    struct tm * t;
    time(&timer);
    t = localtime(&timer);
    char* day = itoa(t->tm_mday);
    char* month = itoa(t->tm_mon + 1);
    char* year = itoa(t->tm_year + 1990);
    string s = month + "-" + day + "-" + year + "-" + currentTime();
    return s;
}
*/