#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>

#define URL_BUFF_SIZE 8182
#define RESPONSE_BUF_SIZE 4096
#define CHUNKED_MAX_LEN 8182

struct arg_para
{
	char protocol[10];		 //请求协议
	char method_str[10];	 //请求类型，GET、POST
	int method;				 //method_str请求类型的int，1:GET、2:POST
	char domain[255];		 //请求的域名
	char port_str[10];		 //请求的端口
	char uri[4096];			 //请求的uri
	char data[4096];		 //请求的数据，逗号间隔键值对
	char data_str[4096];	 //经过处理后的请求数据
	char data_str_len[1000]; //经过处理后的请求数据
	int port;				 //int型port_str
};

void test_print_host(struct hostent *);
struct arg_para *make_data_str(struct arg_para *, char *);
char *make_req_data(struct arg_para *, char *);
int get_line(int, char *, int);
int htoi(unsigned char *);
int _find_key(unsigned char *, int, unsigned char *, int, int *);
int de_chunked(unsigned char *, int, unsigned char *, int *);
void get_body(int, char *, int);
void get_get(int, char *, int);
void kill_request();

/*
	请求参数最大值为6个
	1:请求协议
	2:请求类型，GET、POST
	3:请求的域名
	4:请求的端口
	5:请求的uri
	6:请求的数据，逗号间隔键值对
	cmd: ./http_client http post 127.0.0.1 8888 /c/m2o action,update,tb_name,ecs_order_info,pk_name,order_id,pk_value,4059
		 /home/pray/zhj/c/http_client http post 127.0.0.1 8888 /c/m2o action,update,tb_name,ecs_order_info,pk_name,order_id,pk_value,4064
*/
int main(int arg, char *argv[])
{
	//打印测试参数
	// printf("参数个数:%d \n", arg);
	// for (int i = 0; i < arg; i++)
	// {
	// 	printf("第%d个: %s \n", i, argv[i]);
	// }
	//构建请求参数结构体
	struct arg_para arg_para;
	memset(&arg_para, 0, sizeof(arg_para));
	for (int i = 1; i < arg; i++)
	{
		switch (i)
		{
		case 1:
			strcpy(arg_para.protocol, argv[i]);
			break;
		case 2:
			strcpy(arg_para.method_str, argv[i]);
			if (strcmp(arg_para.method_str, "get") == 0)
			{
				arg_para.method = 1;
			}
			else if (strcmp(arg_para.method_str, "post") == 0)
			{
				arg_para.method = 2;
			}
			else
			{
				printf("暂不支持的method方法！ \n");
				exit(1);
			}
			break;
		case 3:
			strcpy(arg_para.domain, argv[i]);
			break;
		case 4:
			strcpy(arg_para.port_str, argv[i]);
			//把string类型的port_str转为int
			arg_para.port = atoi(arg_para.port_str);
			if (arg_para.port == 0)
			{
				printf("端口号非法！ \n");
				exit(1);
			}
			break;
		case 5:
			strcpy(arg_para.uri, argv[i]);
			break;
		case 6:
			strcpy(arg_para.data, argv[i]);
			break;
		case 7:
			printf("参数错误！ \n");
			exit(1);
			break;
		}
	}
	/*
		//打印测试参数赋值
		printf("protocol: %s \n", arg_para.protocol);
		printf("method_str: %s \n", arg_para.method_str);
		printf("method: %d \n", arg_para.method);
		printf("domain: %s \n", arg_para.domain);
		printf("port_str: %s \n", arg_para.port_str);
		printf("port: %d \n", arg_para.port);
		printf("uri: %s \n", arg_para.uri);
		printf("data: %s \n", arg_para.data);
		exit(0);
	*/

	struct hostent *host;
	host = gethostbyname(arg_para.domain);
	//test_print_host(host);	//测试打印host内容

	int sockfd;								  //socket标识符
	sockfd = socket(AF_INET, SOCK_STREAM, 0); //建立socket链接
	if (sockfd == -1)
	{
		printf("error");
		//printf("scoket 建立失败！ \n");
		exit(1);
	}
	//服务端地址
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;										//协议
	server_addr.sin_addr.s_addr = *((unsigned long *)host->h_addr_list[0]); //地址
	server_addr.sin_port = htons(arg_para.port);							//端口

	int connectfd;
	connectfd = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (connectfd == -1)
	{
		printf("error");
		//printf("connect 建立链接失败！\n");
		//fprintf(stderr, "Value of errno: %d\n", errno);
		exit(1);
	}

	char req_data[URL_BUFF_SIZE]; //请求数据
	char *req_data_p;
	memset(req_data, 0, sizeof(req_data));
	//拼装请求数据
	req_data_p = make_req_data(&arg_para, req_data);
	//printf("请求数据： \n");
	//printf("%s \n", req_data_p);
	//发送请求
	int req_res;
	req_res = send(sockfd, req_data, strlen(req_data), 0);
	if (req_res == -1)
	{
		printf("error");
		//printf("send 数据失败！\n");
		//fprintf(stderr, "Value of errno: %d\n", errno);
		exit(1);
	}

	// kill_request();

	//取出应答头
	// char resp_line_buf[RESPONSE_BUF_SIZE];
	// int r = 1;
	// memset(resp_line_buf, 0, sizeof(resp_line_buf));
	// printf("应答头： \n");
	// while ((r > 0) && strcmp("\n", resp_line_buf))
	// {
	// 	r = get_line(sockfd, resp_line_buf, sizeof(resp_line_buf));
	// }

	char resp_buf[RESPONSE_BUF_SIZE]; //原始应答缓存
	memset(resp_buf, 0, sizeof(resp_buf));
	char resp_de_buf[RESPONSE_BUF_SIZE]; //chunked解码后应答缓存，也是真正的应答内容
	memset(resp_de_buf, 0, sizeof(resp_de_buf));
	recv(sockfd, resp_buf, sizeof(resp_buf), 0); //一次性读出应答内容至缓存
	// printf("应答1： \n");
	// printf("len = %d \n", strlen(resp_buf));
	//printf("%s \n", resp_buf);
	int x = RESPONSE_BUF_SIZE;
	//chunked解码
	de_chunked(resp_buf, RESPONSE_BUF_SIZE, resp_de_buf, &x);
	//printf("应答2： \n");
	printf("%s \n", resp_de_buf);
	if (strcmp("success", resp_de_buf))
	{
	}
	else
	{
	}

	close(sockfd);
}

