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
/* 2018-3-4 ��ʼע�ͣ������main������ע�͡�
 * 2018-3-5 ��ɺ��ĺ���startup������socket�������󶨣�������
            ������и���������

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
/* HTTPЭ��涨������ӿͻ��˷���������������Ӧ�����󲢷��ء�
 * ����ĿǰHTTPЭ��Ĺ涨����������֧��������Ӧ������Ŀǰ��HTTP
 * Э��汾���ǻ��ڿͻ�������Ȼ����Ӧ������ģ��
*/
/* accept_request���������ͻ��������ж�������̬�ļ�����cgi����
 * ��ͨ�����������Լ������жϣ�������Ǿ�̬�ļ����ļ������ǰ�ˣ�
 * �����cgi�����cgi��������
*/
void accept_request(int client)
{
    char buf[1024];
    int numchars;
    char method[255];   //���󷽷���GET��POST
    char url[255];  //������ļ�·��
    char path[512]; //�ļ������·��
    size_t i, j;
    struct stat st;     //stat�ṹ������������һ��linuxϵͳ�ļ����ļ����ԵĽṹ
    int cgi = 0;    //cgi��־λ�������ж��Ƿ��Ƕ�̬����

    char *query_string = NULL;

    numchars = get_line(client, buf, sizeof(buf));  //��client�ж�ȡָ����Сhttp���ݵ�buf
    i = 0; j = 0;
    //���տͻ��˵�http������
    //�����ַ�������ȡ�ո��ַ�ǰ���ַ�������254��
    while(!Isspace(buf[j]) && (i < sizeof(method) - 1))
    {
        method[i] = buf[j];     //����http�����ĸ�ʽ������õ��������󷽷�
        i++; j++;
    }
    method[i] = '\0';

    //���Դ�Сд�Ƚ��ַ������ж�ʹ�õ����������󷽷�
    if (strcasecmp(method, "GET") && strcasecmp(method, "POST"))
    {
        unimplemented(client);  //����֧�ֵķ��������ǣ���֪�ͻ���������ķ���δ��ʵ��
        return;
    }

    if (strcasecmp(method, "POST") == 0)    //�����POST����
        cgi = 1;    //���ñ�־λ��Post��ʾ�Ƕ�̬����

    i = 0;
    while (ISspace(buf[j]) && j < sizeof(buf))  //���˵��ո��ַ�
        j++;

    while (ISspace(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf)))
    {
        url[i] = buf[j];    //�õ�URL(��������׼��Դ�ĵ�ַ)
        i++; j++;
    }
    url[i] = '\0';

    if (strcasecmp(method, "GET") == 0)     //���������get
    {
        query_string = url;     //������Ϣ
        while ((*query_string != '?') && (*query_string != '\0'))   //������ǰ����ַ�
            query_string++; //�ʺ�ǰ����·���������ǲ���
        if (*query_string == '?')   //�õ��ʺţ������Ƕ�̬����
        {
            cgi = 1;
            *query_string = '\0';
            query_string++;     //��ʱָ��ָ���ʺŵ���һλ
        }
    }

    //��������Ŀ��htdocs�ļ��µ��ļ�
    sprintf(path, "htdocs%s", url); //��ȡ�����ļ�·��
    if (path[strlen(path) - 1 == '/')   //����ļ�������Ŀ¼(/)�������index.html
        strcat(path, "index.html");

    //����·�����ļ�������ȡpath�ļ���Ϣ���浽�ṹ��st�У�����Ǻ���stat������
    if (stat(path, &st) == -1)  //���ʧ��
    {
        //����headers����Ϣ
        while (numchars > 0 && strcmp("\n", buf))   //
            numchars = get_line(client, buf, sizeof(buf));  //
        not_found(client);  //��Ӧ�ͻ����Ҳ���
    }
    else    //����ļ���Ϣ��ȡ�ɹ�
    {
        //����Ǹ�Ŀ¼����Ĭ��ʹ�ø�Ŀ¼��index.html�ļ���stat�ṹ���е�st_mode�����ж��ļ�����
        if ((st.st_mode & S_IFMT) == S_IFDIR)
            strcat(path, "/index.html");
        if ((st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) || (st.st_mode & S_IXOTH))
            cgi = 1;

        if (!cgi)   //��̬ҳ������
            serve_file(client, path);   //ֱ�ӷ����ļ���Ϣ���ͻ��ˣ���̬ҳ�淵��
        else //��̬ҳ������
            execute_cgi(client, path, method, query_string);    //ִ��cgi�ű�
    }

    close(client);  //�رտͻ���socket
}

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
    //��ȡ�ļ���buf�У�fgets���ļ��ṹ��ָ��(FILE *resource)�ж�ȡ���ݣ�ÿ�ζ�ȡһ��
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
/* Inform the client that a CGI script could not be executed
 * Parameter: the client socket descriptor.
*/
/*���ط���������״̬��500 */
void cannot_execute(int client)
{
    char buf[1024];
    //����500��http״̬��500���˴�������ΪCGI�ű��޷�ִ�������
    sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<P>Error prohibited CGI execution.\r\n");
    send(client, buf, strlen(buf), 0);
}

