//coder : archer

#include "chat.h"
#include "threadpool.h"

mythread_pool *pool=NULL;
//thread pool
static chat_t *chat=NULL;
//chat room;
static node_t *mems=NULL;
//all mems in chatroom

void chat_init() {
    chat=(chat_t *)malloc(sizeof(chat_t));

    chat->head=NULL;
    chat->num=0;
    chat->firstroom=NULL;
    pthread_rwlock_init(&(chat->roomlock),NULL);
}

int chat_destroy() {
    pthread_rwlock_destroy(&(chat->roomlock));
    return 0;
}

static room_t * newroom(char * name) {
    room_t * theroom=NULL;

    theroom=(room_t *)malloc(sizeof(room_t));

    strcpy(theroom->roomname,name);
    theroom->num=0;
    theroom->roommate=NULL;
    theroom->next=NULL;
    theroom->firstip=NULL;


    pthread_rwlock_init(&(theroom->rblock),NULL);
    pthread_rwlock_init(&(theroom->listlock),NULL);

    return theroom;
}
//make a new room;

int add_new_room(char * name) {
    roomlist *p,*fp;
    room_t * new_room,*end;

    pthread_rwlock_wrlock(&(chat->roomlock));
    p=chat->firstroom;

    while (p!=NULL) {
        if (strcmp(p->roomname,name) == 0) {
            pthread_rwlock_unlock(&(chat->roomlock));
            return 0;
        }
        fp=p;
        p=p->next;
    }
    p=(roomlist *)malloc(sizeof(roomlist));
    strcpy(p->roomname,name);
    p->next=NULL;
    new_room=newroom(name);
    if (chat->num>0) {
        end=chat->head;
        while(end->next!=NULL) end=end->next;
        end->next = new_room;
        fp->next=p;
    } else {
        chat->head=new_room;
        chat->firstroom=p;
    }
    chat->num++;
    pthread_rwlock_unlock(&(chat->roomlock));

    return 1;
}
//make the new room into chatroom;

static unsigned int SDBMhash(const char * str) {
    unsigned int hash = 0;

    while(*str) {
        hash=(*str++)+(hash<<6)+(hash<<16)-hash;
    }

    return hash;
}
//hash function,translate username into a key num;
int gethash(const char * name) {
    return SDBMhash(name)%HASHNUM;
}

int enter_room(char * roomname,char *name,char *ip) {
    room_t * findroom;
    key_t key;
    iplist * new;

    key=gethash(name);

    pthread_rwlock_rdlock(&(chat->roomlock));
    //adding a readlock is enough
    findroom=chat->head;

    while(findroom!=NULL && (strcmp(findroom->roomname,roomname)!=0)) findroom=findroom->next;

    pthread_rwlock_unlock(&(chat->roomlock));
    //findout if the room exists;
    if (findroom==NULL) {
        return -1;
    }

    pthread_rwlock_wrlock(&(findroom->rblock));

    if (rb_search(key,findroom->roommate)!=NULL) {
        pthread_rwlock_unlock(&(findroom->rblock));
        return 0;
    }//findout if the same username exists;
    findroom->roommate=rb_insert(key,name,ip,findroom->roommate);

    rb_print(findroom->roommate);
    pthread_rwlock_unlock(&(findroom->rblock));
    //insert new user into specified room;

    pthread_rwlock_wrlock(&(findroom->listlock));

    new=(iplist*)malloc(sizeof(iplist));
    strcpy(new->ip,ip);
    new->next=findroom->firstip;
    findroom->firstip=new;

    pthread_rwlock_unlock(&(findroom->listlock));

    //updata ip list;
    return 1;
}

