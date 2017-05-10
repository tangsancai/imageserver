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
#include "opencv2/core/core.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <opencv2/nonfree/features2d.hpp>
#include "opencv2/opencv.hpp"
#include <opencv2/nonfree/nonfree.hpp>
#include <opencv2/legacy/legacy.hpp>

#define MAXLINE 4096
#define EPOLLMAX 1024
#define SERV_PORT 9877
#define LISTENQ 1024

/*============================================
UNP：网络库方面函数
============================================*/
ssize_t unp_readn(int fd,void* vptr,int n);
bool unp_setnoblocking(int sockfd);
ssize_t unp_writen(int fd,const void *vptr,size_t n);
/*=============================================
POOL：线程池方面函数
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
SERVERFEANODE
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
DATA：数据处理类，包括下载图片，找出匹配图片，发送匹配图片
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