/************** error_die ****************/
/* Print out an error message with perror() (for system errors; based
 * on value of errno, which indicates system call errors) and exit the
 * program indicating an error.
*/
/* ��ӡ��������Ϣ������ֹ */
void error_die(const char *sc)
{
    perror(sc);     //perror������������һ���������������ԭ���������׼�豸(stderr)��
                    //����sc��ָ�ַ������ȴ�ӡ;�˴���ԭ������ȫ�ֱ���errno��ֵ������Ҫ������ַ���
                    //errnoΪ��¼ϵͳ�����һ�δ�������ȫ�ֱ���
    exit(1);    //exit(1)��ʾ�쳣�˳�
}

/************** execute_cgi ****************/
/* Execute a cgi script. Will need to set environment variables as
 * appropriate.
 * Parameters: client socket descriptor
               path to the cgi script
*/
/* ִ��cgi(���������ӿ�)�ű�����Ҫ�趨���ʵĻ������� */
/* execute_cgi�����������󴫵ݸ�cgi������
 * ��������cgi֮��ͨ���ܵ�pipeͨ�ţ����ȳ�ʼ�������ܵ����������ӽ���ִ��cgi����
 * �ӽ���ִ��cgi���򣬻�ȡcgi�ı�׼���ͨ���ܵ����������̣��ɸ����̷��͸��ͻ���
*/
void execute_cgi(int client, const char *path const char *method, const char *query_string)
{
    char buf[1024];
    int cgi_output[2];
    int cgi_input[2];
    pid_t pid;
    int status;
    int i;
    char c;
    int numchars = 1;
    int content_length = -1;

    buf[0] = 'A'; buf[1] = '\0';
    if (strcasecmp(method, "GET") == 0)     //GET������һ�����ڻ�ȡ/��ѯ��Դ��Ϣ
        while ((numchars > 0) && strcmp("\n", buf)) //��ȡ������ͷ����Ϣ
            numchars = get_line(client, buf, sizeof(buf));  //�ӿͻ��˶�ȡ
    else    //POST������һ�����ڸ�����Դ��Ϣ
    {
        numchars = get_line(client, buf, sizeof(buf));

        //��ȡHTTP��Ϣʵ��Ĵ��䳤��
        while ((numchars > 0) && strcmp("\n", buf)) //���գ��Ҳ�Ϊ���з�
        {
            buf[15] = '\0';
            if (strcasecmp(buf, "Content_Length:") == 0)    //�Ƿ�ΪContent_Length�ֶ�
                content_length = atoi(&buf[16]);    //Content_Length��������HTTP��Ϣʵ��Ĵ��䳤��
            numchars = get_line(client, buf, sizeof(buf));
        }
        if (content_length == -1)
        {
            bad_request(client);    //�����ҳ������Ϊ�գ�û�����ݣ��������Ǵ���ҳ�������ֵĿհ�ҳ��
            return;
        }
    }

    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, strlen(buf), 0);

    //pipe���������ܵ����ɹ�����0�������������pipeʹ�õ������ļ�����������fd[0]:����ˣ�fd[1]:д���
    //������fork�е���pipe�������ӽ��̲���̳��ļ���������
    if (pipe(cgi_output) < 0)
    {
        cannot_execute(client); //�ܵ�����ʧ��
        return;
    }   //�ܵ�ֻ�ܾ��й������ȵĽ��̼���У������Ǹ��ӽ���֮��
    if (pipe(cgi_input) < 0)
    {
        cannot_execute(client);
        return;
    }

    //fork�ӽ��̣����������˸��ӽ��̼��IPC(���̼�ͨ��)ͨ��
    if ((pid = fork()) < 0)
    {
        cannot_execute(client);     //����ʧ��
        return;
    }

    /* ʵ�ֽ��̼�Ĺܵ�ͨ�Ż��� */
    //�ӽ��̼̳��˸����̵�pipe��Ȼ��ͨ���ر��ӽ���output�ܵ���out�ˣ�input�ܵ���in�ˣ�
    //�رո�����output�ܵ���in�ˣ�input�ܵ���out��
    //�ܵ���Ϊ�����ܵ��������ܵ�������ʹ�õ��������ܵ���
    //���������������ڹ������Ƚ�������ʹ�������ܵ��������Ǹ��ӽ��̵Ĺ�ϵ
    //�����ܵ�������һ��ϵͳ�е�������������֮��ͨ��
    //�ӽ���
    if (pid == 0)   //�����ӽ��̣�����ִ��cgi�ű�����
    {
        char meth_env[255];
        char query_env[255];
        char length_env[255];

        //�����ļ�������ض�����̵ı�׼�������
        //dup2�����ڹܵ�ʵ�ֽ��̼�ͨ�����ض����ļ�������
        dup2(cgi_output[1], 1); // 1��ʾstdout��0��ʾstdin����ϵͳ��׼����ض���Ϊcgi_output[1]
        dup2(cgi_input[0], 0);  //��ϵͳ��׼�����ض���Ϊcgi_input[0]
        close(cgi_output[0]);   //�ر�cgi_output�е�out��
        close(cgi_input[1]);    //�ر�cgi_input�е�in��

        //cgi��׼��Ҫ������ķ����洢�����������У�Ȼ���cgi�ű����н���
        sprintf(meth_env, "REQUEST_METHOD=%s", method);
        putenv(meth_env);       //putenv���������������ӻ�������

        if (strcasecmp(method, "GET") == 0) //get
        {
            //����query_string�Ļ�������
            sprintf(query_env, "QUERY_STRING=%s", query_string);
            putenv(query_env);
        }
        else    //post
        {
            //����content_Length�Ļ�������
            sprintf(length_env, "CONTENT_LENGTH=%d", content_length);
            putenv(length_env);
        }
        execl(path, paht, NULL);    //exec�����أ�ִ��cgi�ű�����ȡcgi�ı�׼�����Ϊ��Ӧ���ݷ��͸��ͻ���
        //��Ϊͨ��dup2������ض��򣬱�׼������ݽ���ܵ�output�������

        exit(0);    //�ӽ����˳�
    }
    else    //����Ǹ�����
    {
        close(cgi_output[1]);   //�ر�cgi_output�е�inͨ����ע�������Ǹ����̵�cgi_output���������ӽ������ֿ�
        close(cgi_input[0]);    //�ر�cgi_input��outͨ��
        /* ͨ���رն�Ӧ�ܵ��Ķ˿�ͨ����Ȼ���ض����ӽ��̵�ĳ�ˣ��������ڸ��ӽ���֮�乹����һ����˫��ͨ��
         * ����������ض��򣬽���һ�����͵�ȫ˫���ܵ�ͨ�Ż���
        */
        if (strcasecmp(method, "POST") == 0)    //post��ʽ����ָ���õĴ��䳤���ַ�����
            /* ����post���������� */
        for (i = 0; i < content_length; i++)
        {
            recv(client, &c, 1, 0); //�ӿͻ��˽��յ����ַ�
            write(cgi_input[1], &c, 1); //д��cgi_input��inͨ��
            //���ݴ��͹��̣�input[1](parent) --> input[0](child)[ִ��cgi����] --> STDIN --> STDOUT
            // --> output[1](child) -- > output[0](parent)[��������͸��ͻ���]

        }
        while (read(cgi_output[0], &c, 1) > 0)  //��ȡ������ݵ��ͻ���
            send(client, &c, 1, 0); //

        close(cgi_output[0]);   //�ر�ʣ�µĹܵ���
        close(cgi_input[1]);
        waitpid(pid, &status, 0);   //�ȴ��ӽ�����ֹ
    }

}