void kill_request()
{
	printf("%s \n", "end");
	exit(999);
}

int get_line(int sock, char *buf, int size)
{
	int i = 0;
	char c = '\0';
	int n;

	/*把终止条件统一为 \n 换行符，标准化 buf 数组*/
	while ((i < size - 1) && (c != '\n'))
	{
		/*一次仅接收一个字节*/
		n = recv(sock, &c, 1, 0);
		/* DEBUG printf("%02X\n", c); */
		if (n > 0)
		{
			/*收到 \r 则继续接收下个字节，因为换行符可能是 \r\n */
			if (c == '\r')
			{
				/*使用 MSG_PEEK 标志使下一次读取依然可以得到这次读取的内容，可认为接收窗口不滑动*/
				n = recv(sock, &c, 1, MSG_PEEK);
				/* DEBUG printf("%02X\n", c); */
				/*但如果是换行符则把它吸收掉*/
				if ((n > 0) && (c == '\n'))
				{
					recv(sock, &c, 1, 0);
				}

				else
				{
					c = '\n';
				}
			}
			/*存到缓冲区*/
			buf[i] = c;
			i++;
		}
		else
		{
			c = '\n';
		}
	}
	buf[i] = '\0';
	//printf("len = %d \n", strlen(buf));
	//printf("%s", buf);

	/*返回 buf 数组大小*/
	return (i);
}

void get_body(int sockfd, char *buf, int size)
{
	char c = 'a';
	int i = 0;
	int n = 1;

	while (1)
	{
		n = recv(sockfd, &c, 1, 0);
		if (n == -1)
		{
			printf("error");
			//printf("recv 数据失败xx！\n");
			//fprintf(stderr, "Value of errno: %d\n", errno);
			exit(1);
		}
		if (n == 0)
		{
			break;
		}
		buf[i] = c;
		i++;
	}
	//buf[i] = '\0';
	//printf("%s \n", buf);
}

