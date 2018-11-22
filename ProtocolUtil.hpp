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
#include <unordered_map>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "Log.hpp"

#define ROOT_PATH "wwwroot"
#define DEFAULT_PAGE "index.html"

#define OK 200
#define NOT_FOUND 404

class StringUtil
{
	public:
		static int StrToInt(std::string& str_)
		{}

		static std::string IntToStr(int num_)
		{}
};

class Request
{
	public:
		// 首行、协议头、空行、正文
		std::string req_line;
		std::string req_header;
		std::string blank;
		std::string req_text;
	private:
		// 请求方法、资源标识符、版本号
		std::string method;
		std::string uri;
		std::string version;

		// method: POST->false, GET->true(?)
		bool cgi;

		// 资源路径、参数
		std::string path;
		std::string param;

		// 协议头键值对
		std::unordered_map<std::string, std:::string> header_kv;
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

		bool IsRequestLegal()
		{
			if (strcasecmp(method.c_str(), "POST") == 0 || cgi = (strcasecmp(method.c_str(), "GET") == 0))
			{
				return true;
			}
			return false;
		}

		bool IsPathLegal()
		{
			struct stat st;
			if (stat(path.c_str(), &st) != -1)
			{
				if ()
				if (S_ISDIR(st))
				{
					path += "/";
					path += DEFAULT_PAGE;
				}
			}
		}

		void GetHeaderKV()
		{
			std::string tmp_;
			int start_ = 0, pos_ = 0;
			while (std::string::npos == start_)
			{
				// 取出键值对
				pos_ = req_header.find('\n', start);
				tmp_ = req_header.substr(start_, pos_ - start_);
				start_ = pos_ + 1;

				// 分割键值对存储为key-value方式
				pos_ = tmp_.find(": ");
				header_kv.insert(makepair(tmp_.substr(0, pos_), tmp_.substr(pos_ + 2)));
			}
		}
};

class Response
{
	private:
		std::string rsp_line;
		std::string rsp_header;
		std::string blank;
		std::string rsp_text;
	public:
		int code;
	public:
		Response()
			: blank("\n")
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

		size_t RecvOneLine(std::string& str_)
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
					str_.push_back(ch_);
				}
			}
			return str_.size();
		}

		void GetHeader(std::string& header_)
		{
			// 获取header
			std::string tmp_;
			while (tmp_ != "\n")
			{
				tmp_.clear();
				RecvOneLine(tmp);
				if (tmp_ != "\n")
				{
					header_ += tmp_;
				}
			}
			// 读取协议头最后一行情况
			// key: value\r\n
			// \r\n\r\n
			// 空行有两个\r\n，只读取了一个，所以要把另一个也读了
			RecvOneLine(tmp_);
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

			if (req_->IsRequestLegal())
			{
				// uri解析
				req_->UriParse();
			}
			else
			{
				// 请求方法错误
				rsp_->code = NOT_FOUND;
				goto end;
			}

			// 先获取协议头，分割键值对
			client_->GetHeader(req_->req_header);
			req_->GetHeaderKV();
end:
			delete client_;
			delete req_;
			delete rsp_;
		}
};

#endif