/************** get_line ****************/
/* Get a line from a socket, whether the line ends in a newline,
 * carriage return, or a CRLF combination. Terminates the string read
 * with a null character. If no newline indicator is found before the
 * end of the buffer, the string is terminated with a null. If any of
 * the above three line terminators is read, the last character of the
 * string will be a linefeed and the string will be terminated with a
 * null character.
 * Parameters: the socket descriptor
 *             the buffer to save the data in
 *             the size of the buffer
 * Returns: the number of bytes stored (excluding null)
*/
/* ��ȡһ��http���ģ���\r��\r\n��β */
int get_line(int sock, char *buf, int size)
{
    int i = 0;
    char c = '\0';  //'\0'��ʾNULL
    int n;

    //�����ȡsize-1���ַ������һ���ַ���Ϊ'\0'
    while ((i < size - 1) && (c != '\n'))
    {
        n = recv(sock, &c, 1, 0);   //�����ַ�����
                                    //�ͻ��ͷ���������recv������TCP���ӵ���һ�˽�������
                                    //��һ������ָ�����ն��׽�����������
                                    //�ڶ�������ָ��һ����������ַ���û������������recv�������յ�������

        if (n > 0)  //recv���ճɹ�
        {
            if (c == '\r')  //����ǻس������������ȡ
            {
                //ʹ��MSG_PEEK��־ʹ��һ�ζ�ȡ��Ȼ���Եõ���ζ�ȡ�����ݣ�����Ϊ���մ��ڲ�����
                n = recv(sock, &c, 1, MSG_PEEK);

                if ((n > 0) && (c == '\n')) //  ����ǻس����з�
                    recv(sock, &c, 1, 0);   //�������յ����ַ���ʵ���Ϻ������Ǹ���־λMSG_PEEK��ȡͬ�����ַ�
                                            //����ɾ��������е����ݣ����������ڣ�c=='\n'
                else
                    c = '\n';   //ֻ�Ƕ�ȡ���س�������ֵΪ���з���Ҳ��ֹ�˶�ȡ
            }
            buf[i] = c; //����ȡ�������ݷ��뻺����
            i++;    //ѭ��������һ�ζ�ȡ
        }
        else    //û�ж����κ�����
            c = '\n';
    }
    buf[i] = '\0';

    return(i);  //���ض�ȡ�����ַ�����������'\0'��
}

