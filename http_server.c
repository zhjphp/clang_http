#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	int port = 0;
	int max_client = 128;
	int serversock, clientsock;
	struct sockaddr_in server_addr, client_addr;

	//建立sockets
	if ((serversock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
		exit(1);
	}
	printf("socket -> %d \n", serversock);
	// printf("SOCK_STREAM -> %d \n", SOCK_STREAM);
	// printf("IPPROTO_TCP -> %d \n", IPPROTO_TCP);
	// printf("PF_INET -> %d \n", PF_INET);

	//设置服务器地址参数
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;				 //协议
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY); //地址
	server_addr.sin_port = htons(port);				 //端口

	//服务器端要用 bind() 函数将套接字与特定的IP地址和端口绑定起来，只有这样，流经该IP地址和端口的数据才能交给套接字处理
	if (bind(serversock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		exit(1);
	}
	printf("bind -> %d \n", serversock);

	//如果端口为0，则使用系统分配的端口号
	if (port == 0)
	{
		int addr_len = sizeof(server_addr);
		if (getsockname(serversock, (struct sockaddr *)&server_addr, &addr_len) == -1)
		{
			exit(1);
		}
		port = ntohs(server_addr.sin_port);
		printf("re_port -> %d \n", port);
		//printf("getsockname -> %d \n", getsockname(serversock, (struct sockaddr *)&server_addr, &addr_len));
	}
	
	//使serversock处于监听到来的连接请求的状态
	if (listen(serversock, max_client) < 0)
	{
		exit(1);
	}
	printf("listen -> %d \n", serversock);

	//可输出相关启动信息
	//printf("server start");

	//开始接受请求
	while (1)
	{
		//接收一个serversock中已建立的连接
		if ( (clientsock = accept(serversock, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0 ))
		{
			exit(1);
		}

		//开启多进程
		if (fork() == 0)
		{
			//启动子进程

		} else {
			//子进程启动失败，停止执行，并等待子进程退出
			wait(NULL);
		}
	}

	return 0;
}

int son_process(int clientsock)
{
	char buf[1024];
	char method[255];
	char url[255];



}

void read_line(int clientsock, char *buf, int buf_size)
{
	int recv_len;
	char c = "\0";

	//从sock中循环读取内容
	recv_len = recv(clientsock, &c, 1, 0);
	
}
