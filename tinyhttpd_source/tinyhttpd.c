/* tinyhttpd webserver */
/* This program compiles for Sparc Solaris 2.6.
 * To compile for Linux:
 * 1) Comment out the #include<pthread.h> line.
 * 2) Comment out the line that defines the variable newthread
 * 3) Comment out the two lines that run pthread_create()
 * 4) Uncomment the line that runs accept_request()
 * 5) Remove -lsocket from the Makefile.
*/

/* logs */
/* 2018-3-4 ��ʼע�ͣ������main������ע��
 * 2018-3-5
*/

#include<stdio.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<ctype.h>
#include<strings.h>
#include<string.h>
#include<sys/stat.h>
#include<pthread.h>
#include<sys/wait.h>
#include<stdlib.h>

#define ISspace(x) isspace((int)(x))
//����˵����������c�Ƿ�Ϊ�ո��ַ�
//Ҳ�����ж��Ƿ�Ϊ�ո�(' ')����λ�ַ�('\t')��CR('\r'),����('\n')�����
//����ֵ����cΪ�հ��ַ����򷵻ط�0�����򷵻�0

#define SERVER_STRING "Server: jdbhttpd/0.1.0\r\n"      //����server����

void accept_request(int);   //������׽����ϼ�������һ��http����
void bad_request(int);  //���ظ��ͻ������Ǹ���������400��Ӧ��
void cat(int, FILE *);  //��ȡ��������ĳ���ļ�(FILE) д��socket�׽���(int)
void cannot_execute(int);   //��������ִ��cgi����ʱ���ֵĴ���cgiΪͨ�����ؽӿڼ�����������һ���ͻ��ˣ�����ҳ�������ִ���ڷ������ϵĳ�����������
void error_die(const char *);   //��������Ϣд��perror
void execute_cgi(int, const char *, const char *, const char *);    //����cgi�ű����漰����̬����
int get_line(int, char *, int); //��ȡһ��http����
void headers(int, const char *);    //����http��Ӧͷ
void not_found(int);    //�����Ҳ��������ļ�
void serve_file(int, const char *); //����cat�����������ļ����ݷ��ظ������
int startup(u_short *); //����http���񣬰����󶨶˿ڣ������������̴߳�������
void unimplemented(int);    //���ظ�����������յ���http�������õ�method����֧��

/************** accept_request ****************/
/* A request has caused a call to accept() on the server port to
 * return. Process the request appropriately.
 * Parameters: the socket connected to the client
*/

