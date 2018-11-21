#ifndef __PROTOCOL_UTIL_HPP__
#define __PROTOCOL_UTIL_HPP__

#include <iostream>
#include <string>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>
#include <sstream>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "Log.hpp"

#define ROOT_PATH "webroot"
#define DEFAULT_PAGE "index.html"

#define OK 200
#define NOT_FOUND 404

class Request
{
	public:
		// 首行、协议头、空行、正文
		std::string req_line;
		std::string req_head;
		std::string blank;
		std::string req_text;
	private:
		// 请求方法、资源标识符、版本号
		std::string method;
		std::string uri;
		std::string version;

		// method: POST->false, GET->true(?)
		bool cgi;

		// 资源路径
		std::string path;
		// GET方法的参数
		std::string param;
	public:
		Request()
			: blank("\n")
			, cgi(false)
			, path(ROOT_PATH)
		{}

		void RequestLineParse()
		{
			std::stringstream ss_(req_line);
			ss_ >> method >> uri >> version;
		}

		void UriParse()
		{
			size_t pos_ = uri.find('?');
			if (pos_ != std::string::npos)
			{
				cgi = true;
				path = uri.substr(0, pos_ - 1);
				param = uri.substr(pos_ + 1);
			}
			else
			{
				path += uri;
			}
			if (path[path.size() - 1] == '/')
			{
				path += DEFAULT_PAGE;
			}
		}

		bool IsLegalRequest()
		{
			if (strcasecmp(method.c_str(), "POST") == 0 || strcasecmp(method.c_str(), "GET") == 0)
			{
				return true;
			}
			return false;
		}
};

class Response
{
	private:
		std::string rsp_line;
		std::string rsp_head;
		std::string blank;
		std::string rsp_text;
	public:
		int code;
	public:
		Response()
			: blank("\r\n")
			, code(OK)
		{}
};

class ClientConnect
{
	private:
		int sock;
	public:
		ClientConnect(int client_sock_)
			: sock(client_sock_)
		{}

		size_t RecvOneLine(std::string req_line_)
		{
			ssize_t recv_len_ = 0;
			char ch_ = 0;
			// 分隔符可能有三种情况，均需要处理
			// 1. \n	2. \r	3. \r\n
			while (ch_ != '\n')
			{
				recv_len_ = recv(sock, &ch_, 1, 0);
				if (recv_len_ <= 0)
				{
					if (errno == EINTR)
						continue;
					break;
				}
				else
				{
					if (ch_ == '\r')
					{
						recv(sock, &ch_, 1, MSG_PEEK);
						if (ch_ == '\n')
						{
							recv(sock, &ch_, 1, 0);
						}
						else
						{
							ch_ = '\n';
						}
					}
					req_line_.push_back(ch_);
				}
			}
			return req_line_.size();
		}

		~ClientConnect()
		{
			if (sock > 0)
			{
				close(sock);
			}
		}
};

class ConnectHandler
{
	public:
		// 处理连接请求
		static void* HandleConnect(void* sock_)
		{
			int client_sock_ = *(int*)sock_;
			ClientConnect* client_ = new ClientConnect(client_sock_);
			Request* req_ = new Request;
			Response* rsp_ = new Response;

			// 读取首行
			client_->RecvOneLine(req_->req_line);
			// 首行解析
			req_->RequestLineParse();

			if (req_->IsLegalRequest())
			{
				// uri解析
				req_->UriParse();
			}
			else
			{
				// 请求方法错误
				rsp_->code = NOT_FOUND;
			}

			delete client_;
			delete req_;
			delete rsp_;
		}
};

#endif
