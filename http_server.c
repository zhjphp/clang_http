#include <stdio.h>
#include <sys/types.h>
#include <ctype.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <errno.h>

#define SERVER_STRING "Server: jdbhttpd/0.1.0\r\n"

void son_process(int);
void unimplemented(int);
int read_line(int, char *, int);
pid_t pid;

int main(int argc, char *argv[])
{
	int port = 8989;
	int max_client = 128;
	int serversock, clientsock;
	struct sockaddr_in server_addr, client_addr;

	//建立sockets
	serversock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serversock < 0)
	{
		printf("建立 sockets 失败！ exit(1) \n");
		exit(1);
	}
	printf("建立socket -> %d \n", serversock);
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
		printf("bind serversock 失败！exit(1) \n");
		exit(1);
	}
	printf("bind serversock -> %d, port -> %d \n", serversock, port);

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
		printf("listen serversock 失败！exit(1) \n");
		exit(1);
	}
	printf("listen serversock -> %d \n", serversock);

	//可输出相关启动信息
	//printf("server start");

	int client_addr_size = sizeof(client_addr);

	printf("server start, ready accept! \n\n\n");
	//开始接受请求
	while (1)
	{
		//接收一个serversock中已建立的连接
		clientsock = accept(serversock, (struct sockaddr *)&client_addr, &client_addr_size);
		if (clientsock < 0)
		{
			printf("accept 失败！exit(1) \n");
			exit(1);
		}

		//开启多进程
		if ((pid = fork()) == 0)
		{
			//启动子进程
			printf("建立 clientsock -> %d \n", clientsock);
			printf("create pid -> %d \n", getpid());
			son_process(clientsock);
		}
		else if (pid < 0)
		{
			//子进程启动失败
			printf("fork 失败, pid -> %d \n", getpid());
		}
		else {
			// wait(NULL);
		}

	}

	printf("主进程退出！\n");
	close(serversock);

	return 0;
}

void son_process(int clientsock)
{
	printf("son_process start! \n");
	char buf[1024];
	int number;
	//获取第一行
	number = read_line(clientsock, buf, 1024);
	printf("read_line 1 number = %d \n", number);

	char method[255];
	char url[255];
	int i = 0;
	int j = 0;
	//读取method
	while (!isspace(buf[j]) && (i < sizeof(method) - 1))
	{
		method[i] = buf[j];
		i++;
		j++;
	}
	method[i] = '\0';
	printf("method = %s", method);
	//如果不支持请求的method，则报错。
	if (strcasecmp(method, "GE2T") && strcasecmp(method, "PO2ST"))
	{
		unimplemented(clientsock);
	}

	//读取第二行


	//关闭链接
	printf("发送数据完成，关闭链接\n");
	close(clientsock);
	printf("子进程退出\n\n\n");
	exit(0);
}

void execute_cgi(){

}

int read_line(int clientsock, char *buf, int buf_size)
{
	int recv_len;
	char c = '\0';

	int i = 0;
	int n = 0;
	//从sock中循环读取内容
	while ((i < buf_size - 1) && (c != '\n'))
	{
		//一次读取一个字节
		recv_len = recv(clientsock, &c, 1, 0);
		if (recv_len < 0)
		{
			fprintf(stderr, "Value of errno: %d\n", errno);
			return 0;
		}
		// printf("recv_len = %d \n", recv_len);
		// printf("read one char = %c \n", c);

		//如果接收到数据
		if (recv_len > 0)
		{
			if (c == '\r')
			{
				n = recv(clientsock, &c, 1, MSG_PEEK);
				if ((n > 0) && (c == '\n'))
					recv(clientsock, &c, 1, 0);
				else
					c = '\n';
			}
			//写入缓存
			buf[i] = c;
			i++;
		}
		else
		{
			c = '\n';
		}
	}

	buf[i] = '\0';
	return i;
}

void unimplemented(int client)
{
	char buf[1024];

	/* HTTP method 不被支持*/
	sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
	send(client, buf, strlen(buf), 0);
	/*服务器信息*/
	sprintf(buf, SERVER_STRING);
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "Content-Type: text/html\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "</TITLE></HEAD>\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "</BODY></HTML>\r\n");
	send(client, buf, strlen(buf), 0);
}