int out_room(char *roomname,char *name,char *ip) {
    room_t * findroom;
    key_t key;
    iplist *tmp,*f;

    key=gethash(name);
    pthread_rwlock_rdlock(&(chat->roomlock));
    //adding a readlock is enough
    findroom=chat->head;
    while(findroom!=NULL && (strcmp(findroom->roomname,roomname)!=0)) findroom=findroom->next;
    pthread_rwlock_unlock(&(chat->roomlock));
    //findout if the room exists;
    if (findroom==NULL) return -1;

    pthread_rwlock_wrlock(&(findroom->rblock));
    findroom->roommate=rb_delete(key,findroom->roommate);
    pthread_rwlock_unlock(&(findroom->rblock));
    //delete user from specified room;

    pthread_rwlock_wrlock(&(findroom->listlock));
    tmp=findroom->firstip;
    while(tmp!=NULL && (strcmp(ip,tmp->ip)!=0)) {
        f=tmp;
        tmp=tmp->next;
    }
    if (tmp==findroom->firstip) {
        findroom->firstip=tmp->next;
    } else {
        f->next=tmp->next;
    }
    free(tmp);
    pthread_rwlock_unlock(&(findroom->listlock));
    //updata ip list;
    return 1;
}

void * talk(void *arg) {
    char * buf=((arg_t *)arg)->buf;
    int fd=((arg_t *)arg)->fd;
    char *p,*ip,*room,*name,*content;
    char sendbuf[MAXBUF+1];
    room_t *find;
    iplist *list;
    int len;
    struct sockaddr_in c_addr;


    p=buf;ip=p;
    while(*p!='!') p++;
    *p='\0';
    p+=4;room=p;
    while(*p!='!') p++;
    *p='\0';
    p++;name=p;
    while(*p!='!') p++;
    *p='\0';
    p++;content=p;

    sprintf(sendbuf,"s05%s!%s!%s\n",room,name,content);

    pthread_rwlock_rdlock(&(chat->roomlock));
    find=chat->head;
    while(find!=NULL && strcmp(find->roomname,room)!=0) find=find->next;
    pthread_rwlock_unlock(&(chat->roomlock));
    //find the right room

    if (find == NULL) {
        perror("fault room");
        exit(1);
    }

    bzero(&c_addr,sizeof(c_addr));
    c_addr.sin_family=AF_INET;
    c_addr.sin_port = htons(CLIPORT);
    pthread_rwlock_rdlock(&(find->listlock));
    list=find->firstip;
    while(list!=NULL) {
        c_addr.sin_addr.s_addr=inet_addr(list->ip);
        len = sendto(fd,sendbuf,MAXBUF,0,(struct sockaddr *)&c_addr,sizeof(c_addr));
        if (len < 0) {
            perror("sendto client");
        }

        list=list->next;
    }
    pthread_rwlock_unlock(&(find->listlock));
}

void *private_talk(void *arg) {
    char * buf=((arg_t *)arg)->buf;
    int fd=((arg_t *)arg)->fd;
    char *p,*ip,*room,*srcname,*dstname,*content;
    char sendbuf[MAXBUF+1];
    room_t *find;
    node_t * dstnode;
    int len;
    struct sockaddr_in c_addr;


    p=buf;ip=p;
    while(*p!='!') p++;
    *p='\0';
    p+=4;room=p;
    while(*p!='!') p++;
    *p='\0';
    p++;srcname=p;
    while(*p!='!') p++;
    *p='\0';
    p++;dstname=p;
    while(*p!='!') p++;
    *p='\0';
    p++;content=p;

    sprintf(sendbuf,"s06%s!%s!%s\n",room,srcname,content);

    pthread_rwlock_rdlock(&(chat->roomlock));
    find=chat->head;
    while(find!=NULL && strcmp(find->roomname,room)!=0) find=find->next;
    pthread_rwlock_unlock(&(chat->roomlock));
    //find the right room

    if (find == NULL) {
        perror("fault room");
        exit(1);
    }

    bzero(&c_addr,sizeof(c_addr));
    c_addr.sin_family=AF_INET;
    c_addr.sin_port = htons(CLIPORT);

    pthread_rwlock_rdlock(&(find->listlock));

    dstnode=rb_search(gethash(dstname),find->roommate);
    //find dstuser's ip;
    if (dstnode == NULL) {
        perror("no dst");
    } else {
        c_addr.sin_addr.s_addr=inet_addr(dstnode->ip);
        len = sendto(fd,sendbuf,MAXBUF,0,(struct sockaddr *)&c_addr,sizeof(c_addr));
        if (len < 0) {
            perror("sendto client");
        }
    }
    pthread_rwlock_unlock(&(find->listlock));
}