char *make_req_data(struct arg_para *arg_para, char *req_data)
{
	//处理请求键值对
	if (*(arg_para->data) != '\0' && *(arg_para->data) != 0)
	{
		arg_para = make_data_str(arg_para, req_data);
	}
	//http请求第一行
	if (arg_para->method == 1)
	{
		strcat(req_data, "GET ");
	}
	else if (arg_para->method == 2)
	{
		strcat(req_data, "POST ");
	}
	strcat(req_data, arg_para->uri);
	//如果方法为get，则把请求数据拼装入uri
	if (arg_para->method == 1)
	{
		strcat(req_data, "?");
		strcat(req_data, arg_para->data_str);
	}
	strcat(req_data, " HTTP/1.1");
	strcat(req_data, "\r\n");
	//http请求第二行
	strcat(req_data, "Host: ");
	strcat(req_data, arg_para->domain);
	if (arg_para->port != 80)
	{
		strcat(req_data, ":");
		strcat(req_data, arg_para->port_str);
	}
	strcat(req_data, "\r\n");
	//http请求第三行
	strcat(req_data, "Connection: ");
	strcat(req_data, "close");
	strcat(req_data, "\r\n");
	//http请求第四行
	strcat(req_data, "User-Agent: ");
	strcat(req_data, "mysql sync oracle");
	strcat(req_data, "\r\n");
	//http请求第五行
	strcat(req_data, "Accept: ");
	strcat(req_data, "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8");
	strcat(req_data, "\r\n");
	//http请求第六行
	strcat(req_data, "Accept-Encoding: ");
	strcat(req_data, "none");
	strcat(req_data, "\r\n");
	//http请求第七行
	strcat(req_data, "Accept-Language: ");
	strcat(req_data, "zh-CN,zh;q=0.9");
	strcat(req_data, "\r\n");

	//get请求body
	if (arg_para->method == 1)
	{
		strcat(req_data, "\r\n");
	}

	//post请求body
	if (arg_para->method == 2)
	{
		strcat(req_data, "Content-Type: ");
		strcat(req_data, "application/x-www-form-urlencoded; charset=utf-8");
		strcat(req_data, "\r\n");
		strcat(req_data, "Content-Length: ");
		strcat(req_data, arg_para->data_str_len);
		strcat(req_data, "\r\n\r\n");
		strcat(req_data, arg_para->data_str);
	}

	return req_data;
}

struct arg_para *make_data_str(struct arg_para *arg_para, char *req_data)
{
	char *tmp_arg_data_count;
	int count = 0;	 //键值对数量
	char *delim = ","; //设定分隔符
	char *tmp;

	//计算请求参数总数量
	tmp_arg_data_count = (char *)calloc(1, sizeof(arg_para->data));
	strcpy(tmp_arg_data_count, arg_para->data);
	tmp = strtok(tmp_arg_data_count, delim);
	if (tmp == NULL)
	{
		printf("error");
		//printf("请求参数错误，make_data_str错误！ \n");
		exit(1);
	}
	count++;
	while ((tmp = strtok(NULL, delim)))
	{
		count++;
	}
	free(tmp_arg_data_count);

	//分配请求键值对
	int arr_count = count / 2;
	char *req_k[arr_count]; //请求字符串键值对，key
	char *req_v[arr_count]; //请求字符串键值对，value
	char *tmp_arg_data;
	int i = 0;
	count = 1;
	tmp_arg_data = (char *)calloc(1, sizeof(arg_para->data));
	strcpy(tmp_arg_data_count, arg_para->data);
	tmp = strtok(tmp_arg_data_count, delim);
	req_k[0] = tmp;
	i++;
	count++;
	while ((tmp = strtok(NULL, delim)))
	{
		if ((count % 2 == 0))
		{
			req_v[i - 1] = tmp;
		}
		else if ((count % 2 == 1))
		{
			req_k[i] = tmp;
			i++;
		}
		count++;
	}
	//printf("arr_count = %d \n", arr_count);
	//拼接字符串
	for (int j = 0; j < arr_count;)
	{
		//printf("%s = %s \n", req_k[j], req_v[j]);
		strcat(arg_para->data_str, req_k[j]);
		strcat(arg_para->data_str, "=");
		strcat(arg_para->data_str, req_v[j]);
		j++;
		if (j < arr_count)
		{
			//printf("&\n");
			strcat(arg_para->data_str, "&");
		}
	}
	//printf("%s \n", arg_para->data_str);
	int data_str_len = strlen(arg_para->data_str);
	sprintf(arg_para->data_str_len, "%d", data_str_len);
	//itoa(data_str_len, arg_para->data_str_len, 10);
	free(tmp_arg_data);
	return arg_para;
}

