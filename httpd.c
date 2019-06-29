#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int psend(int s, const char *buf, int len, int flags);
int precv(int s, char *buf, int len, int flags);
int pclosesocket(int s);

/*从文件名获取文件类型类型 */
void gettype(const char *name, char *out)
{
    int len = strlen(name);
    switch (name[len - 1])
    {
    case 'l': /*html*/
    case 's': /*js css */
        strcpy(out, "text/html");
        break;
    default:
        strcpy(out, "application/octet-stream");
    }
}

/*向客户端发送响应头 */
void headers(int fd, const char *name)
{
    char buf[1024];
    strcpy(buf, "HTTP/1.0 200 OK\r\n");
    psend(fd, buf, strlen(buf), 0);
    strcpy(buf, "Server: pyhttpd/1.0.0\r\n");
    psend(fd, buf, strlen(buf), 0);
    char tp[30];
    gettype(name, tp);
    sprintf(buf, "Content-Type: %s\r\n", tp);
    psend(fd, buf, strlen(buf), 0);
    strcpy(buf, "\r\n");
    psend(fd, buf, strlen(buf), 0);
}

/*向客户端发送数据 */
void sendbuff(int fd, const char *buf)
{
    psend(fd, buf, strlen(buf), 0);
}

/*从response信息中获得url */
void geturl(const char *Buffer, char *out)
{
    char Buffer1[20];
    char Buffer3[20];
    sscanf(Buffer, "%s%s%s", Buffer1, out, Buffer3);
}

unsigned char FromHex(unsigned char x)
{
    unsigned char y;
    if (x >= 'A' && x <= 'Z')
        y = x - 'A' + 10;
    else if (x >= 'a' && x <= 'z')
        y = x - 'a' + 10;

    else if (x >= '0' && x <= '9')
        y = x - '0';
    else
        assert(0);
    return y;
};

/*用于URL解码 */
void UrlDecode(const char *str, char *out)
{
    out[0] = 0;
    int length = strlen(str);
    int pos = 0;
    for (size_t i = 0; i < length; i++)
    {
        if (str[i] == '+')
            out[pos++] = ' ';
        else if (str[i] == '%')
        {
            assert(i + 2 < length);
            unsigned char high = FromHex((unsigned char)str[++i]);
            unsigned char low = FromHex((unsigned char)str[++i]);
            out[pos++] = high * 16 + low;
        }
        else
            out[pos++] = str[i];
    }
    out[pos] = 0;
}

/*从url中获取文件名 */
void namefromurl(const char *Buffer, char *out)
{
    int pos = 0;
    int len = strlen(Buffer);
    int i;
    char be[200];
    for (i = 1; i < len; ++i)
        if (Buffer[i] != '?')
            be[pos++] = Buffer[i];
        else
            break;
    be[pos] = 0;
    UrlDecode(be, out);
}

