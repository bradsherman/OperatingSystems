#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <fstream>
#include "ConfigurationProcessor.h"

using namespace std;

ConfigurationProcessor::ConfigurationProcessor() {
    period_fetch = 180;
    num_fetch = 1;
    num_parse = 1;
    search_file = "Search.txt";
    site_file = "Sites.txt";
}

void ConfigurationProcessor::printConfig() {
    // print values of each configuration setting
    cout << "Fetch Period: " << period_fetch << "\n";
    cout << "# Fetch Threads: " << num_fetch << "\n";
    cout << "# Parse Threads: " << num_parse << "\n";
    cout << "Search File: " << search_file << "\n";
    cout << "Site File: " << site_file << "\n";
}

void ConfigurationProcessor::loadConfig(string filename) {
    // load any parameters in filename and update values
    ifstream infile(filename.c_str());
    string param, value, line;
    while(getline(infile, line)) {
        int pos = line.find("=");
        param = line.substr(0, pos);
        value = line.substr(pos+1, line.size());
        // switch param to update value
        if(param == "PERIOD_FETCH") {
            period_fetch = atoi(value.c_str());
        } else if(param == "NUM_FETCH") {
            num_fetch = atoi(value.c_str());
        } else if(param == "NUM_PARSE") {
            num_parse = atoi(value.c_str());
        } else if(param == "SEARCH_FILE") {
            search_file = value;
        } else if(param == "SITE_FILE") {
            site_file = value;
        } else {
            // ignore invalid params
        }
    }
}

int ConfigurationProcessor::getFetchPeriod() {
    return period_fetch;
}

int ConfigurationProcessor::getNumFetch() {
    return num_fetch;
}

int ConfigurationProcessor::getNumParse() {
    return num_parse;
}

string ConfigurationProcessor::getSiteFile() {
    return site_file;
}

string ConfigurationProcessor::getSearchFile() {
    return search_file;
}
