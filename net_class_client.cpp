#include<iostream>
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<netdb.h>

using namespace std;

class Student{
	public:
    int num;
    char name[1024];
};

//客户端通讯程序

int main()
{
	// if (argc != 3)
	// {
	// 	cout << "Using:./demo1服务端的IP 服务端的端口\nExample:./demo1 192.168.237.130 5005/n/n";
	// }
    
	//第1步，创建客户端的socket
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
	{
		perror("socket");
		return -1;
	}

	//第2步，向服务器发起连接请求
	//用于存放服务器IP的数据结构
	struct hostent* h;
	//指定服务端的ip地址
	//gethostbyname 能接收域名/主机名/和字符串格式的IP
	//如果用inet_addr只能接收IP
	if ((h = gethostbyname("192.168.237.130")) == 0)
	{
		std::cout << "gethostbyname failed./n" <<  endl;
		close(sockfd);
		return -1;
	}
	//用于存放服务器端IP和端口的结构体
	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	//指定服务端的IP地址
	memcpy(&servaddr.sin_addr, h->h_addr, h->h_length);
	//指定服务端的通信端口
	//htons()大小端转换
	servaddr.sin_port = htons(atoi("5005"));
	//向服务端发起连接请求
	if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0)
	{
		perror("connect");
		close(sockfd);
		return -1;
	}

	//第3步，与服务端通信，发送一个请求报文后等待回复，然后再发下一个请求报文
	char buffer[1024];
    Student *stu=new Student();
	for (int i = 0; i < 10; i++)
	{
		int iret;
		memset(buffer, 0, sizeof(buffer));
		//生成请求报文内容
		//sprintf(buffer, "这是第%d个超级女生，编号%03d。", i + 1, i + 1);
		//向服务端发送请求报文
        //stu->num=111;
		//stu->name="abc";
		//strcpy(stu->name,"abc");
		strcpy(buffer,"abc");
        	
        //memcpy(buffer,stu,sizeof(Student));
		if ((iret = send(sockfd,buffer, sizeof(buffer), 0)) <= 0)
		{
			perror("send");
			break;
		}
		cout << "发送：" << buffer << endl;
		memset(buffer, 0, sizeof(buffer));
		//接受服务端的回应报文，如果服务端没有发送回应报文，recv()函数将阻塞等待
		if ((iret = recv(sockfd, buffer, sizeof(buffer), 0)) <= 0)
		{
			cout << "iret=" << iret << endl;
			break;
		}
		cout << "接收" << buffer << endl;
		sleep(2);
	}

	//第4步，关闭socket，释放资源
	close(sockfd);

}