void * operate(void *arg) {
    char * buf=((arg_t *)arg)->buf;
    int fd=((arg_t *)arg)->fd;
    char *p,*ip,*room,*name,*jude;
    char sendbuf[MAXBUF+1];
    int len;
    struct sockaddr_in c_addr;
    roomlist *list;
    char tmp_list[MAXBUF+1];
    int flage;

    p=buf;ip=p;
    while(*p!='!') p++;
    *p='\0';
    jude=p+3;
    switch (*jude) {
        case '1':
            //get room list;
            pthread_rwlock_rdlock(&(chat->roomlock));
            list=chat->firstroom;
            strcpy(tmp_list,"s01");
            while(list!=NULL)
            {
                sprintf(sendbuf,"%s!%s",tmp_list,list->roomname);
                strcpy(tmp_list,sendbuf);
                list=list->next;
            }
            pthread_rwlock_unlock(&(chat->roomlock));
            strcpy(sendbuf,tmp_list);
            break;
        case '2':
            //create a new room;
            p=jude+1;room=p;
            if (add_new_room(room))
                sprintf(sendbuf,"s02s");
            else
                sprintf(sendbuf,"s02f");
            break;
        case '3':
            //enter room;
            p=jude+1;room=p;
            while(*p!='!') p++;
            *p='\0';
            p++;name=p;
            if ((flage=enter_room(room,name,ip))==1)
                sprintf(sendbuf,"s03s");
            else
            {
                if (flage==0)
                    sprintf(sendbuf,"s03f");
                else
                    sprintf(sendbuf,"s03w");
            }
            break;
        case '4':
            //exit room;
            p=jude+1;room=p;
            while(*p!='!') p++;
            *p='\0';
            p++;name=p;
            if (out_room(room,name,ip))
                sprintf(sendbuf,"s04s");
            else
                sprintf(sendbuf,"s04f");
            break;
        default:
            sprintf(sendbuf,"e");
            break;
    }
    bzero(&c_addr,sizeof(c_addr));
    c_addr.sin_family=AF_INET;
    c_addr.sin_port = htons(CLIPORT);
    c_addr.sin_addr.s_addr=inet_addr(ip);
    len = sendto(fd,sendbuf,MAXBUF,0,(struct sockaddr *)&c_addr,sizeof(c_addr));

    if (len < 0) {
        perror("sendto client");
    }
}

int handle_msg(int fd,char * buf) {
    node_t * tmp;
    char * buf_arg;

    tmp=rb_search(fd,mems);
    if (tmp==NULL) {
        perror("impossble");
        return -1;
    }

    buf_arg=(char *)malloc(MAXBUFARG*sizeof(char));
    sprintf(buf_arg,"%s!%s",tmp->ip,buf);

    switch (*(buf+2)) {
        case '5':pool_add_worker(talk,buf_arg);break;
        case '6':pool_add_worker(private_talk,buf_arg);break;
        default :pool_add_worker(operate,buf_arg);break;
    }

    return 0;
}

int setnonblocking(int sockfd) {
    int flags;

    if ((flags=fcntl(sockfd,F_GETFL,0))==-1) {
        perror("fcntl");
        return -1;
    }

    flags = flags | O_NONBLOCK;

    if (fcntl(sockfd,F_SETFL,flags)==-1) {
        perror("fcntl");
        return -1;
    }

    return 0;
}

