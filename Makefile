bin=HttpdServer
cc=g++
LDFLAGS=-lpthread

HttpdServer:HttpdServer.cc
	$(cc) -o $@ $^ $(LDFLAGS)

.PHONY:clean
clean:
	rm -rf $(bin)
