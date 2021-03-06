#ifndef __PROTOCOL_UTIL_H__
#define __PROTOCOL_UTIL_H__

#include <iostream>
#include <string>
#include <sstream>
#include <unordered_map>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/sendfile.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
#include "Log.hpp"

#define VERSION "HTTP/1.0"

#define DEFAULT_PAGE	"index.html"
#define ROOT			"wwwroot"

#define OK				200
#define BAD_REQUEST		400
#define NOT_FOUND		404
#define SERVER_ERROR	500

class ProtocolUtil
{
	private:
		static std::unordered_map<std::string, bool> _method;
		static std::unordered_map<std::string, std::string> _content_type;
		static std::unordered_map<int, std::string> _default_page;
	public:
		static bool MethodCheck(std::string& method)
		{
			return _method[method];
		}

		static int RecvOneLine(int sock, std::string& str)
		{
			char ch = 0;
			ssize_t recv_len = 0;
			while (ch != '\n')
			{
				recv_len = recv(sock, &ch, 1, 0);
				if (recv_len <= 0)
				{
					if (recv == 0 && errno == EINTR)
					{
						continue;
					}
					return -1;
				}
				else
				{
					if (ch == '\r')
					{
						recv(sock, &ch, 1, MSG_PEEK);
						if (ch == '\n')
						{
							recv(sock, &ch, 1, 0);
						}
						else
						{
							ch = '\n';
						}
					}
					str.push_back(ch);
				}
			}
			return (int)str.size();
		}

		static int StrToInt(std::string& str)
		{
			int ret = 0;
			std::stringstream ss(str);
			ss >> ret;
			return ret;
		}

		static std::string IntToStr(int num)
		{
			std::stringstream ss;
			ss << num;
			return ss.str();
		}

		static std::string GetContentType(std::string path)
		{
			size_t pos = path.rfind('.');
			if (pos == std::string::npos)
			{
				return "";
			}
			std::string file_suffix = path.substr(pos);
			return _content_type[file_suffix];
		}

		static std::string CodeToDesc(int code)
		{
			switch (code)
			{
				case 200:
					return "OK";
				case 302:
					return "FOUND";
				case 400:
					return "BAD REQUEST";
				case 404:
					return "NOT FOUND";
				case 500:
					return "SERVER ERROR";
				default:
					return "NOT FOUND";
			}
		}

		static std::string& GetDefaultPage(int code)
		{
			return _default_page[code];
		}
};

std::unordered_map<std::string, bool> ProtocolUtil::_method = {
	{"POST", true},
	{"GET", true}
};

std::unordered_map<std::string, std::string> ProtocolUtil::_content_type = {
	{".html", "text/html"},
	{".htm", "text/html"},
	{".js", "application/x-javascript"},
	{".css", "text/css"},
	{".jpg", "application/x-jpg"},
	{".jpeg", "image/jpeg"}
};

std::unordered_map<int, std::string> ProtocolUtil::_default_page = {
	{300, ""},
	{400, ""},
	{404, "wwwroot/404/index.html"},
	{500, ""}
};

class Request
{
	private:
		int _sock;
		
		std::string _line;
		std::string _header;
		std::string _text;

		std::string _method;
		std::string _uri;
		std::string _version;
		std::string _path;
		std::string _param;

		bool _cgi;
		int _resource_size;
		int _content_length;
		std::unordered_map<std::string, std::string> _header_kv;
	public:
		Request(int client_sock)
			: _sock(client_sock)
			, _path(ROOT)
			, _cgi(false)
		{}

		int LineParse()
		{
			// 1. get line and divide line
			if (!GetLine())
			{
				return BAD_REQUEST;
			}

			// 2. check method is legal or not
			//	  if method is "POST" ---> cgi = true
			if (!CheckMethod())
			{
				return BAD_REQUEST;
			}

			// 3. parse uri and get resource path
			ParseUri();
			
			// 4. check resource path legal or not
			if (!CheckPath())
			{
				return NOT_FOUND;
			}

			return OK;
		}

		int HeaderParse()
		{
			// Get header
			std::string tmp;
			while (tmp != "\n")
			{
				tmp.clear();
				if (ProtocolUtil::RecvOneLine(_sock, tmp) < 0)
				{
					LOG(ERROR, "recv header error");
					return SERVER_ERROR;
				}
				if (tmp != "\n")
				{
					_header += tmp;
				}
			}
			
			// Parse header
			size_t cur_pos = _header.find('\n');
			size_t last_pos = 0;
			size_t block_pos = 0;
			while (cur_pos != std::string::npos)
			{
				block_pos = _header.find(": ", last_pos);
				_header_kv.insert(make_pair(_header.substr(last_pos, block_pos - last_pos), _header.substr(block_pos + 2, cur_pos - block_pos - 2)));
				last_pos = cur_pos + 1;
				cur_pos = _header.find('\n', last_pos);
			}
			return OK;
		}

