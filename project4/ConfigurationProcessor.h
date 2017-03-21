#include <iostream>
#include <string>

class ConfigurationProcessor {

    public:
        ConfigurationProcessor();
        void printConfig();
        void loadConfig(std::string);
        int getFetchPeriod();
        int getNumFetch();
        int getNumParse();
        std::string getSiteFile();
        std::string getSearchFile();

    private:
        int period_fetch;
        int num_fetch;
        int num_parse;
        std::string search_file;
        std::string site_file;
};