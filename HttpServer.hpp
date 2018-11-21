#ifndef __Http_SERVER_HPP__
#define __Http_SERVER_HPP__

#include "ProtocolUtil.hpp"

class HttpServer
{
	private:
		int listen_sock;
		int port;
	public:
		HttpServer()
			: listen_sock(0)
			, port(0)
		{}

		void InitServer(int port)
		{
			listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (listen_sock < 0)
			{
				LOG(ERROR, "create socket error!");
				exit(1);
			}

			int opt_ = 0;
			setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt_, sizeof(opt_));

			struct sockaddr_in listen_addr_;
			listen_addr_.sin_family = AF_INET;
			listen_addr_.sin_port = htons(port);
			listen_addr_.sin_addr.s_addr = INADDR_ANY;

			if (bind(listen_sock, (struct sockaddr*)&listen_addr_, sizeof(listen_addr_)) < 0)
			{
				LOG(ERROR, "bind error!");
				exit(2);
			}

			if (listen(listen_sock, 5) < 0)
			{
				LOG(ERROR, "listen error!");
				exit(3);
			}
			LOG(INFO, "init server success!");
		}

		void Start()
		{
			LOG(INFO, "start to accept link...");
			while (1)
			{
				int client_sock_ = 0;
				struct sockaddr_in peer_;
				socklen_t len_ = sizeof(peer_);
				client_sock_ = accept(listen_sock, (struct sockaddr*)&peer_, &len_);
				if (client_sock_ < 0)
				{
					LOG(WARNING, "accept link failed");
					continue;
				}

				LOG(INFO, "accept new link from far-end, create thread handle link...");
				SetNonblock(client_sock_);
				pthread_t tid_ = 0;
				pthread_create(&tid_, NULL, ConnectHandler::HandleConnect, (void*)&client_sock_);
			}
		}

		void SetNonblock(int sock_)
		{
			int flag_ = fcntl(sock_, F_GETFL, 0);
			fcntl(sock_, F_SETFL, flag_ | O_NONBLOCK);
		}

		~HttpServer()
		{
			if (listen_sock >= 0)
			{
				close(listen_sock);
			}
		}
};

#endif
