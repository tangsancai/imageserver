#include "serv.h"

using namespace cv;
using namespace std;

void* handleread(void *arg);//调度读任务队列+具体处理读任务
void* serv_countimage(void *arg);//具体计算图片的特征向量+寻找相似图片
void* handlecount(void *arg);//调度计算任务队列
void* handlewrite(void *arg);//调度写任务队列+具体处理写任务
void* handlewritelogback(void *arg);//日志定时写回磁盘，防止服务器突然崩溃，无法查询近期日志
void* handlesig(void *arg);
void setthreadsigmask(SERVER *srv);
void process(int signo);

SERVERFEANODE *const fea=SIGLENTON::siglenton_getserverfeanode();
/*==========================================================
函数名：main
作  用：
	1.在服务器开启之前，将特征库调入内存：因为所有待检索图
	片的特征向量都需要与特征库中的特征向量逐一计算相似度。
	2.在服务器开启之前，开辟读图片、计算特征向量+相似度、发
	回图片、日志写回、信号处理5个线程
	3.开启服务器，接收检索请求
	4.接收到检索请求，将接收客户端的任务装填到读任务队列中
==========================================================*/
int main(int argc, char **argv)
{	
	SERVER *srv=SIGLENTON::siglenton_getserver();
	setthreadsigmask(srv);
	pthread_t pid;
	pthread_create(&pid,NULL,handleread,srv);//开子线程处理读任务
	pthread_create(&pid,NULL,handlecount,srv);//开子线程处理计算任务队列
	pthread_create(&pid,NULL,handlewrite,srv);//开子线程处理写任务
	pthread_create(&pid,NULL,handlewritelogback,srv);//开子线程处理日志的定时写回磁盘
	pthread_create(&pid,NULL,handlesig,srv);//开子线程专门用于处理信号
	cout<<"open server complete!"<<endl;
	while(true)
	{
		int fdnum=srv->server_waitevent();
		for(int i=0;i<fdnum;++i)
		{
			if(srv->server_isnewlink(i))
				srv->server_addlink();
			else if(srv->server_isnewdata(i)&&srv->server_isvaliddata(i))
				srv->server_addreadtask(i);
		}
	}
	SIGLENTON::siglenton_deleteserver();
	SIGLENTON::siglenton_deleteserverfeanode();
	return 0;
}
/*============================================================
函数名：handlread
作  用：用于接收客户端发来的图片。
实  现：不断循环检测读任务队列，如果检测到读任务队列为空，则
	sleep 1秒，防止线程空转；如果检测到读任务队列非空，摘
	下读任务执行接收图片的操作。最后添加计算任务到计算任务
	队列中
说  明：此处的读任务调度线程和处理线程合成了一个。原因是：计算
	任务耗时特别长，读图片的过程远小于计算过程，所以我认为
	只开一个读任务处理线程慢慢处理就行，按流水线来看，后面的
	计算任务等得起。
=============================================================*/
void* handleread(void *arg)
{
	SERVER *srv=(SERVER*)arg;
	setthreadsigmask(srv);
	while(true)
	{
		if(srv->server_readqempty())
			sleep(1);
		else
		{
			DATA d;
			srv->server_getreadtask(d);
			if(d.data_loadimage())
				srv->server_addcounttask(d);
			else
			{	
				char printstr[20];
				strcpy(printstr,"loadimage failed\n");
				srv->server_debug(printstr);
			}
		}
	}
	return NULL;
}
/*============================================================
函数名：handlecount
作  用：计算任务队列调度函数
实  现：循环检测计算任务队列，如果检测到计算任务队列为空，则
	sleep1秒，防止线程空转；如果检测到读任务队列非空，就从
	线程池中取出工作线程去处理具体的计算任务。
说  明：线程池存放线程为8个。原因：cpu核12,（常活的epoll主线程占
	一个），（以IO为主的读任务+长期阻塞的信号处理线程合占一
	个），（定时唤醒的日志写回线程+计算任务调度线程合占一个）
	，（写任务线程占一个），所以计算任务的线程池放了12-4=8
	个线程。
	1.IO繁忙型：线程个数设成2N
	2.cpu繁忙型：线程个数设成N
	故将2个IO为主的线程合在一起算占1个线程
============================================================*/
void* handlecount(void *arg)
{
	SERVER *srv=(SERVER*)arg;
	setthreadsigmask(srv);
	POOL pool;
	pool_init(pool,8);
	while(true)
	{
		if(srv->server_countqempty())
			sleep(1);
		else
		{
			DATA d;
			srv->server_getcounttask(d);
			serv_arg a(d,srv);
			pool_addtask(pool,serv_countimage,&a);
		}
	}
	return NULL;
}
/*============================================================
函数名：serv_countimage
作  用：计算图片的特征向量，将该特征向量与特征库相比对（计算相
	似度）。
=============================================================*/
void* serv_countimage(void *arg)
{
	serv_arg argtmp=*((serv_arg*)arg);
	DATA d=argtmp.d;
	SERVER *srv=argtmp.srv;
	setthreadsigmask(srv);
	d.data_searchimage(fea);
	srv->server_addwritetask(d);
	return NULL;
}
/*=========================================================
函数名：handlewrite
作  用：将相似度top10的图片发回给客户端
==========================================================*/
void* handlewrite(void *arg)
{
	SERVER *srv=(SERVER*)arg;
	setthreadsigmask(srv);
	while(true)
	{
		if(srv->server_writeqempty())
			sleep(1);
		else
		{
			DATA d;
			srv->server_getwritetask(d);
			d.data_sendimage();
		}
	}
	return NULL;
}
/*=======================================================
函数名：handlewritelogback
作  用：定时写回错误日志
实  现：线程sleep2秒，将内存中的日志信息强制写回硬盘，即使
	是服务器突然意外，也可以查错误日志进行排错
========================================================*/
void* handlewritelogback(void *arg)
{
	SERVER *srv=(SERVER*)arg;
	setthreadsigmask(srv);
	while(true)
	{
		sleep(2);
		srv->server_writelogback();
	}
	return NULL;
}
/*=======================================================
函数名：setthreadsigmask
作  用：设置线程信号屏蔽字
说  明：线程们拥有自己的信号屏蔽字，但线程们共享信号处理函
	数。设置线程信号屏蔽字是为了防止信号频繁打断慢系统
	调用，设置专门信号处理线程来处理这些非硬件引起的信
	号
========================================================*/
void  setthreadsigmask(SERVER *srv)
{
	sigset_t set;
	sigfillset(&set);
	sigdelset(&set,SIGQUIT);
	sigdelset(&set,SIGFPE);
	sigdelset(&set,SIGBUS);
	sigdelset(&set,SIGILL);
	sigdelset(&set,SIGSEGV);
	int s=pthread_sigmask(SIG_BLOCK,&set,NULL);
	if(s!=0)
	{
		char printstr[40];
		strcpy(printstr,"can't block sigs\n");
		srv->server_debug(printstr);
	}
	signal(SIGFPE,process);
	signal(SIGBUS,process);
	signal(SIGILL,process);
	signal(SIGSEGV,process);
}
/*========================================================
函数名：handlesig
作  用：处理非硬件引起的信号
说  明：不太懂得非硬件引起的信号该如何处理，此处仅仅是打印
	而已，防止服务器端因一个线程崩溃导致整个服务器崩溃
=========================================================*/
void* handlesig(void *arg)
{
	SERVER *srv=(SERVER*)arg;
	sigset_t set;
	sigfillset(&set);
	while(true)
	{
		int sig,s;
		s=sigwait(&set,&sig);
		if(s!=0)
		{
			char printstr[40];
			strcpy(printstr,"worry with sigwait\n");
			srv->server_debug(printstr);
		}
		char sigg[20];
		sprintf(sigg,"catch sig:%d\n",sig);
		srv->server_debug(sigg);
	}
	return NULL;
}
/*=========================================================
函数名：process
作  用：处理硬件引起的信号，杀死这些线程，以防止线程崩溃导致
	整个服务器崩溃
==========================================================*/
void process(int signo)
{
	fprintf(stderr,"catch signo[%d],exit pthread[%ld]",signo,pthread_self());
	char g[10]="exit";
	pthread_exit(g);
}
