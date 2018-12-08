bin=HttpServer
cc=g++
LDFLAGS=-lpthread

all:$(bin) cgi httpserver

.PHONY:HttpServer
HttpServer:HttpServer.cc
	$(cc) -g -std=c++11 -o $@ $^ $(LDFLAGS)

.PHONY:cgi
cgi:cgi.cc
	g++ -std=c++11 -o $@ $^

.PHONY:clean
clean:
	rm -rf httpserver

.PHONY:httpserver
httpserver:
	mkdir httpserver
	mv $(bin) httpserver
	cp -rf wwwroot httpserver
	mv cgi httpserver/wwwroot/
	cp start.sh httpserver
