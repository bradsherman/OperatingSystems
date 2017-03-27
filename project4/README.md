Site-Tester
===========

Brad Sherman
------------

To use the program first run:

    make

This will create all the files necessary to use the program. The program can be invoked with:

    ./site-tester <config-file>

The config file parameter is optional, and if no config file is specified, the program will use default values which are:

    Parameter       Description                                                 Default
    PERIOD_FETCH    The time (in seconds) between fetches of the various sites  180
    NUM_FETCH       Number of fetch threads (1 to 8)                            1
    NUM_PARSE       Number of parsing threads (1 to 8)                          1
    SEARCH_FILE     File containing the search strings                          Search.txt
    SITE_FILE       File containing the sites to query                          Sites.txt

The program will then run until told to stop, which can be done by pressing ctrl-c or by sending SIGHUP to the process. Once, the program is told to exit, it will cleanup and create a chart.html. This program uses chart.js, style.css, and all the csv files to display a graph of the results for each website and each batch. To clean up, simply run:

    make clean

This will remove all of the csv files, the executable, the .o files, and chart.html.

The files directory contains a sample config file along with two sample site and search files that are specified in the sample config file.