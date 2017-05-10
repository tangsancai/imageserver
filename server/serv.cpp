#include"serv.h"
#include<iostream>
/*=============================================
 		网络方面的函数实现
=============================================*/
using namespace cv;
using namespace std;
ssize_t unp_readn(int fd,void* vptr,int n)
{
	size_t nleft=n;
	char *ptr=(char*)vptr;	
	ssize_t nread;
	while(nleft>0)
	{
		if(nread=read(fd,ptr,nleft)<0)
		{
			if(errno==EINTR)
				nread=0;
			else 
				return -1;
		}
		else if(nread==0)
			break;
		nleft-=nread;
		ptr+=nread;
	}
	return (n-nleft);
}
bool unp_setnoblocking(int sockfd)
{
	int opts;
	opts=fcntl(sockfd,F_GETFL);
	if(opts>=0)
	{
		opts=opts|O_NONBLOCK;
		if(fcntl(sockfd,F_SETFL,opts)>=0)
			return true;
	}
	return false;	
}
ssize_t unp_writen(int fd,const void *vptr,size_t n)
{
	size_t nleft=n;
	ssize_t nwriten;
    	const char *ptr=(const char*)vptr;
	while(nleft>0)
	{
        	if((nwriten=write(fd,ptr,nleft))<=0) 
		{
            		if(nwriten<0&&errno==EINTR) 
                		nwriten=0;
            		else 
         			return -1;
            	}
        	nleft-=nwriten;
        	ptr+=nwriten;
    	}
    	return n;
}
/*============================================
 		线程池方面的函数实现
============================================*/
void* pool_route(void *arg)
{
	POOL *tmp=(POOL*)arg;
	while(true)
	{
		pthread_mutex_lock(&tmp->mu);
		while(tmp->q.empty()&&!tmp->shutdown)
			pthread_cond_wait(&tmp->co,&tmp->mu);
		if(tmp->shutdown)
		{
			pthread_mutex_unlock(&tmp->mu);
			pthread_exit(NULL);
		}
		POOLTASK pt=tmp->q.front();
		tmp->q.pop();
		pthread_mutex_unlock(&tmp->mu);
		(pt.function)(pt.arg);		
	}
	pthread_exit(NULL);
	return NULL;	
}
void pool_init(POOL &p,int number)
{
	pthread_mutex_init(&p.mu,NULL);
	pthread_cond_init(&p.co,NULL);
	p.num=number;
	p.shutdown=false;
	if(p.num<=0)
		p.num=1;
	else if(p.num>100)
		p.num=100;
	for(int i=0;i<p.num;++i)
		pthread_create(&p.pid[i],NULL,pool_route,&p);
}
void pool_addtask(POOL &p,void* (*function)(void *arg),void *arg)
{
	POOLTASK tmp(function,arg);
	p.q.push(tmp);
	pthread_cond_signal(&p.co);
}
void pool_destroy(POOL &p)
{
	p.shutdown=true;
	pthread_cond_broadcast(&p.co);
	for(int i=0;i<p.num;++i)
		pthread_join(p.pid[i],NULL);
	pthread_mutex_destroy(&p.mu);
	pthread_cond_destroy(&p.co);
}
/*============================================
		FEATURE函数实现
============================================*/
FEATURE::FEATURE(char *name,int tag)
{
	string strname(name);
	if(tag==1)//from jpg
	{
		Mat img=imread(strname,0);
		int minHessian=300;
		SurfFeatureDetector detector(minHessian);
		vector<KeyPoint> keypoint;
		detector.detect(img,keypoint);
		SurfDescriptorExtractor extractor;
		extractor.compute(img,keypoint,descriptors);
	}
	else//from xml
	{
		FileStorage fs(strname,FileStorage::READ);
		fs["surf"]>>descriptors;
		fs.release();
	}
}
Mat FEATURE::feature_getdescriptors()
{
	return descriptors;
}
void FEATURE::feature_storagedescriptors(char* name)
{
	string strname(name);
	FileStorage fs(strname,FileStorage::WRITE);
	fs<<"surf"<<descriptors;
	fs.release();
	
}
double FEATURE::feature_computesimilar(Mat &descriptors2)
{
	double mindist=100000.0;
        if(descriptors.empty()||descriptors2.empty())
                return 2*mindist;
	FlannBasedMatcher matcher;
	vector<DMatch> matchepoints;
	matcher.match(descriptors,descriptors2,matchepoints);
	double result=0.0;
	for(unsigned int i=0;i<matchepoints.size();++i)
		if(matchepoints[i].distance<mindist)
			mindist=matchepoints[i].distance;
	int num=0;
	for(unsigned int i=0;i<matchepoints.size();++i)
		if(matchepoints[i].distance<=2*mindist)    
		{
			result+=matchepoints[i].distance;
			++num;
		}
	result/=num;
	return result;
}
/*===========================================
	     SERVERFENODE函数实现
===========================================*/
SERVERFEANODE::SERVERFEANODE()
{
	DIR *dir;
	struct dirent *ptr;
	char path[40];
	memset(path,'\0',sizeof(path));
	strcpy(path,"./sxml");
	if((dir=opendir(path))!=NULL)
	{
		while((ptr=readdir(dir))!=NULL)
		{
			if(strcmp(ptr->d_name,".")==0||strcmp(ptr->d_name,"..")==0)
				continue;
			else
			{
				char name2[40];//sxml
				char name3[40];//simg
				memset(name2,'\0',sizeof(name2));
				memset(name3,'\0',sizeof(name3));
				strcpy(name2,"./sxml/");
				strcat(name2,ptr->d_name);			
				strcpy(name3,"./simg/");
				char *nametmp=ptr->d_name;
				char *name3tmp=name3;
				name3tmp+=3;
				while(*name3tmp!='/')
					++name3tmp;
				++name3tmp;
				while(*nametmp!='.')
				{
					*name3tmp=*nametmp;
					++name3tmp;++nametmp;	
				}
				strcat(name3,".jpg");
				FEATURE	tmp(name2,2);
				fea.push_back(tmp.feature_getdescriptors());
				string strname(name3);
				filename.push_back(strname);
			}
		}
	}	
	closedir(dir);
	memset(path,'\0',sizeof(path));
	strcpy(path,"./simgupload");
	if((dir=opendir(path))!=NULL)
	{
		while((ptr=readdir(dir))!=NULL)
		{
			if(strcmp(ptr->d_name,".")==0||strcmp(ptr->d_name,"..")==0)
				continue;
			else
			{
				char name2[40];//sxml
				char name3[40];//simg
				char name1[40];//simgupload
				memset(name2,'\0',sizeof(name2));
				memset(name3,'\0',sizeof(name3));
				memset(name1,'\0',sizeof(name1));
				strcpy(name1,"./simgupload/");
				strcat(name1,ptr->d_name);
				strcpy(name3,"./simg/");
				strcat(name3,ptr->d_name);
				
				strcpy(name2,"./sxml/");
				char *nametmp=ptr->d_name;
				char *name2tmp=name2;
				name2tmp+=3;
				while(*name2tmp!='/')
					++name2tmp;
				++name2tmp;
				while(*nametmp!='.')
				{
					*name2tmp=*nametmp;
					++name2tmp;++nametmp;	
				}
				strcat(name2,".xml");
				FEATURE	tmp(name1,1);
				fea.push_back(tmp.feature_getdescriptors());
				tmp.feature_storagedescriptors(name2);
				rename(name1,name3);
				string strname(name3);
				filename.push_back(strname);
			}
		}
	}	
	closedir(dir);
}
void SERVERFEANODE::serverfeanode_getnode(int &i,Mat &featmp,string &filenametmp)
{	
	
	if(i<fea.size())
	{
		featmp=fea[i];
		filenametmp=filename[i];
	}
	else
		i=-1;
}
/*============================================
		MUTEX函数实现
============================================*/
MUTEX::MUTEX(pthread_mutex_t *tmp):mu(tmp)
{
	pthread_mutex_lock(mu);
}
MUTEX::~MUTEX()
{
	pthread_mutex_unlock(mu);
	mu=NULL;
}
/*============================================
		LOG函数实现
============================================*/
LOG::LOG():fp(NULL)
{
	pthread_mutex_init(&mu,NULL);
	fp=fopen("server_log","wb");
	if(fp==NULL)
		fprintf(stderr,"falied open log\n");
}
LOG::~LOG()
{
	fclose(fp);
	pthread_mutex_destroy(&mu);
}	
void LOG::log_debug(char *info,char *arg,long int pid,bool type)
{
//	volatile 
	MUTEX g(&mu);
	char tmp[100];
	memset(tmp,'\0',sizeof(tmp));
	strcpy(tmp,"TIME: ");
	time_t timep;
	time(&timep);
	strcat(tmp,ctime(&timep));
	if(type)
		strcat(tmp,"[DEBUG][");
	else
		strcat(tmp,"[LOG][");
	char tmppid[10];
	sprintf(tmppid,"%ld",pid);
	strcat(tmp,tmppid);
	strcat(tmp,"]:");
	strcat(tmp,info);
	if(arg!=NULL)
	{
		strcat(tmp,arg);
		strcat(tmp,"\n");			
	}
	fwrite(tmp,strlen(tmp),1,fp);
}
void LOG::log_writebackfile()
{
//	volatile 
	MUTEX g(&mu);
	int fd=fileno(fp);
	fflush(fp);
	fsync(fd);
}
/*============================================
		DATA函数实现
============================================*/
DATA::DATA():log(NULL),sockfd(-1){}
DATA::DATA(LOG *logtmp,int sockfd):log(logtmp),sockfd(sockfd)
{
	memset(filename,'\0',sizeof(filename));
	strcpy(filename,"simgtmp/");
	char picname[7];
	sprintf(picname,"%d",sockfd);	
	int i=0;
	while(i<7)
	{
		if(picname[i]=='\0')
			break;
		filename[8+i]=picname[i];
			++i;
	}
	filename[i+8]='.';
	filename[i+9]='j';
	filename[i+10]='p';
	filename[i+11]='g';
	filename[i+12]='\0';
}
//void DATA::data_search_send_image(SERVERFEANODE *const fea)
void DATA::data_searchimage(SERVERFEANODE *const fea)
{
	char printstr[40];
	memset(printstr,'\0',sizeof(printstr));
	strcpy(printstr,"start search analogous image\n");
	log->log_debug(printstr,NULL,(long int)pthread_self(),false);
	FEATURE picfea(filename,1);
	Mat picdes=picfea.feature_getdescriptors();
	int i=0;
	while(true)
	{
		Mat tmp1;
		string tmp2;
		double tmp3;
		fea->serverfeanode_getnode(i,tmp1,tmp2);
		if(i==-1)
			break;
		++i;
		tmp3=picfea.feature_computesimilar(tmp1);
		feasimnode g(tmp2,tmp3);
		if(vq.size()<10)
			vq.push(g);
		else if(vq.top().similar>tmp3)
		{
			vq.pop();
			vq.push(g);
		}
		else
			continue;
	}
	memset(printstr,'\0',sizeof(printstr));
	strcpy(printstr,"search analogous image over\n");
	log->log_debug(printstr,NULL,(long int)pthread_self(),false);
}
void DATA::data_sendimage()
{
	char printstr[40];
	memset(printstr,'\0',sizeof(printstr));
	strcpy(printstr,"start send image\n");
	log->log_debug(printstr,NULL,(long int)pthread_self(),false);
	while(!vq.empty())
	{
		char picname[40];
		memset(picname,'\0',sizeof(picname));
		strcpy(picname,vq.top().filename.c_str());
		
		FILE *ulf=fopen(picname,"rb");
		if(ulf==NULL)
		{
			memset(printstr,'\0',sizeof(printstr));
			strcpy(printstr,"failed open file:\n");
			strcat(printstr,picname);
			log->log_debug(printstr,NULL,(long int)pthread_self(),true);
			close(sockfd);
			return;
		}
		long filelen;//计算文件长度
		fseek(ulf,0,SEEK_END);
		filelen=ftell(ulf);
		rewind(ulf);
		char flen[10];
		memset(flen,'\0',sizeof(flen));
		sprintf(flen,"%d",filelen);
		unp_writen(sockfd,flen,10);//至此发送文件大小	
		char picturename[20];
		memset(picturename,'\0',sizeof(picturename));
		for(int i=7;i<vq.top().filename.size();++i)
			picturename[i-7]=vq.top().filename[i];
		unp_writen(sockfd,picturename,20);//send filename
		long left=filelen;
		char send[MAXLINE];
		while(left>MAXLINE)
		{
			fread(send,MAXLINE,1,ulf);
			unp_writen(sockfd,send,MAXLINE);
			left-=MAXLINE;
		}
		if(left<=MAXLINE)
		{
			fread(send,left,1,ulf);
			unp_writen(sockfd,send,left);			
		}
		fclose(ulf);
		sleep(1);
		vq.pop();
	}
	memset(printstr,'\0',sizeof(printstr));
	strcpy(printstr,"send image complete!\n");
	log->log_debug(printstr,NULL,(long int)pthread_self(),false);
	close(sockfd);
}
char* DATA::data_getfilename()
{
	return filename;
}
int DATA::data_getsockfd()
{
	return sockfd;
}
bool DATA:: data_loadimage()
{
	char recv[MAXLINE];
	memset(recv,'\0',sizeof(recv));
	char printstr[40];
	strcpy(printstr,"start load image\n");
	log->log_debug(printstr,NULL,(long int)pthread_self(),false);
	char flen[7];
	if((int)read(sockfd,flen,7)<=0)
	{
		strcpy(printstr,"image len error\n");
		log->log_debug(printstr,NULL,(long int)pthread_self(),true);
		return false;
	}
	strcpy(printstr,"the size of image is: ");
	log->log_debug(printstr,flen,(long int)pthread_self(),false);
	int bytes=atoi(flen);	
	if(bytes>0)
	{
		FILE *dlf=NULL;
		dlf=fopen(filename,"wb");
		if(dlf==NULL)
		{
			strcpy(printstr,"failed open file\n");
			log->log_debug(printstr,NULL,(long int)pthread_self(),true);
			return false;
		}
		strcpy(printstr,"start recv image: ");
		log->log_debug(printstr,filename,(long int)pthread_self(),false);
		while(bytes>0)
		{
			int rd=read(sockfd,recv,MAXLINE);
			if(rd>0)
			{
				bytes-=rd;
				fwrite(recv,rd,1,dlf);
			}
		}		
		fclose(dlf);
		strcpy(printstr,"write done\n\n\n");
		log->log_debug(printstr,NULL,(long int)pthread_self(),false);
	}
	else
	{
		strcpy(printstr,"bytes: 0\n\n\n");
		log->log_debug(printstr,NULL,(long int)pthread_self(),true);
		return false;
	}
	return true;
}		
/*============================================
		QUEUE函数实现
============================================*/
QUEUE::QUEUE()
{
	pthread_mutex_init(&qmu,NULL);
}
QUEUE::~QUEUE()
{
	pthread_mutex_destroy(&qmu);
}
bool QUEUE::queue_gettask(DATA& d)
{
	volatile MUTEX g(&qmu);
	if(q.empty())
		return false;
	d=q.front();
	q.pop();
	return true;	
}
void QUEUE::queue_pushtask(DATA d)
{
	volatile MUTEX g(&qmu);
	q.push(d);
}
bool QUEUE::queue_empty()
{
	volatile MUTEX g(&qmu);
	return q.empty();
}
/*============================================
		SERVER函数实现
============================================*/
SERVER::SERVER()
{
	struct sockaddr_in servaddr;//初始化监听套接字，开启“被动打开”状态	
	listenfd=socket(AF_INET,SOCK_STREAM,0);
	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	servaddr.sin_port=htons(SERV_PORT);
	bind(listenfd,(struct sockaddr *)&servaddr,sizeof(servaddr));
	listen(listenfd,LISTENQ);

	epollfd=epoll_create(LISTENQ+1);//初始化epoll，并将监听套接字加入epoll
	struct epoll_event ev;
	ev.data.fd=listenfd;
	ev.events=EPOLLIN|EPOLLET;
	epoll_ctl(epollfd,EPOLL_CTL_ADD,listenfd,&ev);	
}
SERVER::~SERVER()
{
	close(listenfd);
	close(epollfd);
}
int SERVER::server_waitevent()
{
	int fdnum=epoll_wait(epollfd,events,EPOLLMAX,-1);
	return fdnum;
}
void SERVER::server_addlink()
{
	struct sockaddr_in clientaddr;
	socklen_t clilen;
	int connfd=accept(listenfd,(sockaddr *)&clientaddr,&clilen);
	char printstr[40];
	if(connfd>0)
	{
		
		if(unp_setnoblocking(connfd))
		{
			int kltag=1;
			int kidle=150;
			int kinterval=75;
			int kcount=2;
			int tag=setsockopt(connfd,SOL_SOCKET,SO_KEEPALIVE,(char*)&kltag,sizeof(kltag));
			tag=setsockopt(connfd,IPPROTO_TCP,TCP_KEEPIDLE,(void*)&kidle,sizeof(kidle));
			tag=setsockopt(connfd,IPPROTO_TCP,TCP_KEEPINTVL,(void*)&kinterval,sizeof(kinterval));
			tag=setsockopt(connfd,IPPROTO_TCP,TCP_KEEPCNT,(void*)&kcount,sizeof(kcount));
			if(tag!=0)
			{
				strcpy(printstr,"set socket keepalive failed\n");	
				log.log_debug(printstr,NULL,(long int)pthread_self(),true);
			}
			const int on=1;
			tag=setsockopt(connfd,IPPROTO_TCP,TCP_NODELAY,(int*)&on,sizeof(int));
			if(tag==-1)
			{
				strcpy(printstr,"set socket nodelay failed\n");	
				log.log_debug(printstr,NULL,(long int)pthread_self(),true);
			}
			struct epoll_event ev;
			ev.data.fd=connfd;
			ev.events=EPOLLIN|EPOLLET|EPOLLONESHOT;
			epoll_ctl(epollfd,EPOLL_CTL_ADD,connfd,&ev);
		}
		else
		{
			strcpy(printstr,"setnblocking failed\n");
			log.log_debug(printstr,NULL,(long int)pthread_self(),true);
		}
	}
	else
	{
		strcpy(printstr,"accept connect failed\n");	
		log.log_debug(printstr,NULL,(long int)pthread_self(),true);
	}
}
bool SERVER::server_isnewlink(int i)
{
	return events[i].data.fd==listenfd;
}
bool SERVER::server_isnewdata(int i)
{
	return events[i].events&EPOLLIN;
}
bool SERVER::server_isvaliddata(int i)
{
	int sockfd=events[i].data.fd;
	if(sockfd>0)
		return true;
	char printstr[40];
	strcpy(printstr,"get sockfd failed\n");
	log.log_debug(printstr,NULL,(long int)pthread_self(),true);	
	return false;
}
void SERVER::server_writelogback()
{
	log.log_writebackfile();
}
void SERVER::server_addreadtask(int i)
{
	DATA d(&log,events[i].data.fd);
	readq.queue_pushtask(d);	
}
bool SERVER::server_getreadtask(DATA &d)
{
	return readq.queue_gettask(d);	
}
void SERVER::server_addcounttask(DATA d)
{
	countq.queue_pushtask(d);	
}
bool SERVER::server_getcounttask(DATA &d)
{
	return countq.queue_gettask(d);	
}
void SERVER::server_addwritetask(DATA d)
{
	writeq.queue_pushtask(d);	
}
bool SERVER::server_getwritetask(DATA &d)
{
	return writeq.queue_gettask(d);	
}
void SERVER::server_debug(char *info)
{
	log.log_debug(info,NULL,(long int)pthread_self(),true);
}
bool SERVER::server_readqempty()
{
	return readq.queue_empty();
}
bool SERVER::server_countqempty()
{
	return countq.queue_empty();
}
bool SERVER::server_writeqempty()
{
	return writeq.queue_empty();
}
SERVER *SIGLENTON::srv=NULL;
pthread_mutex_t SIGLENTON::lock1;
SERVERFEANODE *SIGLENTON::fea=NULL;
pthread_mutex_t SIGLENTON::lock2;
