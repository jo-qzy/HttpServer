#include "HttpServer.hpp"

static void Usage(std::string name_)
{
	std::cout << "Usage: " << name_ << " port" << std::endl;
}

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		Usage(argv[0]);
	}
    int fd = -1;
    if ((fd = open("./log", O_WRONLY | O_CREAT)) < 0)
    {
        LOG(ERROR, "open error");
    }
    dup2(1, fd);
    if (daemon(1, 1) < 0)
    {
        LOG(ERROR, "daemon failed");
    }
	HttpServer* server_ = new HttpServer;
	server_->InitServer(atoi(argv[1]));
    server_->Start();

	delete server_;
	return 0;
}
