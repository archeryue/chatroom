//coder : archer
#include "chat.h"
#include <pthread.h>

typedef struct user
{
	char name[8];//your name;
	char room[8];//your room;
	char sname[8];//private talking name;
} user_t;

static pthread_mutex_t scrlock;
static char sendbuf[MAXBUF+1];
static user_t *userinfo=NULL;
static int isend=0;

void printmsg(char *buf)
{
	char *p,*jude,*room,*srcname;
	char *out;
	jude=buf+2;
	switch (*jude)
	{
		case '1':
			out=jude+2;
			p=jude+1;
			if (*p!='\0')
			{
				printf("room list:\n");
				p++;
				while(*p!='\0')
				{
					if (*p=='!')
					{
						*p='\0';
						printf("%s ",out);
						p++;
						out=p;
					}
					else
						p++;
				}
				printf("%s\n\n",out);
			}
			else
			{
				printf("there is no room now,you can create new room by -m newroom.\n\n");
			}
			break;
		case '2':
			p=jude+1;
			if (*p=='s')
				printf("create new room success\n\n");
			else
				printf("create new room fail\n\n");
			break;
		case '3':
			p=jude+1;
			if (*p=='s')
				printf("enter room success\n\n");
			else
			{ 
				if (*p=='f')
					printf("maybe you should change your name.try -n\n\n");
				else
					printf("there is no such room,try -l\n\n");
				userinfo->room[0]='\0';
			}
			break;
		case '4':
			p=jude+1;
			if (*p=='s')
			{
				printf("exit room success\n\n");
				userinfo->room[0]='\0';
			}
			else
				printf("create new room fail\n\n");
			break;
		case '5':
			p=jude+1;room=p;
			while (*p!='!') p++;
			*p='\0';
			p++;srcname=p;
			while (*p!='!') p++;
			*p='\0';
			p++;out=p;
			if (strcmp(userinfo->room,room)==0)
				printf("%s : %s\n",srcname,out);
			break;
		case '6':
			p=jude+1;room=p;
			while (*p!='!') p++;
			*p='\0';
			p++;srcname=p;
			while (*p!='!') p++;
			*p='\0';
			p++;out=p;
			if (strcmp(userinfo->room,room)==0)
				printf("[s]%s : %s\n",srcname,out);
			break;
		default:
			break;
	}	
}

int getmsg(char *buf)
{
	char * p;
	int flag=0;
	if (*buf!='-')
	{
		printf("try -h for help\n\n");
		return 0;
	}
	p=buf+1;
	switch (*p)
	{
		case 'l':
			sprintf(sendbuf,"c01");
			flag=1;
			break;
		case 'n':
			p+=2;
			if (strlen(p)>7 || strlen(p)<1 || *(p-1)=='\0')
			{
				printf("your name should be shorter then 7 chars and longer then 1 char\n\n");
			}
			else
			{
				if (userinfo->room[0]!='\0')
					printf("you can not change your name,while chating\n\n");
				else
				{
					strcpy(userinfo->name,p);
					printf("get name success\n\n");
				}
			}
			break;
		case 'm':
			p+=2;
			if (strlen(p)>7 || strlen(p)<1 || *(p-1)=='\0')
			{
				printf("room name should be shorter then 7 chars and longer then 1 char\n\n");
			}
			else
			{
				sprintf(sendbuf,"c02%s",p);
				flag=1;
			}
			break;
		case 'i':
			if (userinfo->name[0]=='\0')
			{
				printf("you should have a name,try -n\n\n");
			}
			else
			{
				if(userinfo->room[0]!='\0')
				{
					printf("you must leave your room first.\n\n");
				}
				else
				{
					p+=2;
					strcpy(userinfo->room,p);
					sprintf(sendbuf,"c03%s!%s",userinfo->room,userinfo->name);
					flag=1;
				}
			}
			break;
		case 'o':
			if (userinfo->room[0]!='\0')
			{
				p+=2;
				sprintf(sendbuf,"c04%s!%s",userinfo->room,userinfo->name);
				flag=1;
			}
			else
			{
				printf("you are not in room\n\n");
			}
			break;
		case 't':
			if (userinfo->room[0]!='\0')
			{
				printf("input content:\n");
				gets(buf);
				sprintf(sendbuf,"c05%s!%s!%s",userinfo->room,userinfo->name,buf);
				flag=1;
			}	
			else
			{
				printf("you must in a room first\n\n");
			}
			break;
		case 's':
			if (userinfo->room[0]!='\0')
			{
				p+=2;
				strcpy(userinfo->sname,p);
				printf("intput content:\n");
				gets(buf);
				sprintf(sendbuf,"c06%s!%s!%s!%s",userinfo->room,userinfo->name,userinfo->sname,buf);
				flag=1;
			}	
			else
			{
				printf("you must in a room first\n\n");
			}
			break;
		case 'h':
			printf("welcome to chatroom!\nusage:\n\t-l :show rooms' list;\n\t-n username:get your name,len<8;\n\t-m roomname:create a new room,len<8;\n\t-i roomname:enter a room;\n\t-o :exit room;\n\t-t :talk;\n\t-s username:private talk to other user;\n\t-h help infomation;\nmore help information,please read README.thankyou\n\n");
			break;
		default :
			printf("try -h for help\n\n");
			break;
	}
	
	return flag;	
	
}

