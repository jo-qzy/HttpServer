#ifndef __PROTOCOL_UTIL_HPP__
#define __PROTOCOL_UTIL_HPP__

#include <iostream>
#include <string>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
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

#define HTTP_VERSION "http/1.0"

class StringUtil
{
	public:
		static int StringToInt(std::string& str_)
		{
			std::stringstream ss(str_);
			int ret_ = 0;
			ss >> ret_;
			return ret_;
		}

		static std::string IntToString(int num_)
		{
			std::stringstream ss;
			ss << num_;
			return ss.str();
		}

		static std::string CodeToDescribe(int code_)
		{
			switch(code_)
			{
				case 200:
					return "OK";
				case 404:
					return "NOT FOUND";
				default:
					return "NOT FOUND";
			}
		}
};

class Request
{
	public:
		// 首行、协议头、空行、正文
		std::string line;
		std::string header;
		std::string blank;
		std::string text;
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

		int resourse_size;
		int content_length;

		// 协议头键值对
		std::unordered_map<std::string, std::string> header_kv;
	public:
		Request()
			: blank("\n")
			, cgi(false)
			, path(ROOT_PATH)
			, resourse_size(0)
		{}

		void RequestLineParse()
		{
			std::stringstream ss_(line);
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

		bool IsMethodLegal()
		{
			cgi = strcasecmp(method.c_str(), "GET");
			if (strcasecmp(method.c_str(), "POST") == 0 || cgi)
			{
				return true;
			}
			return false;
		}

		bool IsPathLegal()
		{
			struct stat st;
			if (stat(path.c_str(), &st) < 0)
			{
				LOG(WARNING, "path not found!");
				return false;
			}
			if (S_ISDIR(st.st_mode))
			{
				path += "/";
				path += DEFAULT_PAGE;
				stat(path.c_str(), &st);
			}
			if ((st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) || (st.st_mode & S_IXOTH))
			{
				cgi = true;
			}
			resourse_size = st.st_size;
		}

		void HeaderParse()
		{
			std::string tmp_;
			size_t start_ = 0, pos_ = 0;
			while (start_ < header.size())
			{
				// 取出键值对
				pos_ = header.find('\n', start_);
				if (pos_ == std::string::npos)
				{
					break;
				}
				tmp_ = header.substr(start_, pos_ - start_);
				start_ = pos_ + 1;

				// 分割键值对存储为key-value方式
				pos_ = tmp_.find(": ");
				if (pos_ == std::string::npos)
				{
					continue;
				}
				header_kv.insert(std::make_pair(tmp_.substr(0, pos_), tmp_.substr(pos_ + 2)));
			}
		}

		bool IsTextExist()
		{
			if (strcasecmp(method.c_str(), "POST") == 0)
			{
				return true;
			}
			return false;
		}

		int GetContentLength()
		{
			std::string tmp_ = header_kv["Content-Length"];
			if (tmp_ == "")
			{
				return 0;
			}
			else
			{
				content_length = StringUtil::StringToInt(tmp_);
			}
			return content_length;
		}

		void GetParam()
		{
			// 有没有可能url有一部分参数，正文还有参数？
			param += text;
		}

		bool IsCgi()
		{
			return cgi;
		}

		int GetResourseSize()
		{
			return resourse_size;
		}
};

class Response
{
	private:
		std::string line;
		std::string header;
		std::string blank;
		std::string text;
	public:
		int code;
	public:
		Response()
			: blank("\n")
			, code(OK)
		{}

		void MakeStatusLine()
		{
			line = HTTP_VERSION;
			line += " ";
			line += StringUtil::IntToString(code);
			line += " ";
			line += StringUtil::CodeToDescribe(code);
		}

		void MakeHeader(Request*& req_)
		{
			header = "Content-Length: ";
			header += StringUtil::IntToString(req_->GetResourseSize());
			header += "\n";
			header += "Content-Type: text/html\n";
		}
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
				RecvOneLine(tmp_);
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

		void RecvText(std::string& text_, int content_lenth_)
		{
			char ch_ = 0;
			int recv_length_ = 0;
			int ret_ = 0;
			while (recv_length_ < 0)
			{
				ret_ = recv(sock, &ch_, 1, 0);
				if (ret_ <= 0)
				{
					if (errno == EINTR)
					{
						continue;
					}
					else
					{
						return;
					}
				}
				text_ += ch_;
				recv_length_++;
			}
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

			// 读取首行并解析
			client_->RecvOneLine(req_->line);
			req_->RequestLineParse();

			// 请求方法是否合法
			if (!req_->IsMethodLegal())
			{
				// 请求方法错误
				rsp_->code = NOT_FOUND;
				goto end;
			}
			// uri合法，开始解析
			req_->UriParse();

			// 路径是否合法
			if (!req_->IsPathLegal())
			{
				rsp_->code = NOT_FOUND;
				goto end;
			}

			// 首行分析完毕，获取协议头，分割键值对
			client_->GetHeader(req_->header);
			req_->HeaderParse();

			// 分析是否需要读取正文
			if (req_->IsTextExist())
			{
				client_->RecvText(req_->text, req_->GetContentLength());
				req_->GetParam();
			}

			ProcessResponse(client_, req_, rsp_);
end:
			delete client_;
			delete req_;
			delete rsp_;
		}
		
		static int ProcessCgi(ClientConnect*& client_, Request*& req_, Response*& rsp_)
		{
			rsp_->MakeStatusLine();
			rsp_->MakeHeader(req_);
		}

		static int ProcessResponse(ClientConnect*& client_, Request*& req_, Response*& rsp_)
		{
			// 判定cgi方法
			if (req_->IsCgi())
			{
				ProcessCgi(client_, req_, rsp_);
			}
			else
			{
				//ProcessNonCgi();
			}
			return 1;
		}

};

#endif
