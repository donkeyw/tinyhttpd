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
