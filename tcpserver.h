#ifndef TCPSERVER_H
#define TCPSERVER_H
#include "datastruct.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <unistd.h>
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>


#include <QObject>
#include <QThread>
#include <QMutex>
#include <unistd.h>
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>



#define MAXSIZE 1024


class TcpServer : public QObject
{
    Q_OBJECT
public:
    explicit TcpServer(QObject *parent = 0);
    virtual ~TcpServer();
    void init();
    MainWindow *mw;
private:
    QThread acceptthread;
    QThread srthread;
    QThread getMsgThread;
    int socket_fd;
    int accept_fd;
    int port;
    sockaddr_in myserver;
    sockaddr_in remote_addr;
    RecvBuf recvbuf;
    QMutex mutexbuf;
     list<Msg> msgList;
     QMutex mutex;
     list<RecvStream> recvStreamList;
     QMutex mutexRecvStream;
     const static int m_timeout=2500000;
     const static int m_resendtimes=3;
     const static string HEX_DIGITS[16];


     string toHexStr(unsigned char* bytes, int len);
     int toOriginalMsg(unsigned char * composed,int comlen, unsigned char * original, int *origlen);
     int toComposedMsg(unsigned char * original,int origlen, unsigned char * composed, int* comlen);
     int addCheckCode(unsigned char * original , int len);
     int checkCode(unsigned char * original, int len);
     Msg * getMsgToSend();
     int  handleMsgList();
     void handleRegister(RecvStream* prs);
     void handleAuthentication(RecvStream* prs);
signals:
    void addPacket(string,string,string);
public slots:
    void sendrecv();
    void acceptsocket();
    void getMsgFromBuf();
};

#endif // TCPSERVER_H
