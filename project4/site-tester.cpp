#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include "ConfigurationProcessor.h"
#include "CurlSite.h"
#include "ConcurrentQueue.h"

using namespace std;

vector<string> fileToVector(string);
void printVec(vector<string>);
void start_work(int);
void * fetch_site(void *);
void * parse_site(void *);

struct site_info {
    string site_name;
    string site_content;
};

// Globals
ConfigurationProcessor CONFIG;
vector<string> sites;
vector<string> searches;
ConcurrentQueue<string> fetch_queue;
int active_fetch_threads;
ConcurrentQueue<struct site_info *> parse_queue;
int active_parse_threads;

int main(int argc, char *argv[])
{
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
    // queue all the sites to be fetched
    for(size_t i = 0; i < sites.size(); i++) {
        fetch_queue.enqueue(sites[i]);
    }

    pthread_t * fetch_threads;
    active_fetch_threads = 0;
    fetch_threads = (pthread_t *)malloc(CONFIG.getNumFetch());

    // have different threads fetch each site
    while(!fetch_queue.empty()) {
        // if we reach max threads, wait for them to finish
        // and start making more
        if(active_fetch_threads == CONFIG.getNumFetch()) {
            while(active_fetch_threads > 0) {
                pthread_join(fetch_threads[active_fetch_threads], NULL);
                active_fetch_threads--;
            }
        }
        // TODO: cannot take address of temporary
        pthread_create(&fetch_threads[active_fetch_threads], NULL, fetch_site, &fetch_queue.dequeue());
        active_fetch_threads++;
    }


    pthread_t * parse_threads;
    active_parse_threads = 0;
    parse_threads = (pthread_t *)malloc(CONFIG.getNumParse());

    while(!parse_queue.empty()) {
        // if we reach max threads, wait for them to finish
        // and start making more
        if(active_parse_threads == CONFIG.getNumParse()) {
            for(int i = 0; i < CONFIG.getNumParse(); i++) {
                pthread_join(parse_threads[i], NULL);
                active_parse_threads--;
            }
        }

        pthread_create(&parse_threads[active_parse_threads], NULL, parse_site, parse_queue.dequeue());
        active_parse_threads++;
    }

    alarm(CONFIG.getFetchPeriod());
}

void * fetch_site(void * arg) {
    string * s = (string*)arg;
    // curl a site
    CurlSite site = CurlSite();
    cout << "fetching " << *s << endl;
    site.getSiteContent(*s);
    //site.printContent();
    cout << "before first thread ends\n";
    struct site_info * new_site = (struct site_info *)malloc(sizeof(struct site_info));
    new_site->site_name = *s;
    new_site->site_content = site.getContent();
    parse_queue.enqueue(new_site);
    return NULL;
}

void * parse_site(void * args) {
    // possibly use Output class to write results upon finding them here
    struct site_info * s = (struct site_info *) args;
    cout << "parsing sites\n";
    cout << "site: " << s->site_name << endl;
    cout << "content: " << s->site_content << endl;
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