#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>          
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <sqlite3.h>
#include <signal.h>
#include <time.h>                                                                                                                                    
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

#define  R  1   // user - register
#define  L  2   // user - login
#define  Q  3   // user - quit

#define DATABASE "myusr.db"

//定义通信结构体
typedef struct{
	char type;
	char name[20];          //姓名
	char password[20];      //密码
	int age;                //年龄
	int id;                 //工号id
    int money;              //工资
    char pos[64];           //职位
    char mail[128];         //邮箱
    char phone[20];         //电话
    char addr[128];         //地址
	char data[500];         
}__attribute__((packed)) MSG;

sqlite3 *db;

void do_quit(int acceptfd,MSG *msg); 
void do_client(int acceptfd,sqlite3 *db);
void do_register(int acceptfd,MSG *msg,sqlite3 *db);
void do_login(int acceptfd,MSG *msg,sqlite3 *db);
void do_root_menu(int acceptfd,MSG *msg,sqlite3 *db);
void do_usr_menu(int acceptfd,MSG *msg,sqlite3 *db);
void do_add(int acceptfd,MSG *msg,sqlite3 *db);
void do_root_query(int acceptfd,MSG *userMsg,sqlite3 *db); 
void do_delete(int acceptfd,MSG *msg,sqlite3 *db);
void do_update(int acceptfd,MSG *msg,sqlite3 *db);
void do_usr_query(int acceptfd,MSG *msg,sqlite3 *db); 
void do_usr_update(int acceptfd,MSG *msg,sqlite3 *db);
void do_preview(int acceptfd,MSG *msg,sqlite3 *db);

/*
 *  Name   : sig_child_handle
 *  Description : 处理孤儿进程
 *  Input  : 
 *  Output :
 */
void sig_child_handle(int signo)
{
	if(SIGCHLD == signo)
		while(waitpid(-1,NULL,WNOHANG) > 0);
}

/*
 *  Name   : main
 *  Description : 主程序入口
 *  Input  : 
 *  Output :
 */
int main(int argc, const char *argv[])
{
	int sockfd;
	int n;
	int acceptfd;
	int reuse = 1;
	pid_t pid;
	char *errmsg;
	char sql[1024];
    
	//终端输入判断
	if(argc != 3)  
	{
		printf("Usage:%s serverip port.\n",argv[0]);
		return -1;
	}
    
	//打开数据库
	if(sqlite3_open(DATABASE,&db) != SQLITE_OK){
		printf("%s\n",sqlite3_errmsg(db));
		return -1;
	}
	else 
		printf("数据库打开成功\n");
    
	//创建表
	memset(sql, '\0', sizeof(sql));
	sprintf(sql,"create table if not exists usr(id integer primary key autoincrement,name vchar(20),password vchar,age integer,salary integer,pos vchar(64),mail vchar(128),phone vchar(20),addr vchar(128))");
	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK){
		printf("%s\n",errmsg);
	}
	else
		printf("用户表创建成功\n");

 	//创建套接字	
	if((sockfd = socket(AF_INET, SOCK_STREAM,0)) < 0)
	{
		perror("socket error.\n");
		exit(1);
	}

    //设置地址绑定快速重用
	setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));

	//填充服务器要绑定的地址信息结构体，为bind铺路
	struct sockaddr_in serveraddr,clientaddr;
	socklen_t client_len = sizeof(clientaddr);

	bzero(&serveraddr,sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(argv[1]);
	serveraddr.sin_port = htons(atoi(argv[2]));

	//把地址信息绑定到套接字上
	if(bind(sockfd,(struct sockaddr *)&serveraddr,sizeof(serveraddr)) < 0)
	{
		perror("connect bind.\n");
		exit(1);
	}

	//把主动套接字变为被动套接子
	if(listen(sockfd, 10) < 0)
	{
		perror("listen error.\n");
		exit(1);
	}
	puts("服务器已经启动！");

	signal(SIGCHLD,sig_child_handle);

	//开始与客户端通信
	while(1){
		if((acceptfd = accept(sockfd,(struct sockaddr *)&clientaddr,&client_len)) < 0){
			perror("acceptfd error.\n");
			return -1;
		}

		if((pid = fork()) < 0){
			perror("fork error.\n");
			return -1;
		}
		else if(pid == 0){
			close(sockfd);
			do_client(acceptfd,db);   //处理客户端请求
		}
		else{
			close(acceptfd);
		}
	}
	return 0;
}


/*
 *  Name   :      do_client
 *  Description : 客户端请求函数
 *  Input  : 
 *  Output :
 */
