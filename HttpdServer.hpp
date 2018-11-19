#ifndef __HTTPD_SERVER_HPP__
#define __HTTPD_SERVER_HPP__

#include <iostream>
#include <string>
#include <stdlib.h>
#include <sys/socket.h>
#include "Log,hpp"

class HttpdServer
{
	private:
		int listen_sock;
		int port;
	public:
		HttppdServer()
			: listen_sock(0)
			, port(0)
		{}

		void InitServer(int port)
		{
			listen_sock = socket(AF_INET, SOCK__STREAM, IPPROTO_TCP);
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
			socklen_t len_ = sizeof(listen_addr);

			if (bind(listen_sock, (struct sockaddr*)&listen_addr_, sizeof(listen_addr_)) < 0)
			{
				LOG();
				exit(2);
			}

			if (listen(listen_sock, 5) < 0)
			{
				LOG();
				exit(3);
			}
		}

		void start()
		{
			while (1)
			{
				int client_sock_;
				struct sockaddr_in peer_;
				socklen_t len_ = sizeof(peer_);
				accept(&client_sock_, (struct sockaddr*)&peer_, &len_);
				if (client_sock < 0)
				{
					LOG();
					continue;
				}

				pthread_t tid_ = 0;
				pthread_create(&tid_, NULL, (void*)RequestUtil, (void*)port);
			}
		}

		~HttpdServer()
		{
			close(listen_sock);
		}
};

#endif
