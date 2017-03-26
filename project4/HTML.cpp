#include <iostream>
#include <fstream>
#include <stdlib.h>
#include "HTML.h"

using namespace std;

HTML::HTML() {
    outfile = "chart.html";
}

void HTML::writeHTMLPage(vector<string> websites, char numCSVS) {
    cout << "INFO: Writing to " << outfile << "\n";
    ofstream of(outfile.c_str(), ofstream::out);
    string sites = "";
    size_t i;
    for(i = 0; i < websites.size(); i++) {
        string tmp = string("<option value=") + websites[i] + ">" + websites[i] + "</option>\n";
        sites += tmp;
    }
    string csvs;
    vector<string> csv_vector = getCSVS(numCSVS);
    for(i = 0; i < csv_vector.size(); i++) {
        string tmp = string("<option value=") + csv_vector[i] + ">" + csv_vector[i] + "</option>\n";
        csvs += tmp;
    }
    string main = "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
            "<head>\n"
                "<meta charset=\"utf-8\">\n"
                "<title>Site Stats</title>\n"
                "<link rel=\"stylesheet\" type=\"text/css\" href=\"style.css\">\n"
                "<script type=\"text/javascript\" src=\"https://d3js.org/d3.v4.min.js\"></script>\n"
            "</head>\n"

            "<body>\n"
                "<h1 style=\"text-align: center;\">Site Stats</h1>\n"
                "<div id=\"listsContainer\" style=\"text-align:center;\">\n"
                    "<label>Site</label>\n"
                    "<select id=\"siteList\" onchange=\"startup()\">\n"
                     + sites +
                    "</select>\n"
                    "<label style=\"margin-left: 50px;\">Batch</label>\n"
                    "<select id=\"batchList\" onchange=\"startup()\">\n"
                    + csvs +
                    "</select>\n"
                "</div>\n"
                "<script type=\"text/javascript\" src=\"chart.js\"></script>\n"
                "<p><strong>Created by: </strong>Brad Sherman</p>\n"
            "</body>\n"
        "</html>\n";

    of << main;
    of.close();
}

vector<string> HTML::getCSVS(char numCSVS) {
    int i;
    vector<string> csvs;
    char n = '1';
    while(n < numCSVS) {
        string file = "";
        file.push_back(n);
        file += ".csv";
        csvs.push_back(file);
        n++;
    }
    return csvs;
}