		void TextParse()
		{
			// Check text exist or not
			if (strcasecmp(_method.c_str(), "POST") != 0)
			{
				return;
			}
			std::string content_length = _header_kv["Content-Length"];
			if (content_length == "")
			{
				return;
			}
			_content_length = ProtocolUtil::StrToInt(content_length);
			
			// Get text
			char ch = 0;
			ssize_t recv_len = 0;
			while (recv_len < _content_length)
			{
				if (recv(_sock, &ch, 1, 0) <= 0)
				{
					if (errno == EINTR)
					{
						continue;
					}
					return;
				}
				_text.push_back(ch);
				recv_len++;
			}
			_param = _text;
		}

		int GetResourceSize()
		{
			return _resource_size;
		}

		void SetResourceSize(int resource_size)
		{
			_resource_size = resource_size;
		}

		int GetCgi()
		{
			return _cgi;
		}

		std::string& GetPath()
		{
			return _path;
		}

		std::string GetParam()
		{
			return _param;
		}

		void SetPath(int code)
		{
			_path.clear();
			_path = ProtocolUtil::GetDefaultPage(code);
			CheckPath();
		}

		void RecvAllData()
		{
			ssize_t recv_len = 0;
			char ch = 0;
			while (ch != '\0')
			{
				recv_len = recv(_sock, &ch, 1, 0);
				if (recv_len <= 0)
				{
					if (recv_len == 0)
					{
						return;
					}
					else if (recv_len < 0 && errno == EINTR)
					{
						continue;
					}
					else
					{
						LOG(ERROR, "recv reuqest error");
						break;
					}
				}
			}
		}
	private:
		bool GetLine()
		{
			// Get line and divide
			if (ProtocolUtil::RecvOneLine(_sock, _line) < 0)
			{
				LOG(ERROR, "Recv line error");
				return false;
			}
			std::stringstream ss(_line);
			ss >> _method >> _uri >> _version;
			return true;
		}

		bool CheckMethod()
		{
			// Check method
			for (size_t i = 0; i < _method.size(); i++)
			{
				if (_method[i] >= 'a')
					_method[i] -= 'a' - 'A';
			}
			if (!ProtocolUtil::MethodCheck(_method))
			{
				return false;
			}

			// Confirm cgi model
			if (_method == "POST")
			{
				_cgi = true;
			}
			return true;
		}

		void ParseUri()
		{
			// Uri parse --- Get path
			size_t pos = _uri.rfind('?');
			// If uri include character '?', cgi = true and
			// divide uri into two part --- path and parameter
			if (pos != std::string::npos)
			{
				// Uri has param, cgi-->true
				_cgi = true;
				_path += _uri.substr(0, pos);
				_param = _uri.substr(pos + 1);
				if (_path[_path.size() - 1] == '/')
				{
					_path += DEFAULT_PAGE;
				}
			}
			else
			{
				_path += _uri;
			}
		}

		bool CheckPath()
		{
			// Check resource path legal and get resource size
			struct stat st;
			if (stat(_path.c_str(), &st) < 0)
			{
				return false;
			}
			if (S_ISDIR(st.st_mode))
			{
				_path += '/';
				_path += DEFAULT_PAGE;
				stat(_path.c_str(), &st);
			}
			if ((S_IRUSR & st.st_mode) || (S_IRGRP & st.st_mode) || (S_IROTH & st.st_mode))
			{
				_resource_size = st.st_size;
			}
			else
			{
				return false;
			}
			return true;
		}
};

class Response
{
	private:
		int _sock;

		int _code;
		std::string _line;
		std::string _header;
		std::string _blank;
		std::string _text;
	public:
		Response(int client_sock)
			: _sock(client_sock)
			, _code(OK)
			, _blank("\n")
		{}

		void SetCode(int status)
		{
			_code = status;
		}

		int GetStatus()
		{
			return _code;
		}

		void MakeStatusLine()
		{
			_line += VERSION;
			_line += " ";
			_line += ProtocolUtil::IntToStr(_code);
			_line += " ";
			_line += ProtocolUtil::CodeToDesc(_code);
			_line += "\n";
		}

