bin=HttpServer
cc=g++
LDFLAGS=-lpthread

HttpServer:HttpServer.cc
	$(cc) -std=c++11 -o $@ $^ $(LDFLAGS)

.PHONY:clean
clean:
	rm -rf $(bin)
