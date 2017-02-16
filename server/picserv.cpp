#include"serv.h"

using namespace cv;
using namespace std;

void* serv_readimage(void *arg);//具体处理某个读任务
void* handleread(void *arg);//管理读任务队列
void* serv_countimage(void *arg);
void* handlecount(void *arg);
void* handlewritelogback(void *arg);//日志定时写回磁盘，防止服务器突然崩溃，无法查询近期日志
void* handlesig(void *arg);//专门用于处理各种信号
void setthreadsigmask(SERVER *srv);
void process(int signo);

SERVERFEANODE *const fea=new SERVERFEANODE();

int main(int argc, char **argv)
{	
	SERVER srv;
	setthreadsigmask(&srv);
	pthread_t pid;
	pthread_create(&pid,NULL,handleread,&srv);//开子线程处理【读任务】队列
	pthread_create(&pid,NULL,handlecount,&srv);//开子线程处理【检索任务+写任务】队列
	pthread_create(&pid,NULL,handlewritelogback,&srv);//开子线程处理日志的定时写回磁盘
	pthread_create(&pid,NULL,handlesig,&srv);//开子线程专门用于处理信号
	cout<<"open server complete!"<<endl;
	while(true)
	{
		int fdnum=srv.server_waitevent();
		for(int i=0;i<fdnum;++i)
		{
			if(srv.server_isnewlink(i))
			{
				srv.server_addlink();
			}
			else if(srv.server_isnewdata(i)&&srv.server_isvaliddata(i))
			{
				srv.server_addreadtask(i);
			}
		}
	}
	return 0;
}
void process(int signo)
{
	fprintf(stderr,"catch signo[%d],exit pthread[%ld]",signo,pthread_self());
	char g[10]="exit";
	pthread_exit(g);
}
void* serv_readimage(void *arg)
{
	serv_arg argtmp=*((serv_arg*)arg);
	DATA d=argtmp.d;
	SERVER *srv=argtmp.srv;
	setthreadsigmask(srv);
//	int j=2;
//	j/=0;
	if(d.data_loadimage())
		srv->server_addcounttask(d);
	else
	{
		char printstr[20];
		strcpy(printstr,"loadimage failed\n");
		srv->server_debug(printstr);
	}
	return NULL;
}
void* handleread(void *arg)
{
	SERVER *srv=(SERVER*)arg;
	setthreadsigmask(srv);
	POOL pool;
	pool_init(pool,20);
	while(true)
	{
		if(srv->server_readqempty())
			sleep(1);
		else
		{
			DATA d;
			srv->server_getreadtask(d);
			serv_arg a(d,srv);
			pool_addtask(pool,serv_readimage,&a);
		}
	}
	pool_destroy(pool);
	return NULL;
}

struct feasimnode
{
	string filename;
	double similar;
	feasimnode(string tmp1,double tmp2):filename(tmp1),similar(tmp2){}
	friend bool operator<(feasimnode tmp1,feasimnode tmp2)
	{
		return tmp1.similar<tmp2.similar;
	}
};

void* serv_countimage(void *arg)
{
	serv_arg argtmp=*((serv_arg*)arg);
	DATA d=argtmp.d;
	SERVER *srv=argtmp.srv;
	setthreadsigmask(srv);
	d.data_search_send_image(fea);
	return NULL;
}
void* handlecount(void *arg)
{
	SERVER *srv=(SERVER*)arg;
	setthreadsigmask(srv);
	while(true)
	{
		if(srv->server_countqempty())
			sleep(1);
		else
		{
			DATA d;
			srv->server_getcounttask(d);
			serv_arg a(d,srv);
			while(true)
			{
				pthread_t pid;
				int res=pthread_create(&pid,NULL,serv_countimage,&a);
				if(res!=0)
					sleep(2);
				else
					break;
			}
		}
	}
	return NULL;
}

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