void do_client(int acceptfd,sqlite3 *db)
{
	MSG *msg=(MSG *)malloc(sizeof(MSG));
	while(1){
		if(recv(acceptfd, msg, sizeof(*msg),0) < 0){
			fprintf(stderr,"%s\n",strerror(errno));
			return;
		}
		switch(msg->type){
		case 'R':
			do_register(acceptfd,msg,db);
			break;
		case 'L':
			do_login(acceptfd,msg,db);
			break;
		case 'Q':
			do_quit(acceptfd,msg);
			return;
		default:
			printf("输入错误,请重新输入。\n");
			break;
		}
	}
	return;
}


/*
 *  Name   : do_quit
 *  Description : 退出程序
 *  Input  : 
 *  Output :
 */  
void do_quit(int acceptfd,MSG *msg) 
{
	sqlite3_close(db);
	free(msg);
	close(acceptfd);
	printf("用户退出登录\n");
	return;
}


/*
 *  Name   : do_register
 *  Description : 员工注册函数
 *  Input  : 
 *  Output :
 */
void do_register(int acceptfd,MSG *msg,sqlite3 *db)
{
	char * errmsg;
	char sql[500];
	int ret = -1;

	if(recv(acceptfd,msg,sizeof(MSG),0) < 0){
		perror("register recvive error");
		return;
	}

	memset(sql, '\0', sizeof(sql));
	sprintf(sql,"insert into usr values(null,'%s','%s',%d,null,null,'%s','%s','%s');",msg->name,msg->password,msg->age,msg->mail,msg->phone,msg->addr);

	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK){
		printf("%s\n",errmsg);
		strcpy(msg->data,"注册失败");
	}
	else{
		printf("用户注册成功!\n");
		strcpy(msg->data,"注册成功");
	}

	if(send(acceptfd,msg,sizeof(MSG),0) < 0){
		perror("send error.");
		return;
	}

    return;
}

/*
 *  Name   :      do_login
 *  Description : 登录函数    管理员账户:admin, 密码：123456
 *  Input  : 
 *  Output :
 */
void do_login(int acceptfd,MSG *msg,sqlite3 *db)
{
	char sql[500] ={};
	char *errmsg;
	int nrow;
	int ncloumn;
	char **resultp;

	if(recv(acceptfd,msg,sizeof(MSG),0) < 0){
		perror("register recvive error");
		return;
	}

	memset(sql, '\0', sizeof(sql));
	sprintf(sql,"select * from usr where name = '%s' and password = '%s';",msg->name,msg->password);

	if(sqlite3_get_table(db,sql,&resultp,&nrow,&ncloumn,&errmsg)!=SQLITE_OK){
		printf("%s\n",errmsg);
		return;
	}
	else
		printf("get_table ok!\n");
	
	//root登录
	if((strcmp("admin",msg->name) == 0)&&(strcmp("123456",msg->password) == 0)){
		strcpy(msg->data,"管理员登录成功.");
		printf("管理员登录成功.\n");
		send(acceptfd,msg,sizeof(MSG),0);
		do_root_menu(acceptfd,msg,db);
		return;
	}
	
	//usr登录成功
	if((nrow >= 1) && (strcmp(resultp[10], msg->name) == 0) && (strcmp(resultp[11], msg->password) == 0)){
		strcpy(msg->data,"登录成功");
		msg->id=atoi(resultp[9]);       //获取登录员工id号
		printf("用户登录成功.\n");
		send(acceptfd,msg,sizeof(MSG),0);
        do_usr_menu(acceptfd,msg,db);
		return;
	}else{
		strcpy(msg->data,"用户名或密码错误.");
		printf("用户登录失败.\n");
		send(acceptfd,msg,sizeof(MSG),0);
		return;
	}
	return;
}

/*
 *  Name   :      do_root_menu
 *  Description : 管理员登录界面
 *  Input  : 
 *  Output :
 */
void do_root_menu(int acceptfd,MSG *msg,sqlite3 *db)
{
	while(1){
		if(recv(acceptfd,msg,sizeof(MSG),0) < 0){
			perror("open root_menu error.");
			return;
		}
		switch(msg->type){
		case 'A':
			do_add(acceptfd,msg,db);
			break;
		case 'D':
			do_delete(acceptfd,msg,db);
			break;
		case 'S':
			do_root_query(acceptfd,msg,db);
			break;
		case 'U':
			do_update(acceptfd,msg,db);
			break;
		case 'P':
			do_preview(acceptfd,msg,db);
			break;
		case 'E':	
			return;
		
		default:
			printf("输入错误,请重新输入。");
			break;
		}
	}
	return;
}