/*
   * 十六进制表示的字符串转换为相应的十进制值 传入"7f"返回127
*/
int htoi(unsigned char *s)
{
	int i;
	int n = 0;
	if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) //判断是否有前导0x或者0X
	{
		i = 2;
	}
	else
	{
		i = 0;
	}
	for (; (s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'z') || (s[i] >= 'A' && s[i] <= 'Z'); ++i)
	{
		if (tolower(s[i]) > '9')
		{
			n = 16 * n + (10 + tolower(s[i]) - 'a');
		}
		else
		{
			n = 16 * n + (tolower(s[i]) - '0');
		}
	}
	return n;
}

/*
 * 查找关键数据串在长数据中出现的位置
 * 参数：1长数据指针，2搜索最大长度，3关键字指针，4关键字长度，5搜索起始位置(返回出现位置，若未找到则不变)
 * 返回：返回1 成功 返回 0 未找到
 */
int _find_key(unsigned char *data, int data_length, unsigned char *key, int key_length, int *position)
{
	int i = *position;
	if (key == NULL || i < 0)
	{
		return 0;
	}
	for (; i <= data_length - key_length; i++)
	{
		if (memcmp(data + i, key, key_length) == 0)
		{
			*position = i;
			return 1;
		}
	}
	return 0;
}

/*
 * 对HTTP的chunked消息进行合块
 * 参数：1待处理数据，2数据长度(分配的长度即可，不一定要求出实际有效长度)，3返回合块后的数据，4合块长度
 * 算法具有前驱性，返回和传入data可以是同一块内存区域(不建议)
 */
int de_chunked(unsigned char *data, int data_length, unsigned char *dest, int *dest_length)
{
	char chunked_hex[CHUNKED_MAX_LEN + 1]; // 十六进制的块长度
	int chunked_len;					   // 块长度
	int ret;
	int begin = 0;
	int end = 0;
	int i = 0;
	int index = 0;

	ret = _find_key(data, data_length, "0\r\n\r\n", 5, &end);
	if (ret == 0) //信息不完整
		return 0;

	ret = _find_key(data, data_length, "\r\n\r\n", 4, &begin);
	begin = begin + 4; //移动到数据起点

	while (memcmp(data + begin, "0\r\n\r\n", 5) != 0)
	{
		//获得当前块长度
		ret = _find_key(data + begin, CHUNKED_MAX_LEN, "\r\n", 2, &i);
		if (ret == 0) //信息不完整
			return 0;
		memcpy(chunked_hex, data + begin, i);
		chunked_hex[i] = '\0';
		chunked_len = htoi(chunked_hex);
		//移动到当前块数据段
		begin = begin + i + 2;
		//获得当前块数据
		if (memcmp(data + begin + chunked_len, "\r\n", 2) != 0)
			return 0; //信息有误
		memcpy(dest + index, data + begin, chunked_len);
		index = index + chunked_len;
		//移动到下一块块长度
		begin = begin + chunked_len + 2;
		i = begin;
		if (begin > end) //结构错误
			return -1;
	}
	*dest_length = index;
	return 1;
}

//测试打印host内容
void test_print_host(struct hostent *host)
{
	if (!host)
	{
		printf("域名解析失败！ \n");
		exit(1);
	}
	for (int i = 0; host->h_aliases[i]; i++)
	{
		printf("Aliases %d: %s\n", i + 1, host->h_aliases[i]);
	}
	printf("Address type: %s\n", (host->h_addrtype == AF_INET) ? "AF_INET" : "AF_INET6");
	for (int i = 0; host->h_addr_list[i]; i++)
	{
		printf("IP addr %d: %s\n", i + 1, inet_ntoa(*(struct in_addr *)host->h_addr_list[i]));
	}
}