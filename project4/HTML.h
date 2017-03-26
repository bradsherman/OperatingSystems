#include <string>
#include <vector>

class HTML {

    public:
        HTML();
        void writeHTMLPage(std::vector<std::string>, char);
        std::vector<std::string> getCSVS(char);

    private:
        std::string outfile;

};