/************** headers ****************/
/* Return the informational HTTP headers about a file.
 * Parameters: the socket to print the headers on the
 * name of the file
*/
/* �ɹ�����http��Ӧͷ����Ϣ */
void headers(int client, const char *filename)
{
    char buf[1024];
    (void)filename; //(void)var �������ǣ�����δʹ�ñ����ı��뾯��

    strcpy(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, SERVER_STRING);         //SERVER_STRINGΪdefine��server����
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
}

/************** not_found ****************/
/* Give a client a 404 not found status message
*/
/* ����404 */
void not_found(int client)
{
    char buf[1024];
    //����404
    sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Conent-type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
    send(cliend, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "your request because the resource specified\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "is unavailable or nonexistent.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}


/************** serve_file ****************/
/* Send a regular file to the client. Use headers, and report
 * errors to client if they occur.
 * Parameters: a pointer to a file structure produced from the socket
 *             file descriptor
 *             the name of the file to serve
*/
/* ��������ļ����ͻ�client */
void serve_file(int client, const char *filename)
{
    FILE *resource = NULL;
    int numchars = 1;
    char buf[1024];

    buf[0] = 'A';
    buf[1] = '\0';  //��������֪���Ǹ�ʲô��
    while ((numchars > 0) && strcmp("\n", buf))     //read & discard headers
        numchars = get_line(client, buf, sizeof(buf));      //���ж�ȡֵclient

    resource = fopen(filename, "r");    //��ֻ����ʽ���ļ�
                                        //fopen���ļ����������ļ�ָ�루FILE*��
    if (resource == NULL)
        not_found(client);  //����ļ������ڣ�����not_found����404
    else
    {
        headers(client, filename);  //�ȷ����ļ�ͷ����Ϣ����������ȡ�ļ�200
        cat(client, resource);      //��resource������ָ���ļ��е����ݷ��͸�client
    }
    fclose(resource);   //�ر�
}

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
/* Inform the client that the requested web method has not been
 * implemented.
 * Parameter: the client socket
*/
/* ֪ͨclient����������󷽷�����������֧�� */
void unimplemented(int client)
{
    char buf[1024];
    //����501��ʾ��Ӧ����δʵ��
    sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</TITLE></HEAD>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
    send(cliend, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}

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