/*
 *  Name   : do_usr_menu
 *  Description : 用户登录界面
 *  Input  : 
 *  Output :
 */
void do_usr_menu(int acceptfd,MSG *msg,sqlite3 *db)
{
	while(1){
		if(recv(acceptfd,msg,sizeof(MSG),0) < 0){
			perror("open usr_menu error.");
			return;
		}
		switch(msg->type){
		case 'S':
			do_usr_query(acceptfd,msg,db);
			break;
		case 'C':
			do_usr_update(acceptfd,msg,db);
			break;
		case 'O':
			return;
		default:
			printf("客户端输入错误。\n");
			break;
		}
	}
	return;
}

/*
 *  Name   : do_add
 *  Description : 添加员工信息函数
 *  Input  : 
 *  Output :
 */
void do_add(int acceptfd,MSG *msg,sqlite3 *db)
{
	do_register(acceptfd,msg,db);
	return ;
}

/*
 *  Name   : do_usr_query
 *  Description: 员工查询自己信息函数
 *  Input  : 
 *  Output :
 */
void do_usr_query(int acceptfd,MSG *msg,sqlite3 *db) 
{
	char sql[500] = {};
	char *errmsg;
	char **dbResult;
	int nRow,nColumn;
    MSG recvmsg;
    
	recv(acceptfd,msg,sizeof(MSG),0);
    memset(&recvmsg,'\0',sizeof(MSG));
	memset(sql, '\0', sizeof(sql));
	sprintf(sql, "select * from usr where id = %d ;", msg->id);
	printf("%d\n",msg->id);
	if(sqlite3_get_table(db, sql, &dbResult, &nRow, &nColumn,&errmsg) != SQLITE_OK) {
		fprintf(stderr, "%s\n", errmsg);
		return;
	} else {
		recvmsg.id = atoi(dbResult[9]);
		strcpy(recvmsg.name,dbResult[10]);
		strcpy(recvmsg.password, dbResult[11]);
		recvmsg.age =atoi(dbResult[12]);
		recvmsg.money = 5000;
		strcpy(recvmsg.pos, "staff");
		strcpy(recvmsg.mail, dbResult[15]);
		strcpy(recvmsg.phone, dbResult[16]);
		strcpy(recvmsg.addr, dbResult[17]);
		strcpy(recvmsg.data, "查询成功");
	}
	
	if (send(acceptfd, &recvmsg, sizeof(MSG), 0) < 0) {
			perror("usr_query send error.");
			return;
		}else{
			printf("发送成功\n");
		}
	return;
}

/*
 *  Name   : do_root_query
 *  Description: 管理员查询员工信息函数
 *  Input  : 
 *  Output :
 */
void do_root_query(int acceptfd,MSG *msg,sqlite3 *db) 
{
	char sql[500] = {};
	char *errmsg;
	char **dbResult;
	int nRow,nColumn;
	MSG recvmsg;

	recv(acceptfd,msg,sizeof(MSG),0);
	
	memset(sql, '\0', sizeof(sql));
	sprintf(sql, "select * from usr where id = %d;", msg->id);
	if(sqlite3_get_table(db, sql, &dbResult, &nRow, &nColumn,&errmsg) != SQLITE_OK) {
		fprintf(stderr, "%s\n", errmsg);
		return;
	} else if(nRow == 0){
		strcpy(recvmsg.data, "该员工不存在.");
		send(acceptfd,&recvmsg,sizeof(MSG),0);
		return;
	}else{
		recvmsg.id = atoi(dbResult[9]);
		strcpy(recvmsg.name,dbResult[10]);
		strcpy(recvmsg.password, dbResult[11]);
		recvmsg.age =atoi(dbResult[12]);
		recvmsg.money = 5000;
		strcpy(recvmsg.pos, "staff");
		strcpy(recvmsg.mail, dbResult[15]);
		strcpy(recvmsg.phone, dbResult[16]);
		strcpy(recvmsg.addr, dbResult[17]);
		strcpy(recvmsg.data, "查询成功");
	}
	
	if (send(acceptfd, &recvmsg, sizeof(MSG), 0) < 0) {
			fprintf(stderr,"%s\n",strerror(errno));
			return;
		}
	return;
}

/*
 *  Name   : do_preview
 *  Description: 管理员查看所有员工信息
 *  Input  : 
 *  Output :
 */
