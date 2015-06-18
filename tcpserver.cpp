#include "tcpserver.h"

const string TcpServer::HEX_DIGITS[16]={"0","1","2","3","4","5","6","7","8","9","a","b","c","d","e","f"};
TcpServer::TcpServer(QObject *parent) :
    QObject(parent)
{


}
TcpServer::~TcpServer()
{
    close(socket_fd);
}


void TcpServer::init()
{
        port = 8899;
        if(( socket_fd = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP)) < 0 ){
                throw "socket() failed";
        }

        memset(&myserver,0,sizeof(myserver));
        myserver.sin_family = AF_INET;
        myserver.sin_addr.s_addr = htonl(INADDR_ANY);
        myserver.sin_port = htons(port);

        if( bind(socket_fd,(sockaddr*) &myserver,sizeof(myserver)) < 0 ) {
            printf("bind socket error: %s(errno:%d)\n)",strerror(errno),errno);
             fflush(stdout);
                           return ;                                                      ;
        }

        if( listen(socket_fd,10) < 0 ) {
                throw "listen() failed";
        }
        this->moveToThread(&acceptthread);
        connect(&acceptthread,SIGNAL(started()),this,SLOT(acceptsocket()));
        acceptthread.start();

}
void TcpServer::getMsgFromBuf()
{
    RecvStream recvstream,rs;
    static int t=0;
    int r=0;
    while(1)
    {
        mutexRecvStream.lock();
        if(recvbuf.size>0)
        {
            r=recvbuf.getDataFromBuf(recvstream.stream, &(recvstream.size));
            printf("recvbuf size %d,r %d,recvstream.size %d\n", recvbuf.size,r,recvstream.size);
        }
        mutexRecvStream.unlock();
        if(r)
        {
            for(int i=0; i<recvstream.size;i++)
                printf("%02x ", recvstream.stream[i]);
            printf("\n");
            fflush(stdout);
            r =0;
            toOriginalMsg(recvstream.stream,recvstream.size, rs.stream,&rs.size);
            int j = checkCode(rs.stream,rs.size);
            if( j < 0)
            {
                continue;
            }
            if(j ==1)
            {
                MsgHeader header;
                int i = header.fromStream(rs.stream);
                string s,s1,s2;
                s = toHexStr((unsigned char*)header.phoneNumber,6);
                //mw->ui->tableWidget->setItem(t,0,new QTableWidgetItem(s.c_str()));
                WORD tmp=ntohs(header.msgId);

                s1 = toHexStr((unsigned char*)&(tmp),2);
                //mw->ui->tableWidget->setItem(t,1,new QTableWidgetItem(s.c_str()));
                s2 = toHexStr(rs.stream,rs.size);
                //mw->ui->tableWidget->setItem(t,3,new QTableWidgetItem(s1.c_str()));
                emit addPacket(s,s1,s2);
                t++;

                //cout << "msgid "<<hex<<header.msgId<<endl;
                switch(header.msgId)
                {
                    case 0x0100: handleRegister(&rs);break;
                    case 0x0102: handleAuthentication(&rs);break;
                    default:break;
                }
            }

        }
    }
}
void TcpServer::handleRegister(RecvStream* prs)
{
    Msg msg;
    RegisterAck ra;
    MsgHeader head;
    head.fromStream(prs->stream);
    ra.header = head;
    ra.header.msgId = 0x8100;
    ra.sn=head.msgSerialNumber;
    ra.authenticationCode ="abcde";
    ra.result = 0;
    ra.header.property = 21;
    unsigned char ori[1024];
    int len;
    len = ra.toStream(ori);
    len = addCheckCode(ori,len);

    toComposedMsg(ori,len, msg.stream, &(msg.len));
    msg.isAck =true;
    mutex.lock();
    msgList.push_front(msg);
    mutex.unlock();

}

void TcpServer::handleAuthentication(RecvStream* prs)
{
    Msg msg;
    PlatformAck pa;
    pa.header.fromStream(prs->stream);
    pa.serialNumber=pa.header.msgSerialNumber;
    pa.header.msgId = 0x8001;
    pa.result = 0;
    pa.msgId = 0x0102;
    unsigned char ori[1024];
    int len;
    len = pa.toStream(ori);
    len = addCheckCode(ori,len);
    toComposedMsg(ori,len, msg.stream, &(msg.len));
    msg.isAck =true;
    mutex.lock();
    msgList.push_front(msg);
    mutex.unlock();


}

