#include<iostream>
#include <queue>
#include <vector>
#include <thread>
#include <condition_variable>
#include <functional>
#include <mutex>
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<string.h>

using namespace std;

class ThreadPool
{
    public:
    //线程池初始化时
    /*
        1.设置线程池大小
        2.给每个线程设置等待唤醒、在任务队列中取任务执行、重新进入等待
    */

    ThreadPool(int numthreads) : stop(false)
    {
        /*给线程池加入第一个线程，阻塞输入str
        如果str=="stop"，则对象中stop置为true
        如果str!="stop"，则输入错误，重新进入循环
        该线程为控制服务器关闭提供命令
        */
        threads.emplace_back([this]{
            string str;
            while(!stop)
            {
                cin>>str;
                if(str=="stop")
                {
                    stop=true;
                    cout<<"stop success..."<<endl;
                    break;
                }
                else
                {
                    cout<<"input error"<<endl;
                    continue;
                }
                cout<<"stop ending..."<<endl;
            }
        });

        /*
        开创线程池，线程池大小为对象创建时传入
        在线程中，互斥从tasks队列中取出任务，然后并行执行task
        */
        for(int i=0;i<numthreads;i++)
        {
           threads.emplace_back([this]{
                while(true)
                {
                    //取task时要互斥，但执行task时不能互斥
                    unique_lock<mutex> lock(mtx);
                    cv.wait(lock,[this]{
                        return !tasks.empty()||stop;
                        });
                    if(tasks.empty()&&stop)
                        break;
                    function<void()> task (tasks.front());
                    tasks.pop();
                    lock.unlock();

                    task();
                }
            });
        }
    }
    /*
    在程序退出之前，要把每个线程join()进去，防止有任务没有执行完毕就退出了，可能会造成数据丢失
    */
    ~ThreadPool()
    {
        cout<<"ThreadPool unconduct.."<<endl;
        for(auto & t : threads)
        {
            cv.notify_all();
            if(t.joinable())
                t.join();
        }
    }
    bool getStop()
    {
        return stop;
    }
    /*
    添加任务
    传入执行函数及参数，在把函数和参数bind成一个function
    把该函数加进tasks队列中，并唤醒一个线程执行该任务
    */
    template<class F,class ...Args>
    void enqueue(F &&f,Args && ...args)
    {
        function<void()> task =bind(forward<F>(f), forward<Args>(args)...);
        {
            //添加任务队列时要设置互斥
            unique_lock<mutex> lock(mtx);
            tasks.emplace(task);
        }
        cv.notify_one();
        cout<<"notify_one..."<<endl;
    }

    private:
    vector<thread> threads;
    queue<function<void()> > tasks;
    mutex mtx;
    condition_variable cv;
    bool stop;
};

class ServerTcp
{
    public:
    int init()
    {
        //创建服务端的socket
	    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    	if (listenfd == -1)
	    {
	    	perror("socket");
	    	return -1;
    	}

	    //把服务端用于通信的IP和端口绑定到socker上
    	//sockaddr_in结构体是为了强制装换成sockaddr结构体
    	struct sockaddr_in servaddr;
	    memset(&servaddr, 0, sizeof(servaddr));
	    //指定协议
	    servaddr.sin_family = AF_INET;
	    //服务端任意网卡的IP都可以用于通讯
    	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	    //指定通信端口，普通用户只能用1024以上的端口
    	servaddr.sin_port = htons(atoi("5005"));

    	//绑定服务端的IP和端口（为socket分配ip和端口）
	    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0)
    	{
	    	perror("bind");
	    	close(listenfd);
	    	return -1;
	    }
        return listenfd;
    }

    private:

};

void ServerOpt(int clientfd)
{
    //与客户端通信，接受客户端发过来的报文后，回复ok
	char buffer[1024];
	while (true)
	{
		memset(buffer,0,sizeof(buffer));
		int iret;
		//如果客户端已经断开连接，recv()函数返回0
		if ((iret = recv(clientfd, buffer, sizeof(buffer), 0)) <= 0)
		{
			cout << "iret=" << iret << endl;
			break;
		}
		cout << "接收到：" << buffer << endl;
		//生成回应报文内容
		strcpy(buffer, "ok");
		//向客户端发送回应报文
		if ((iret = send(clientfd, buffer, strlen(buffer), 0)) <= 0)
		{
			perror("send");
			break;
		}
		cout << "发送：" << buffer << endl;

	}
	close(clientfd);
}

int main()
{
    ThreadPool tp(10);
    ServerTcp st;
    int listenfd=st.init();
    //第3步，把socker设置为可连接（监听）的状态
	if (listen(listenfd, 5) != 0)
	{
		perror("listen");
		close(listenfd);
		return -1;
	}

    //输入stop之后还会再接收一次请求，因为在进入循环时accept会处于阻塞状态，作者暂时没有更好解决方法
	while(!tp.getStop())
	{
		int clientfd = accept(listenfd, 0, 0);
		if (clientfd == -1)
		{
			perror("accept");
			close(listenfd);
			return -1;
		}
		cout << "客户端已连接" << endl;
        tp.enqueue(ServerOpt,clientfd);
	}
    close(listenfd);
}