		void MakeHeader(Request*& req)
		{
			_header += "Content-Length: ";
			_header += ProtocolUtil::IntToStr(req->GetResourceSize());
			_header += "\n";
			_header += "Content-Type: ";
			std::string content_type = ProtocolUtil::GetContentType(req->GetPath());
			if (content_type == "")
			{
				_header += "text/html";
			}
			else
			{
				_header += content_type;
			}
			_header += "\n";
		}

		void SendResponse(Request*& req)
		{
			send(_sock, _line.c_str(), _line.size(), 0);
			send(_sock, _header.c_str(), _header.size(), 0);
			send(_sock, "\n", 1, 0);
			if (req->GetCgi())
			{
				send(_sock, _text.c_str(), _text.size(), 0);
			}
			else
			{
				int fd = open(req->GetPath().c_str(), O_RDONLY);
				sendfile(_sock, fd, NULL, req->GetResourceSize());
			}
		}

		std::string& GetText()
		{
			return _text;
		}

		~Response()
		{
			if (_sock > 0)
			{
				close(_sock);
			}
		}
};

class ConnectHandler
{
	public:
		static void* HandleConnect(int sock)
		{
			Request* req = new Request(sock);
			Response* resp = new Response(sock);
			int code = OK;
			
			// Handle request
			if ((code = req->LineParse()) != OK)
			{
				LOG(WARNING, "line parse failed");
				resp->SetCode(code);
				goto end;
			}

			if ((code = req->HeaderParse()) != OK)
			{
				LOG(WARNING, "header parse failed");
				resp->SetCode(code);
				goto end;
			}

			req->TextParse();

			// start response
			LOG(INFO, "request parse success, start to response");
			ProcessResponse(req, resp);
end:
			if (resp->GetStatus() != OK)
			{
				// Send page 404 not found
				// req->RecvAllData();
				//req->HeaderParse();
				//req->TextParse();
				//shutdown(client_sock, SHUT_RD);
				/*
				send(client_sock, "HTTP/1.1 302 FOUND\r\n", strlen("HTTP/1.1 302 FOUND\r\n"), 0);
				send(client_sock, "Content-Length: 0\r\n", strlen("Content-Length: 0\r\n"), 0);
				send(client_sock, "Location: http://192.168.44.134:8080/404/index.html\r\n\r\n", strlen("Location: http://192.168.44.134:8080/404/index.html\r\n\r\n"), 0);
				*/
				req->SetPath(code);

				ProcessResponse(req, resp);
				shutdown(sock, SHUT_RDWR);
			}
			delete req;
			delete resp;
		}
	private:
		static void ProcessResponse(Request*& req, Response*& resp)
		{
			if (req->GetCgi())
			{
				ProcessCgi(req, resp);
			}
			else
			{
				ProcessNonCgi(req, resp);
			}
		}

		static void ProcessCgi(Request*& req, Response*& resp)
		{
			// Create pipe for communicate
			int input[2], output[2];
			pipe(input);
			pipe(output);
			std::string param = req->GetParam();

			// Create child process to deal parameter
			int pid = fork();
			if (pid < 0)
			{
				resp->SetCode(NOT_FOUND);
				return;
			}
			else if (pid == 0)
			{
				close(input[1]);
				close(output[0]);
				dup2(input[0], 0);
				dup2(output[1], 1);

				// Set data size so child process can know the data size
				std::string env = "Content-Length=";
				env += ProtocolUtil::IntToStr(param.size());
				putenv((char*)env.c_str());

				const std::string& path = req->GetPath();
				execl(path.c_str(), path.c_str(), NULL);
				exit(1);
			}
			else
			{
				close(input[0]);
				close(output[1]);

				ssize_t send_len = 0;
				ssize_t send_cur = 0;
				ssize_t send_total = param.size();
				while (send_cur < send_total && (send_len = write(input[1], param.c_str() + send_cur, send_total - send_cur)) > 0)
				{
					send_cur += send_len;
				}

				std::string& text = resp->GetText();
				char ch;
				while (read(output[0], &ch, 1) > 0)
				{
					text.push_back(ch);
				}
				waitpid(pid, NULL, 0);

				req->SetResourceSize(text.size());
				resp->MakeStatusLine();
				resp->MakeHeader(req);
				resp->SendResponse(req);
			}
		}

		static void ProcessNonCgi(Request*& req, Response*& resp)
		{
			resp->MakeStatusLine();
			resp->MakeHeader(req);
			resp->SendResponse(req);
		}
};

#endif