void TcpServer::acceptsocket()
{

        while( 1 ) {

                socklen_t sin_size = sizeof(struct sockaddr_in);
                if(( accept_fd = accept(socket_fd,(struct sockaddr*) &remote_addr,&sin_size)) == -1 )
                {
                        throw "Accept error!";
                        continue;
                }

                this->moveToThread(&srthread);
                connect(&srthread,SIGNAL(started()),this,SLOT(sendrecv()));
                srthread.start();

        }
        close(accept_fd);
        return ;
}
Msg * TcpServer::getMsgToSend()
{
    Msg *pmsg=NULL, *pmsg1 = NULL;
    struct timeval endtime;
    gettimeofday(&endtime, NULL);
    list<Msg>::iterator it;
//	printf("sdfdfdsfdsf\n");
    //fflush(stdout);
    mutex.lock();
    for(it=msgList.begin(); it != msgList.end(); ++it)
    {

        pmsg = &(*it);
        long int time = (endtime.tv_sec - pmsg->sendTime.tv_sec)*1000000 + (endtime.tv_usec - pmsg->sendTime.tv_usec);
        if(!(pmsg->complete))
        {
            pmsg1 = pmsg;
            break;
        }
        else if((time >= m_timeout) &&!(pmsg->isAck))
        {
            if(pmsg->resendTimes <m_resendtimes)
            {
                pmsg->sendChars = 0;
                pmsg->complete = false;
                pmsg1 = pmsg;
                break;
            }

        }
    }
    mutex.unlock();
    return pmsg1;
}
int  TcpServer::handleMsgList()
{
    WORD sn;
    Msg *pmsg = NULL;
        list<Msg>::iterator it1;
        mutex.lock();
        for(it1 = msgList.begin(); it1 != msgList.end(); )
        {
            pmsg = &(*it1);


            if((pmsg->complete)&&((pmsg->resendTimes >= m_resendtimes)||(pmsg->isAck)))
            {
                msgList.erase(it1++);
                if(pmsg->resendTimes >= m_resendtimes)
                {
                    //save msg or other things
                }

            }
            else
            {
                ++it1;
            }
        }
        mutex.unlock();



}
void TcpServer::sendrecv()
{
    int sfd=accept_fd;
    fd_set readset, writeset;
    Msg *pmsg=NULL;

    this->moveToThread(&getMsgThread);
    connect(&getMsgThread,SIGNAL(started()),this,SLOT(getMsgFromBuf()));
    getMsgThread.start();


    struct timeval timeout={0,200};
    int maxfd;
    int ret, sendChars,recvChars, r;

           while (1)
           {

             handleMsgList();


             if(pmsg == NULL)
             {
                 pmsg = getMsgToSend();
             }


             FD_ZERO(&readset);            //每次循环都要清空集合，否则不能检测描述符变化
             FD_SET(sfd, &readset);     //添加描述符
             FD_ZERO(&writeset);
             FD_SET(sfd,&writeset);

             maxfd = sfd +1;    //描述符最大值加1


            ret = select(maxfd, &readset, &writeset, NULL, &timeout);   // 阻塞模式
             switch( ret)
             {
               case -1:
                 exit(-1);
                 break;
               case 0:  //timeout

                 break;
               default:

                 if (FD_ISSET(sfd, &readset))  //测试sock是否可读，即是否网络上有数据
                 {

                    mutexRecvStream.lock();
                     recvChars = recv(sfd, recvbuf.stream+recvbuf.size, 1560-recvbuf.size,0);
                     printf("recvbuffing len:%d,%d\n", recvbuf.size,recvChars);
                     fflush(stdout);
                     if( recvChars < 0 )
                     {
                         printf("recv message error\n");
                         return ;
                     }
                     else if(recvChars == 0)
                     {
                         //网络断开，尝试重连
                     }
                     else{
                         recvbuf.size += recvChars;

                     }
                     mutexRecvStream.unlock();
                 }
                  if (FD_ISSET(sfd, &writeset))
                 {
                      if((pmsg != NULL) && (pmsg->sendChars < pmsg->len)&&(!pmsg->complete))
                      {
                          sendChars = send( sfd,pmsg->stream+pmsg->sendChars, pmsg->len - pmsg->sendChars,0 );
                          if( sendChars < 0 )
                          {
                                      printf("send message error\n");
                                      return ;
                          }
                          else
                          {
                              pmsg->sendChars += sendChars;
                          }
                      }
                      if((pmsg != NULL) && (pmsg->sendChars == pmsg->len)&&(!pmsg->complete))
                      {
                          (pmsg->resendTimes)++;
                          gettimeofday(&(pmsg->sendTime),NULL);
                          pmsg->complete = true;
                          pmsg = NULL;
                      }
                 }

             }
    }
}
int TcpServer::toOriginalMsg(unsigned char * composed,int comlen, unsigned char * original, int *origlen)
{
    int j = 0;
    int start = 0;
    int tmplen = 0;

    for(int i=0;i<comlen;i++)
    {
        if ((composed[i] == 0x7e)&&(i != comlen-1))
        {
            start = i + 1;
            continue;
        }
        if ((composed[i] == 0x7e)&&(i == comlen-1))
        {
            if(tmplen != 0)
            {
                        memcpy(&original[j], &composed[start],tmplen);
            }
            j = j + tmplen;
            continue;
        }
        if (composed[i] != 0x7d)
        {
            tmplen++;
        }
        else
        {
            printf("j=%d  tmplen= %d, start=%d\n", j, tmplen, start);
            if(tmplen != 0)
            {
                        memcpy(&original[j], &composed[start],tmplen);
            }
            if(composed[i+1] == 0x02)
            {
                original[j+tmplen] = 0x7e;
            }
            else if(composed[i+1] == 0x01)
            {
                original[j+tmplen] = 0x7d;
            }
            else
            {
                return -1;
            }
            j = j + tmplen + 1;
            start = start + tmplen + 2;
            i++;
            tmplen = 0;
        }
    }
    *origlen = j;


    return 0;
}

