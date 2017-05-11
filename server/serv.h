#ifndef _SERV_H_
#define _SERV_H_

#include<unistd.h>
#include<errno.h>
#include<sys/socket.h>
#include<netinet/tcp.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/epoll.h>
#include<fcntl.h>
#include<pthread.h>
#include<queue>
#include<signal.h>
#include<time.h>
#include<string>
#include<dirent.h>
#include"opencv2/core/core.hpp"
#include"opencv2/features2d/features2d.hpp"
#include"opencv2/highgui/highgui.hpp"
#include<opencv2/nonfree/features2d.hpp>
#include"opencv2/opencv.hpp"
#include<opencv2/nonfree/nonfree.hpp>
#include<opencv2/legacy/legacy.hpp>
#include<iostream>

#define MAXLINE 4096
#define EPOLLMAX 1024
#define SERV_PORT 9877
#define LISTENQ 1024

/*============================================
UNP：网络库方面函数
unp_readn：读多个字节
unp_setnoblocking：设置套接字非阻塞
unp_writen：写多个字节，避免频繁系统调用write（保存上下文，耗时）
============================================*/
ssize_t unp_readn(int fd,void* vptr,int n);
bool unp_setnoblocking(int sockfd);
ssize_t unp_writen(int fd,const void *vptr,size_t n);
/*=============================================
POOL：线程池方面函数
pool_init：初始化线程池，开number个的线程，函数为pool_route
pool_route：这些线程阻塞在条件变量，接阻塞后用函数指针调用实际执行函数
pool_addtask：添加任务到任务队列，并唤醒阻塞的线程
pool_destory：销毁线程池
=============================================*/
struct POOLTASK
{
	void* (*function)(void*);
	void* arg;
	POOLTASK(){}
	POOLTASK(void*(*fun)(void*),void* a):function(fun),arg(a){}
};
struct  POOL
{
	pthread_t pid[100];
	pthread_mutex_t mu;
	pthread_cond_t co;
	std::queue<POOLTASK> q;
	int num;
	bool shutdown;
};
void* pool_route(void *arg);
void pool_init(POOL &p,int number);
void pool_addtask(POOL &p,void* (*function)(void *arg),void *arg);
void pool_destroy(POOL &p);
/*=============================================
FEATURE：特征相关
feature_getdescriptors：获得目标图片的特征向量
feature_computesimilar：计算目标图片与特征库各个特征向量的相似度
=============================================*/
class FEATURE
{
private:
	FEATURE(const FEATURE&);
	FEATURE& operator=(const FEATURE&);
	cv::Mat descriptors;
public:
	FEATURE(char *name,int tag);
	cv::Mat feature_getdescriptors();
	void feature_storagedescriptors(char *name);
	double feature_computesimilar(cv::Mat &descriptors2);
};
/*============================================
MUTEX：提供安全使用锁的方法
实 现：在构造函数中锁住互斥量，在析构函数中解锁互斥量。
       如此，只需定义MUTEX tmp的临时变量就可以完成上锁
       解锁，不会出现忘记解锁，重复解锁的问题	
============================================*/
class MUTEX
{
private:
	pthread_mutex_t *mu;
	MUTEX(const MUTEX&);
	MUTEX& operator=(const MUTEX&);
public:
	MUTEX(pthread_mutex_t *tmp);
	~MUTEX();
};
/*============================================
LOG：日志的具体实现
log_debug：将信息打印到日志文件中
log_writebackfile：将内存中的日志信息同步到硬盘
============================================*/
class LOG
{
private:
	FILE *fp;
	pthread_mutex_t mu;
	LOG(const LOG&);
	LOG& operator=(const LOG&);
public:
	LOG();
	~LOG();
	void log_debug(char *info,char *arg,long int pid,bool type);
	void log_writebackfile();
};
/*============================================
SERVERFEANODE：特征结构体
serverfeanode_getnode：获取特定图片的特征向量
============================================*/
class SERVERFEANODE
{
private:
	std::vector<cv::Mat> fea;
	std::vector<std::string> filename;
	SERVERFEANODE(const SERVERFEANODE&);
 SERVERFEANODE& operator=(const SERVERFEANODE&);
public:
	SERVERFEANODE();
	void serverfeanode_getnode(int &i,cv::Mat &featmp,std::string &filenametmp);
};
/*============================================
feasimnode：相似度排序使用的结构体

DATA：流转于三个任务队列的任务类
data_loadimage：接收客户端发来的图片
data_searchimage：计算图片的特征向量，并在特征库中寻找top10相似度的图片
data_sendimage：将top10相似度的图片发回给客户端
data_getsockfd：获取对端sockfd
data_getfilename：获取本任务的目标图片名
============================================*/
struct feasimnode
{
	std::string filename;
	double similar;
	feasimnode(std::string tmp1,double tmp2):filename(tmp1),similar(tmp2){}
	friend bool operator<(feasimnode tmp1,feasimnode tmp2)
	{
		return tmp1.similar<tmp2.similar;
	}
};
class DATA
{
private:
	LOG *log;//全局的打印文件句柄
	int sockfd;//需要往sock读写，所以做成成员变量
	char filename[20];//需要提取地特征向量的文件
	std::priority_queue<feasimnode> vq;//存放相似图片的名称
public:
	DATA();
	DATA(LOG *logtmp,int sockfd);
	bool data_loadimage();
	void data_searchimage(SERVERFEANODE *const fea);
	void data_sendimage();
	int data_getsockfd();
	char* data_getfilename();
};
/*============================================
QUEUE：任务队列的具体实现
queue_gettask：在任务队列中获取任务。队列弹出已被获取的任务
queue_pushtask：将任务压入队列
queue_empty：判断任务队列空否
============================================*/
class QUEUE
{
private:
	std::queue<DATA> q;
	pthread_mutex_t qmu;
	QUEUE(const QUEUE&);
	QUEUE& operator=(const QUEUE&);
public:
	QUEUE();
	~QUEUE();
	bool queue_gettask(DATA& d);
	void queue_pushtask(DATA d);
	bool queue_empty();
};
/*============================================
SERVER：用于管理服务器的启动、日志、任务队列等信息
server_waitevent：服务端监听开启后，epoll wait事件发生
server_isnewlink：判断是新连接来了
server_addlink：如果是新连接，那么添加监听
server_isnewdata：判断是数据来了
server_isvaliddata：判断数据是否有效
server_writelogback：内存日志信息定时写回硬盘方法
server_addreadtask：添加任务到读任务队列
server_getreadtask：从读任务队列中摘下任务
server_addcounttask：添加计算任务到计算任务队列
server_getcounttask：从计算任务队列中摘下任务
server_addwritetask：添加写任务到写任务队列
server_getwritetask：从写任务队列中摘下任务
server_readqempty：判断读任务队列是否为空
server_countqempty：判断计算任务队列是否为空
server_writeqempty：判断写任务队列是否为空
server_debug：信息写到内存的日志信息中
============================================*/
class SERVER
{
private:
	QUEUE readq;
	QUEUE countq;
	QUEUE writeq;
	LOG log;
	int listenfd;
	int epollfd;
	struct epoll_event events[EPOLLMAX];
	SERVER(const SERVER&);
	SERVER& operator=(const SERVER&);
public:
	SERVER();
	~SERVER();
	int server_waitevent();
	void server_addlink();
	bool server_isnewlink(int i);
	bool server_isnewdata(int i);
	bool server_isvaliddata(int i);
	void server_writelogback();
	void server_addreadtask(int i);
	bool server_getreadtask(DATA &d);
	void server_addcounttask(DATA d);
	bool server_getcounttask(DATA &d);
	void server_addwritetask(DATA d);
	bool server_getwritetask(DATA &d);
	void server_debug(char *info);
	bool server_readqempty();
	bool server_countqempty();
	bool server_writeqempty();
};

