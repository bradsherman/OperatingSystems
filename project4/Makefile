FLAGS=-lcurl -pthread -Wall -g
CC=/usr/bin/g++

all: site-tester

site-tester: site-tester.cpp ConfigurationProcessor.o CurlSite.o
	$(CC) $^ -o $@ $(FLAGS)

ConfigurationProcessor.o: ConfigurationProcessor.cpp
	$(CC) -c $<

CurlSite.o: CurlSite.cpp
	$(CC) -c $<

clean:
	rm -f ConfigurationProcessor.o site-tester CurlSite.o
