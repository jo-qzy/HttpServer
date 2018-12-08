/*************************************************************************
	> File Name: Cal.c
	> Author: jo-qzy
	> Mail: jo-qzy@foxmail.com
	> Created Time: Thu 29 Nov 2018 10:09:28 AM CST
 ************************************************************************/

#include <iostream>
#include <string>
#include <cstdio>
#include <unistd.h>

using namespace std;

int main()
{
	size_t size = atoi(getenv("Content-Length"));
	char ch = 0;
	string param;
	while (size)
	{
		read(0, &ch, 1);
		param.push_back(ch);
		size--;
	}
	int num1, num2;
	sscanf(param.c_str(), "a=%d&b=%d", &num1, &num2);
	cout << "<html>\n<body>\n" << endl;
	cout << "<h1>" << num1 << "+" << num2 << "=" << num1 + num2 << "</h1>" << endl;
	cout << "<h1>" << num1 << "-" << num2 << "=" << num1 - num2 << "</h1>" << endl;
	cout << "<h1>" << num1 << "*" << num2 << "=" << num1 * num2 << "</h1>" << endl;
	cout << "</body>\n</html>\n";
	return 0;
}
