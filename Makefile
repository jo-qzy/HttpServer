bin=HttpServer
cc=g++
LDFLAGS=-lpthread

HttpServer:HttpServer.cc
	$(cc) -o $@ $^ $(LDFLAGS)

.PHONY:clean
clean:
	rm -rf $(bin)
