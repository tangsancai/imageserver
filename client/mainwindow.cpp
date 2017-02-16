#include "mainwindow.h"
#include "ui_mainwindow.h"
#include<unistd.h>
#include<errno.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<arpa/inet.h>
#include<string>
#include<iostream>
#include<QtDebug>
#include<QString>
#include<QMovie>

#define SERV_PORT 9877
#define MAXLINE 4096
using namespace std;
ssize_t unpluo_writen(int fd, const void *vptr, size_t n)
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


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}
int sockfd;
int mypos;
bool MyConnect(char *patharg)
{
    struct sockaddr_in	servaddr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    inet_pton(AF_INET, patharg, &servaddr.sin_addr);

    if(connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))<0)
        return false;
    return true;
}
char cimgpath[20][40];
void recv_pic(int sockfd)
{
    memset(cimgpath,'\0',sizeof(cimgpath));
    int num=0;
    char cimgpathtmp[20][40];
    memset(cimgpathtmp,'\0',sizeof(cimgpathtmp));

    for(int nn=0;nn<20;++nn)
    {
        char recv[MAXLINE];
        char flen[10];
        if((int)read(sockfd,flen,10)<=0)
            break;
        char filename[20];
        memset(filename,'\0',sizeof(filename));
        read(sockfd,filename,20);
        char picpath[40];
        memset(picpath,'\0',sizeof(picpath));
        strcpy(picpath,"./cimg/");
        strcat(picpath,filename);
        strcpy(cimgpathtmp[num++],picpath);
        int bytes=atoi(flen);
        if(bytes>0)
        {
            FILE *dlf=NULL;
            dlf=fopen(picpath,"wb");
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
        }
    }
    int numtmp=num;
    for(int i=0;i<num;++i)
        strcpy(cimgpath[i],cimgpathtmp[--numtmp]);
}

void str_cli(char* imgpath)
{
    char send[MAXLINE];
    char *filename=strtok(imgpath,"\n");//需要去除\n否则文件打开失败
    FILE *ulf=fopen(filename,"rb");
    long filelen;//计算文件长度
    fseek(ulf,0,SEEK_END);
    filelen=ftell(ulf);
    rewind(ulf);
    char flen[7];
    sprintf(flen,"%d\n",filelen);
    unpluo_writen(sockfd,flen,7);//至此发送文件大小
    long left=filelen;
    while(left>MAXLINE)
    {
        fread(send,MAXLINE,1,ulf);
        unpluo_writen(sockfd,send,MAXLINE);
        left-=MAXLINE;
    }
    if(left<=MAXLINE)
    {
        fread(send,left,1,ulf);
        unpluo_writen(sockfd,send,left);
    }
    fclose(ulf);
    recv_pic(sockfd);
    close(sockfd);
    return;
}

char imgpatharg[40];
char serverpath[40];
void MainWindow::on_searchid_clicked()
{
    system("rm ./cimg/*");
    MyConnect(serverpath);
    str_cli(imgpatharg);

    if(cimgpath[0][0]!='\0')
    {
        QMovie *movie=new QMovie(QLatin1String(cimgpath[0]));
        ui->imagelabel->setMovie(movie);
        movie->start();
        ui->imagename->setText(QLatin1String(cimgpath[0]));
        mypos=0;
    }
}

void MainWindow::on_pathid_returnPressed()
{
    QString qpath=ui->pathid->text();
    memset(imgpatharg,'\0',sizeof(imgpatharg));
    QByteArray qpatharry=qpath.toLatin1();
    char* patharg=qpatharry.data();
    int i=0;
    while(*patharg!='\0')
    {
        imgpatharg[i++]=*patharg;
        ++patharg;
    }
}

void MainWindow::on_pathid_cursorPositionChanged(int arg1, int arg2)
{
    QString qpath=ui->pathid->text();
    memset(imgpatharg,'\0',sizeof(imgpatharg));
    QByteArray qpatharry=qpath.toLatin1();
    char* patharg=qpatharry.data();
    int i=0;
    while(*patharg!='\0')
    {
        imgpatharg[i++]=*patharg;
        ++patharg;
    }
}

void MainWindow::on_serverid_cursorPositionChanged(int arg1, int arg2)
{
    QString qpath=ui->serverid->text();
    memset(serverpath,'\0',sizeof(serverpath));
    QByteArray qpatharry=qpath.toLatin1();
    char* patharg=qpatharry.data();
    int i=0;
    while(*patharg!='\0')
    {
        serverpath[i++]=*patharg;
        ++patharg;
    }
}

void MainWindow::on_serverid_returnPressed()
{
    QString qpath=ui->serverid->text();
    memset(serverpath,'\0',sizeof(serverpath));
    QByteArray qpatharry=qpath.toLatin1();
    char* patharg=qpatharry.data();
    int i=0;
    while(*patharg!='\0')
    {
        serverpath[i++]=*patharg;
        ++patharg;
    }
}

void MainWindow::on_pre_clicked()
{
    if(mypos>0&&cimgpath[mypos-1][0]!='\0')
    {
        --mypos;
        QMovie *movie=new QMovie(QLatin1String(cimgpath[mypos]));
        ui->imagelabel->setMovie(movie);
        movie->start();
        ui->imagename->setText(QLatin1String(cimgpath[mypos]));
    }
}

void MainWindow::on_post_clicked()
{
    if(mypos<19&&cimgpath[mypos+1][0]!='\0')
    {
        ++mypos;
        QMovie *movie=new QMovie(QLatin1String(cimgpath[mypos]));
        ui->imagelabel->setMovie(movie);
        movie->start();
        ui->imagename->setText(QLatin1String(cimgpath[mypos]));
    }
}
