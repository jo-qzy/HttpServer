bin=HttpServer
cc=g++
LDFLAGS=-lpthread

.PHONY:clean HttpServer
HttpServer:HttpServer.cc
	$(cc) -g -std=c++11 -o $@ $^ $(LDFLAGS)

clean:
	rm -rf $(bin)