int main() {
    char recvbuf[MAXBUF+1];
    int listenfd,new_fd,epfd;
    int i,ret,curfdnum,evnum;
    socklen_t len;
    int buflen;
    int backlognum=2;
    struct sockaddr_in my_addr,remote_addr;
    struct epoll_event ev;
    struct epoll_event events[MAXEPOLLSIZE];
    struct rlimit rt;

    char new_ip[16];
    char roomname[8];
    char username[8];

    //init thread pool
    pool_init(MAXPOOLSIZE);
    //init chat
    chat_init();

    rt.rlim_max = rt.rlim_cur = MAXEPOLLSIZE;

    if (setrlimit(RLIMIT_NOFILE,&rt) == -1)    {
        //set the max num of fd.
        perror("setrlimit");
        exit(1);
    } else {
        printf("rlimit success\n");
    }

    if ((listenfd = socket(AF_INET,SOCK_STREAM,0)) == -1) {
        perror("listenfd");
        exit(1);
    } else {
        printf("listenfd success\n");
    }

    setnonblocking(listenfd);
    //set nonblocking.

    bzero(&my_addr,sizeof(my_addr));
    my_addr.sin_family=AF_INET;
    my_addr.sin_port = htons(SERPORT);
    my_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenfd,(struct sockaddr *)&my_addr,sizeof(struct sockaddr)) == -1) {
        perror("bind");
        exit(1);
    } else {
        printf("bind success\n");
    }

    if (listen(listenfd,backlognum) == -1) {
        perror("listen");
        exit(1);
    } else {
        printf("listen success\n");
    }

    epfd = epoll_create(MAXEPOLLSIZE);
    len = sizeof(struct sockaddr_in);
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = listenfd;

    if (epoll_ctl(epfd,EPOLL_CTL_ADD,listenfd,&ev) < 0) {
        perror("epoll_ctl");
        exit(1);
    } else {
        printf("epoll_ctl listenfd_add success\n");
    }
    curfdnum = 1;

    while(1) {
        if ((evnum = epoll_wait(epfd,events,curfdnum,-1)) == -1) {
            perror("epoll_wait");
            break;
        }

        for(i=0;i<evnum;i++) {
            if (events[i].data.fd == listenfd) {
                //add new socket
                new_fd = accept(listenfd,(struct sockaddr *)&remote_addr,&len);
                if (new_fd < 0) {
                    perror("accept");
                    continue;
                } else {
                    strcpy(new_ip,inet_ntoa(remote_addr.sin_addr));
                    printf("src ip:%s src port:%d fd:%d\n",new_ip,ntohs(remote_addr.sin_port),new_fd);
                    setnonblocking(new_fd);
                    ev.events = EPOLLIN | EPOLLET;
                    ev.data.fd = new_fd;
                    if (epoll_ctl(epfd,EPOLL_CTL_ADD,new_fd,&ev)<0) {
                        perror("epoll_ctl newfd");
                        continue;
                    } else {
                        curfdnum++;
                        printf("add newfd success\n");
                        send(new_fd,"welcome!\n",MAXBUF,0);
                        mems=rb_insert(new_fd,"\0",new_ip,mems);
                    }
                }
            } else {
                //recvive from client
                bzero(recvbuf,MAXBUF+1);

                buflen=recv(events[i].data.fd,recvbuf,MAXBUF,0);

                if (buflen <= 0) {
                    if (errno != 11) {
                        epoll_ctl(epfd,EPOLL_CTL_DEL,events[i].data.fd,&ev);
                        curfdnum--;
                        printf("delfd success\n");
                    }
                } else {
                    //handle recv msg;
                    printf("%s\n",recvbuf);
                    handle_msg(events[i].data.fd,recvbuf);
                }        
            }
        }
    }

    pool_destroy();
    chat_destroy();

    close(listenfd);
    return 0;
}
