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
	HttpServer* server_ = new HttpServer;
	server_->InitServer(atoi(argv[1]));
	server_->Start();

	delete server_;
	return 0;
}