/************** bad_request ****************/
/* Inform the client that a request it has made has a problem.
 * Parameters: client socket
*/
/* ���ظ��ͻ�����ʱ��������400��Ӧ�� */
void bad_request(int client)
{
    char buf[1024];
    //����400���������Ϣ
    sprintf(buf, "HTTP/1.0 400 BAD REWUEST\r\n");   //sprintf����ʽ��������д���ַ���
    send(client, buf, sizeof(buf), 0);      //��buf���͸�client
    sprintf(buf, "Content-type: text/html\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "<P>Your browser sent a bad request, ");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "such as a POST without a Cotent-Length.\r\n");
    send(client, buf, sizeof(buf), 0);
}

/************** cat ****************/
/* Put the entire contents of a file out on a socket. This function
 * is named after the UNIX "cat" command, because it might have been
 * easier just to do something like pipe, fork, and exec("cat").
 * Parameters: the client socket descriptor
                FILE pointer for the file to cat
*/
/* ���ļ��ṹָ��resource�е����ݷ�����client */
void cat(int client, FILE *resource)
{
    //�����ļ�������
    char buf[1024];
    //��ȡ�ļ���buf��
    fgets(buf, sizeof(buf), resource);
    while (!feof(resource)) //�ж��ļ��Ƿ��ȡ��ĩβ��feof����������ϵ��ļ������������ļ��������ط�0ֵ
    {
        //���ļ����е��ַ�ȫ�����͸�client
        send(client, buf, strlen(buf), 0);        //�ͻ��ͷ�����������send������TCP���ӵ���һ�˷�������
                                                  //��һ������Ϊ���Ͷ��׽��֣����ĸ�����һ��Ϊ0

        fgets(buf, sizeof(buf), resource);

        /* ���ļ��ṹ��ָ��resource�ж�ȡ����bufsize-1�����ݣ�'\0'��
         * ÿ�ζ�ȡһ�У��������bufsize�����ȡ����н�����
         * ͨ��feof�����ж�fgets�Ƿ���������ֹ
         * ���⣬�������ļ�ƫ��λ�ã���һ�ֶ�ȡ�����һ�ֶ�ȡ���λ�ü���
        */
    }
}

/************** cannot_execute ****************/
/*

*/

/************** error_die ****************/
/*

*/

/************** execute_cgi ****************/
/*

*/

/************** get_line ****************/
/*

*/

/************** headers ****************/
/*

*/

/************** not_found ****************/
/*

*/

/************** serve_file ****************/
/*

*/

/************** startup ****************/
/* This function starts the process of listening for web connections
 * on a specified port. If the port is 0, then dynamically allocate a
 * port and modify the original port variable to reflect the actual
 * port.
 * Parameters: pointer to variable containing the port to connect on
 * Returns: the socket
*/
/*���������������������������׽��֣��󶨶˿ڣ�������
 *����һ�������׽���
*/
int startup(u_short *port)
{
    int httpd = 0;
    struct sockaddr_in name;

    httpd = socket(PF_INET, SOCK_STREAM, 0);    //�������������׽��֣��ɹ�����Ǹ���ʧ�ܷ���-1
                                                //�ڴ����м������Ǳ����д��������ע��PF_INET��AF_INET��������ʵ�ǿ��Ի��õģ���ָ���淶��
                                                //AF_INET������������ʹ��������
                                                //SOCK_STREAM��ʾ����׽��������������ӵ�һ���˵�
    if (httpd == -1)
        error_die("socket");

    memset(&name, 0, sizeof(name));
    /*�����׽��ֵ�ַ�ṹ*/
    name.sin_family = AF_INET;  //���õ�ַ�أ�sin_family��ʾaddress family��AF_INET��ʾʹ��TCP/IPЭ����ĵ�ַ
    name.sin_port = htons(*port);   //ָ���˿ڣ�sin_port��ʾport number��htons�������������޷��Ŷ�������ת���������ֽ�˳��
    name.sin_addr.s_addr = htonl(INADDR_ANY);   //ͨ���ַ��s.addr��ʾip address��htonl��������ת�����޷��ų����͵������ֽ�˳��

    if (bind(httpd, (struct sockaddr *)&name, sizeof(name)) < 0)    //��httpd�׽��ְ󶨵�ָ����ַ�Ͷ˿�
                                                                    //bind������name�еķ������׽��ֵ�ַ���׽���������httpd��ϵ����
                                                                    //�ɹ�����0��ʧ�ܷ���-1
        error_die("bind");

    if (*port == 0) //���*port==0����̬����һ���˿�
    {
        int namelen = sizeof(name);
        //���Զ˿�0����bind��getsockname���ڷ������ں˸���ı��ض˿ں�
        if (getsockname(httpd, (struct sockaddr *)&name, &namelen) == -1)   //getsockname���ڻ�ȡ�׽��ֵĵ�ַ�ṹ
                                                                            //���������Զ˿ں�0����bind֮�����ڷ����ں˸���ı��ض˿ں�
                                                                            //�ɹ�����0��ʧ�ܷ���-1
            error_die("getsockname");
        *port = ntohs(name.sin_port);   //�����ֽ�˳��ת��Ϊ�����ֽ�˳�򣬷��������ֽ�˳�������
    }

    if (listen(httpd, 5) < 0)   //listen��httpd�������׽���ת��Ϊ�����׽��֣�֮����Խ������Կͻ��˵���������
                                //5���������ʾ���ں��ڿ�ʼ�ܾ���������֮ǰ��Ӧ�÷�������еȴ���δ������������������ͨ������Ϊ1024
        error_die("listen");

    return(httpd);
}

/************* unimplemented **************/
/*

*/

int main()
{
    int server_sock = -1;
    u_short port = 0;   //����Ķ˿�Ϊ0
    int client_sock = -1;
    struct sockaddr_in client_name;      //sockaddr_in���ݽṹ���������̺����Ĳ�����ָ����ַ��Ϣ��
    int client_name_len = sizeof(client_name);
    pthread_t newthread;    //pthread_t���������߳�ID

    server_sock = startup(&port);   //�������˼����׽�������
    printf("httpd running on port %d\n", port);

    /*���̲߳���������ģʽ*/
    while(1)
    {
        /*���߳�*/
        //accept�����ȴ����Կͻ��˵��������󵽴�����������server_sock���ڶ�������Ϊ����ͻ����׽��ֵ�ַ(&client_name)
        //����һ�������ӵ������������ɹ���Ϊ�Ǹ���������������������Ϊ-1
        client_sock = accept(server_sock, (struct sockaddr *)&client_name, &client_name_len);   //accept�����ȴ��ͻ�����������

        if (client_sock == -1)      //acceptδ�ɹ�����������������
            error_die("accept");    //error_die������������Ϣд��perror�С�

        /*���������߳���accept_request��������������*/
        /*accept_request(client_sock);*/
        //pthread_createΪ��Unixϵͳ�д����̵߳ĺ������ɹ��򷵻�0��client_sockΪ���̺߳���accept_request���ݵĲ���
        if (pthread_create(&newthread, NULL, accept_request, client_sock) != 0) //���������̣߳�ִ�лص�����accept_request������client_sock
            perror("pthread_create");      //perror��ӡ���һ��ϵͳ������Ϣ���˴�Ϊ�����߳�ʧ��
    }

    close(server_sock); //�ر��׽��֣���Э��ջ���ԣ����ر�TCP����

    return 0;
}