void do_preview(int acceptfd,MSG *msg,sqlite3 *db) 
{
	char sql[500] = {};
	char *errmsg;
	char **dbResult;
	int nRow,nColumn;
	MSG recvmsg;
    int i=1;

	while(1){
		memset(sql, '\0', sizeof(sql));
		sprintf(sql, "select * from usr where id = %d;", i++);
		if(sqlite3_get_table(db, sql, &dbResult, &nRow, &nColumn,&errmsg) != SQLITE_OK) {
			fprintf(stderr, "%s\n", errmsg);
			return;
		}else if(nRow == 0){
			strcpy(recvmsg.data , "no data");
	        send(acceptfd, &recvmsg, sizeof(MSG), 0);
			return;
		}else{
			recvmsg.id = atoi(dbResult[9]);
			strcpy(recvmsg.name,dbResult[10]);
			strcpy(recvmsg.password, dbResult[11]);
			recvmsg.age =atoi(dbResult[12]);
			recvmsg.money = 5000;
			strcpy(recvmsg.pos, "staff");
			strcpy(recvmsg.mail, dbResult[15]);
			strcpy(recvmsg.phone, dbResult[16]);
			strcpy(recvmsg.addr, dbResult[17]);
		}

	    if(send(acceptfd, &recvmsg, sizeof(MSG), 0) < 0) {
			perror("root_query send error.");
			return;
		   }
	}
	return;
}

/*
 *  Name   : do_delete
 *  Description: 删除员工信息函数
 *  Input  : 
 *  Output :
 */
void do_delete(int acceptfd,MSG *msg,sqlite3 *db)
{
	char sql[500] = {0};
	char *errmsg;
	char **dbResult;
	int nRow,nColumn;
	MSG recvmsg;

	if(recv(acceptfd,msg,sizeof(MSG),0) < 0){
		perror("root_delete_recv error.");
		return;
	}

	memset(sql, '\0', sizeof(sql));
	sprintf(sql, "select * from usr where id = %d;", msg->id);
		
	if (sqlite3_get_table(db, sql, &dbResult, &nRow, &nColumn, &errmsg) != SQLITE_OK) {
		fprintf(stderr, "%s\n", errmsg);
		return;
	} else if (nRow < 1) {
		strcpy(recvmsg.data, "该员工不存在。");
		if(send(acceptfd, &recvmsg, sizeof(recvmsg), 0) < 0){
			perror("send error.");
			return;}
	}else{
		sprintf(sql, "delete from usr where id = %d;", msg->id);
		if(sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK){
			fprintf(stderr, "%s\n", errmsg);
			return;
		}
		strcpy(recvmsg.data, "删除成功");
		sprintf(sql,"update usr set id=id-1 where id > %d;",msg->id);
		if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK){
			fprintf(stderr,"%s\n",errmsg);
			return;
		}
		if(send(acceptfd, &recvmsg, sizeof(recvmsg), 0) < 0) {
			perror("send error.");
			return;
		}
	}
	return;
}

/*
 *  Name   : do_update
 *  Description: 管理员修改员工信息
 *  Input  : 
 *  Output :
 */
void do_update(int acceptfd,MSG *msg,sqlite3 *db)
{
	char sql[500] = {0};
	char **dbResult;
	int nRow,nCloumn;
	char *errmsg;

	if(recv(acceptfd,msg,sizeof(MSG),0) < 0){
		perror("update_recv error.");
		return;
	}

	memset(sql, '\0', sizeof(sql));
	sprintf(sql,"select * from usr where id='%d';",msg->id);
	if(sqlite3_get_table(db,sql,&dbResult,&nRow,&nCloumn,&errmsg) != SQLITE_OK){
		fprintf(stderr, "%s\n", errmsg);
		return;
	}else if(nRow >= 1){
		sqlite3_free_table(dbResult);
		sprintf(sql,"update usr set name='%s',password='%s',age=%d,mail='%s',phone='%s',addr='%s' where id = %d;",msg->name,msg->password,msg->age,msg->mail,msg->phone,msg->addr,msg->id);
		if(sqlite3_get_table(db,sql,&dbResult,&nRow,&nCloumn,&errmsg) != SQLITE_OK){
			fprintf(stderr, "%s\n", errmsg);
			return;
		}
		strcpy(msg->data,"员工信息修改完成");
		if(send(acceptfd,msg,sizeof(MSG),0) < 0){
			perror("update_send error.");
			return;
		}
	}else{
		sqlite3_free_table(dbResult);
		strcpy(msg->data,"该员工不存在。");
		if(send(acceptfd,msg,sizeof(MSG),0) < 0){
			perror("update_send error.");
			return;
		}
	}
	return;
}

/*
 *  Name   : do_usr_update
 *  Description: 员工修改自己信息函数
 *  Input  : 
 *  Output :
 */
void do_usr_update(int acceptfd,MSG *msg,sqlite3 *db)
{
	do_update(acceptfd,msg,db);
}






