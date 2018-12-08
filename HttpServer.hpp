#ifndef __Http_SERVER_HPP__
#define __Http_SERVER_HPP__

#include "ProtocolUtil.hpp"
#include "ThreadPool.hpp"

class HttpServer
{
	private:
		int _listen_sock;
		int _port;
	public:
		HttpServer()
			: _listen_sock(0)
			, _port(0)
		{}

		void InitServer(int port)
		{
			signal(SIGPIPE, SIG_IGN);

			_listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (_listen_sock < 0)
			{
				LOG(ERROR, "create socket error!");
				exit(1);
			}

			size_t opt = 1;
			setsockopt(_listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

			struct sockaddr_in listen_addr;
			listen_addr.sin_family = AF_INET;
			listen_addr.sin_port = htons(port);
			listen_addr.sin_addr.s_addr = INADDR_ANY;

			if (bind(_listen_sock, (struct sockaddr*)&listen_addr, sizeof(listen_addr)) < 0)
			{
				printf("%s\n", strerror(errno));
				LOG(ERROR, "bind error:%s!");
				exit(2);
			}

			if (listen(_listen_sock, 5) < 0)
			{
				LOG(ERROR, "listen error!");
				exit(3);
			}
			LOG(INFO, "init server success!");
		}

		void Start()
		{
			LOG(INFO, "start to accept link...");
			ThreadPool tp;
			while (1)
			{
				int client_sock = 0;
				struct sockaddr_in peer;
				socklen_t len = sizeof(peer);
				client_sock = accept(_listen_sock, (struct sockaddr*)&peer, &len);
				if (client_sock < 0)
				{
					LOG(WARNING, "accept link failed");
					continue;
				}

				LOG(INFO, "accept new link from far-end, create thread handle link...");
				// SetNonblock(client_sock);
				Task t(client_sock, ConnectHandler::HandleConnect);
				tp.PushTask(t);
				//int* new_sock = new int(client_sock);
				//pthread_t tid = 0;
				//pthread_create(&tid, NULL, ConnectHandler::HandleConnect, (void*)new_sock);
			}
		}

		void SetNonblock(int sock)
		{
			int flag = fcntl(sock, F_GETFL, 0);
			fcntl(sock, F_SETFL, flag | O_NONBLOCK);
		}

		~HttpServer()
		{
			if (_listen_sock >= 0)
			{
				close(_listen_sock);
			}
		}
};

#endif