void * listen_process(void *arg)
{
	struct sockaddr_in my_addr;
	struct sockaddr_in ser_addr;
	int fd;
	int len;
	socklen_t addr_len;
	char recvbuf[MAXBUF+1];

	
	if ((fd=socket(AF_INET,SOCK_DGRAM,0)) == -1)
	{
		perror("socket");
		exit(1);
	}
	
	bzero(&my_addr,sizeof(my_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(CLIPORT);
	my_addr.sin_addr.s_addr = INADDR_ANY;

	if((bind(fd,(struct sockaddr *)&my_addr,sizeof(my_addr)))==-1)
	{
		perror("bind");	
		exit(1);
	}

	while(!isend)
	{
		len = recvfrom(fd,recvbuf,MAXBUF,0,(struct sockaddr *)&ser_addr,&addr_len);
		
		if (len<0)
		{
			perror("recvfrom");
			exit(1);
		}
		
		printmsg(recvbuf);
	}
	pthread_exit(NULL);
}

int main()
{
	int sendfd;
	char recvbuf[MAXBUF+1];
	char *buf;
	socklen_t len;
	int buflen;
	struct sockaddr_in ser_addr;
	pthread_t id;
	
	if(pthread_create(&id,NULL,listen_process,NULL)<0)	
	{
		perror("pthread create");
		exit(1);
	}

	if ((sendfd=socket(AF_INET,SOCK_STREAM,0)) == -1)
	{
		perror("socket");
		exit(1);
	}
	
	bzero(&ser_addr,sizeof(ser_addr));
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_port = htons(SERPORT);
	ser_addr.sin_addr.s_addr = inet_addr(SERIP);
	
	if (connect(sendfd,(struct sockaddr *)&ser_addr,sizeof(struct sockaddr)) == -1)
	{
		perror("connect");
		exit(1);
	}	
	

	if ((len=recv(sendfd,recvbuf,MAXBUF,0))<=0)
	{
		perror("recv");
		exit(1);
	}
	else
	{
		printf("\n%s\n",recvbuf);
	}

	userinfo=(user_t *)malloc(sizeof(user_t));
	userinfo->name[0]='\0';
	userinfo->room[0]='\0';	

	int quit=0;
	int is_send;
	buf=(char *)malloc(MAXBUF*sizeof(char));
	while(!quit)
	{	
		gets(buf);
		if (strcmp(buf,"-q")==0) 
			quit=1;
		else
			is_send=getmsg(buf);

		if (is_send)
		{
			if ((len=send(sendfd,sendbuf,MAXBUF,0))<=0)
			{
				perror("send");
				exit(1);
			}
		}
		sendbuf[0]='\0';
	}

	printf("bye\n");
	isend=1;

	//pthread_join(id,NULL);
	free(buf);
	free(userinfo);
	close(sendfd);
	return 0;
}