int TcpServer::toComposedMsg(unsigned char * original,int origlen, unsigned char * composed, int* comlen)
{
    int j = 0;
    int tmplen  = 0;
    int start = 0;

    composed[j] = 0x7e;
    j++;
    for(int i =0; i< origlen; i++)
    {
        if((original[i] != 0x7e)&&(original[i]!=0x7d))
        {
            tmplen++;
        }
        else
        {
        //printf("j=%d  tmplen= %d, start=%d\n", j, tmplen, start);
            if(tmplen != 0)
            {
                memcpy(composed+j, original+start, tmplen);
            }
            start =start+tmplen+1;
            if(original[i] == 0x7e)
            {
                composed[j+tmplen] = 0x7d;
                composed[j+tmplen+1] = 0x02;
            }
            if(original[i] == 0x7d)
            {
                composed[j+tmplen] = 0x7d;
                composed[j+tmplen+1] = 0x01;
            }

            j=j+tmplen+2;
            tmplen =0;
        }
    }

    if(tmplen != 0)
    {
        memcpy(composed+j, original+start, tmplen);

    }
    j = j + tmplen;
    composed[j]=0x7e;
    j++;

    *comlen = j;
    return 0;
}

int TcpServer::addCheckCode(unsigned char * original, int len)
{
    unsigned char c= 0x00;
    for(int i = 0;i<len; i++)
    {
        c= c ^original[i];
    }
    original[len] = c;
    return len+1;
}

int TcpServer::checkCode(unsigned char * original, int len)
{
    unsigned char c=0x00;
    if(len<2)
    {
        return -2;
    }
    for(int i=0; i<len-1;i++)
    {
        c = c^original[i];
    }
    if(original[len-1] != c)
    {
        return -1;
    }
    else
    {
        return 1;
    }
}

/**
 * convert bytes array to hex string
 */
string TcpServer::toHexStr(unsigned char* bytes, int len)
{
    int h,l;
    std::ostringstream out1;

    for(int i=0;i<len;i++) {

        h = (bytes[i]>>4)&0xf;
        l = bytes[i]&0xf;

        out1<<HEX_DIGITS[h]<<HEX_DIGITS[l]<< " ";
    }

    return out1.str();
}
