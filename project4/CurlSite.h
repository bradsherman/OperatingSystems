#include <string>

using namespace std;

class CurlSite {

    public:
        CurlSite();
        void getSiteContent(string);
        void printContent();
        int countTerm(string);
        string getContent();

    private:
        string content;
        static size_t WriteMemoryCallback(void *, size_t, size_t, void *);
};