/*send 404 */
void not_found(int client)
{
    char buf[1024];
    sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
    psend(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
    psend(client, buf, strlen(buf), 0);
    sprintf(buf, "your request because the resource specified\r\n");
    psend(client, buf, strlen(buf), 0);
    sprintf(buf, "is unavailable or nonexistent.\r\n");
    psend(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    psend(client, buf, strlen(buf), 0);
}
/*拷贝文本文件 */
void cattext(int client, FILE *resource)
{
    char buf[1024];
    /* 从文件文件描述符中读取指定内容 */
    fgets(buf, sizeof(buf), resource);
    while (!feof(resource))
    {
        psend(client, buf, strlen(buf), 0);
        fgets(buf, sizeof(buf), resource);
    }
}
/*拷贝非文本文件 */
void catfile(int client, FILE *resource)
{
    char buf[1024];
    int rc;
    while( (rc = fread(buf,sizeof(unsigned char), 1024,resource)) != 0 )
        psend(client, buf, rc, 0); 
}
/*向客户端发送一个文件 */
void sendfile(int fd, const char *name)
{
    char filename[300];
    filename[0] = 0;
    strcpy(filename, "filedir/");
    if (name[0] == 0)
        strcat(filename, "index.html");
    else
        strcat(filename, name);
    printf("%s\r\n", filename);
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL)
    {
        headers(fd, "404.html");
        not_found(fd);
    }
    else
    {
        char tp[30];
        gettype(filename, tp);
        headers(fd, filename);
        if (strcmp(tp, "text/html") == 0)
            cattext(fd, fp);
        else
            catfile(fd, fp);
        fclose(fp);
    }
}

/*处理来自客户端的信息 */
void deal(int fd)
{
    char Buffer[8193];
    int recvlen = precv(fd, Buffer, 8192, 0);
    if (recvlen > 0)
    { /*如果接收到数据 */
        Buffer[recvlen] = 0;
        char url[1000], name[200];
        geturl(Buffer, url);    /*获取url */
        namefromurl(url, name); /* 获取文件名 */
        sendfile(fd, name);
    }
    pclosesocket(fd);
}

int getport(void)
{
    return 9090;
}

/*以下为不可直接移植部分，每个平台均需要单独编写 */

#ifdef WIN32

#include <windows.h>
#include <process.h>
#pragma comment(lib, "Ws2_32.lib")

int psend(int s, const char *buf, int len, int flags)
{
    return send(s, buf, len, flags);
}
int precv(int s, char *buf, int len, int flags)
{
    return recv(s, buf, len, flags);
}
int pclosesocket(int s)
{
    return closesocket(s);
}
static void FdHandler(void * lpCtx)
{
    int fd = (SOCKET)lpCtx;
    deal(fd);
    _endthread();
}

int main()
{
    WORD wVersionRequested;
    WSADATA wsaData;
    int port = getport();
    int err;
    SOCKADDR_IN addr;
    int bindhand;
    SOCKET lfd;
    err = WSAStartup(MAKEWORD(1, 1), &wsaData);
    if (err != 0)
    {
        printf("error:WSAStartup failed\r\n");
        return -1;
    }
    lfd = socket(PF_INET, SOCK_STREAM, 0);
    addr.sin_addr.S_un.S_addr = 0;
    addr.sin_family = PF_INET;
    addr.sin_port = htons(port);
    bindhand = bind(lfd, (struct sockaddr *)&addr, sizeof(SOCKADDR_IN));
    if (bindhand != NOERROR)
    {
        printf("error:failed to bind:%d port\r\n", port);
        return -1;
    }
    else
    {
        printf("httpd successful,port:%d\r\n", port);
    }

    listen(lfd, 1970);
    do
    {
        bindhand = sizeof(SOCKADDR_IN);
        SOCKET fd = accept(lfd, (struct sockaddr *)&addr, &bindhand);
        printf("from [%s:%d]\r\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
        _beginthread(FdHandler, 0,(void *)fd);
    } while (1);
    WSACleanup();
    return 0;
}
#else

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <pthread.h>

int psend(int s, const char *buf, int len, int flags)
{
    return send(s, buf, len, flags);
}
int precv(int s, char *buf, int len, int flags)
{
    return recv(s, buf, len, flags);
}
int pclosesocket(int s)
{
    return close(s);
}
void FdHandler(int fd)
{
    deal(fd);
    pthread_detach(pthread_self());
}

int main(void)
{
    int server_sock = -1;
    u_short port = getport();
    int client_sock = -1;
    struct sockaddr_in client_name;
    int client_name_len = sizeof(client_name);
    pthread_t newthread;
    struct sockaddr_in name;
    server_sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&name, 0, sizeof(name));
    name.sin_family = AF_INET;
    name.sin_port = htons(port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(server_sock, (struct sockaddr *)&name, sizeof(name)) < 0)
    {
        printf("error:failed to bind:%d port\r\n", port);
        return -1;
    }else
    {
        printf("httpd successful,port:%d\r\n", port);
    }
    listen(server_sock, 1970);
    while (1)
    {
        client_sock = accept(server_sock,(struct sockaddr *)&client_name,
                    &client_name_len);
        printf("from [%s:%d]\r\n", inet_ntoa(name.sin_addr), ntohs(name.sin_port));
        pthread_create(&newthread, NULL, FdHandler, client_sock);
    }
    return (0);
}
#endif
