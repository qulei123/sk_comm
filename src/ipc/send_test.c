/* send.c */
#include <mqueue.h>
#include <stdbool.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#define MAXSIZE 1024
int main(int argc,char**argv)
{
    if(argc!=2)
    {
        printf("Usage: %s /mqname \n", argv[0]);
        return -1;
    }
    char Msg[MAXSIZE]="Hello World";
    char *name = argv[1];

    int flags = O_RDWR | O_CREAT | O_EXCL ;
    mode_t mode = S_IRUSR | S_IWUSR| S_IRGRP |S_IROTH;

    struct mq_attr attr;
    attr.mq_flags=0;
    attr.mq_maxmsg=10;
    attr.mq_msgsize=100;
    attr.mq_curmsgs=0;
    //mq_unlink(name);
    mqd_t mqid = mq_open(name, flags, mode, &attr);
    if (mqid == -1)
    {
        if (EEXIST == errno)
        {
            mqid = mq_open(name, O_RDWR);
            if (mqid == -1)
            {
                printf("mq_open error %s (%d)\r\n", strerror(errno), errno);        
                return -1;
            }
        }
        else
        {
            printf("mq_open error %s (%d)\r\n", strerror(errno), errno);        
            return -1;
        }
    }

    int i;
    for(i=0;i<5;i++)
    {
        if(mq_send(mqid,Msg,strlen(Msg),i)==-1)
        {
            perror("mq_send error");
            return -1;
        }
    }

    mq_close(mqid);

    return 0;
}