struct serv_arg
{
	DATA d;
	SERVER *srv;
	serv_arg(DATA dtmp,SERVER *stmp):d(dtmp),srv(stmp){};
	serv_arg(){};
};
/*=============================================
SIGLENTON：单例，使server/feature在本进程中只有一个实例
siglenton_getserver：获取服务器对象
siglenton_getserverfeanode：获取特征库对象
siglenton_deleteserver：销毁服务器对象
siglenton_deleteserverfeanode：销毁特征库对象
==============================================*/
class SIGLENTON
{
private:
	static SERVER *srv;
	SIGLENTON();
	SIGLENTON(const SIGLENTON &);
	SIGLENTON& operator=(const SIGLENTON &);
	static pthread_mutex_t lock1;
	
	static SERVERFEANODE *fea;
	static pthread_mutex_t lock2;
public:
	static SERVER* siglenton_getserver()
	{
		if(srv==NULL)
		{
			MUTEX mu1(&lock1);
			if(srv==NULL)
				srv=new SERVER();
		}
		return srv;
	}
	static SERVERFEANODE* siglenton_getserverfeanode()
	{
		if(fea==NULL)
		{
			MUTEX mu2(&lock2);
			if(fea==NULL)
				fea=new SERVERFEANODE();
		}
		return fea;
	}
	static void siglenton_deleteserver()
	{
		if(srv!=NULL)
			delete srv;
		srv=NULL;
	}
	static void siglenton_deleteserverfeanode()
	{
		if(fea!=NULL)
			delete fea;
		fea=NULL;
	}
};
#endif
