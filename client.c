#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define  R  1   // user - register
#define  L  2   // user - login
#define  Q  3   // user - query

//定义通信结构体
typedef struct {
	char type;
	char name[20];     //姓名
	char password[20]; //密码
	int  age;          //年龄
	int  id;           //员工id
	int  money;        //工资
	char pos[64];      //职位
	char mail[128];    //邮箱
	char phone[20];    //电话
	char addr[128];    //地址
	char data[500];
}__attribute__((packed)) MSG;

void do_register(int socketfd,MSG *msg);
void do_login(int socketfd,MSG *msg);
void do_root_menu(int socketfd, MSG *msg);
void do_usr_menu(int socketfd, MSG *msg);
void do_add(int socketedfd,MSG *msg);
void do_root_query(int socketfd,MSG *msg); 
void do_delete(int socketfd, MSG *msg);
void do_update(int socketfd, MSG * msg);
void do_usr_query(int socketfd,MSG *msg);
void do_usr_update(int socketfd,MSG *msg);
void do_preview(int socketfd,MSG *msg);

/*
 *  Name   : main 
 *  Description : 程序主入口
 *  Input  : 
 *  Output :  
 */
int main(int argc, const char *argv[])
{
	int socketfd;
	MSG msg;
	int n;

	if(argc != 3)  //终端输入判断
	{
		printf("Usage:%s serverip port.\n",argv[0]);
		return -1;                                                                            
	}

	if((socketfd = socket(AF_INET,SOCK_STREAM,0)) < 0){
		perror("fail to socket");
		exit(-1);
	}

	//网络信息结构体填充
	struct sockaddr_in serveraddr;
	bzero(&serveraddr,sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(atoi(argv[2]));
	serveraddr.sin_addr.s_addr = inet_addr(argv[1]);

	//连接
	if(connect(socketfd,(struct sockaddr *)&serveraddr,sizeof(serveraddr)) < 0){
		perror("fail to connect");
		close(socketfd);
		exit(-1);
	}
	puts("客户端就绪");

	while(1){
		printf("*********员工信息管理系统*********\n");
		printf("*                                *\n");
		printf("* 1: 注册  2: 登录  3: 退出系统  *\n");
		printf("*                                *\n");
		printf("**********************************\n");
		printf("请选择服务内容: ");
		scanf("%d",&n);
		getchar();
		if((n!=1)&&(n!=2)&&(n!=3)){
			printf("输入错误，请重新输入!\n");
		}else{
			switch(n){
			case 1: //注册
				msg.type = 'R';
				send(socketfd,&msg,sizeof(MSG),0);
				do_register(socketfd,&msg);
				break;
			case 2: //登录
				msg.type = 'L';
				send(socketfd,&msg,sizeof(MSG),0);
				do_login(socketfd,&msg);
				break;
			case 3: //退出
				msg.type = 'Q';
				send(socketfd,&msg,sizeof(MSG),0);
				close(socketfd);
				exit(0);
				return 0;
			default:
				printf("输入错误，请重新输入！\n");
				break;
			}
		}
	}		
	return 0;
}

/*
 *  Name   : do_register 
 *  Description : 注册函数
 *  Input  : 
 *  Output :  
 */
void do_register(int socketfd,MSG *msg)
{
	printf("请输入姓名:");
	scanf("%s",msg->name);
	getchar();

	printf("请输入登录密码:");
	scanf("%s",msg->password);
	getchar();

	printf("请输入年龄:");
	scanf("%d",&msg->age);
	getchar();

	printf("请输入邮箱:");
	scanf("%s",msg->mail);
	getchar();

	printf("请输入电话:");
	scanf("%s",msg->phone);
	getchar();

	printf("请输入住址:");
	scanf("%s",msg->addr);
	getchar();

	if(send(socketfd,msg,sizeof(MSG),0) < 0){
		printf("register fail to send.\n");
		return;
	}

	if(recv(socketfd,msg,sizeof(MSG),0) < 0){
		printf("register fail to recv.\n");
		return;
	}

	printf("%s\n",msg->data);
	printf("您注册的信息是：\n");
	printf("姓名:%s\n",msg->name);
	printf("密码:%s\n",msg->password);
	printf("年龄:%d\n",msg->age);
	printf("邮箱:%s\n",msg->mail);
	printf("电话:%s\n",msg->phone);
	printf("住址:%s\n",msg->addr);

	return;
}

/*
 *  Name   : do_login 
 *  Description : 登录函数
 *  Input  : 
 *  Output :  
 */
void do_login(int socketfd, MSG *msg)
{
	printf("请输入姓名:");
	scanf("%s", msg->name);
	getchar();

	printf("请输入登录密码:");
	scanf("%s", msg->password);
	getchar();

	if(send(socketfd, msg, sizeof(MSG),0) < 0){
		perror("login fail to send.\n");
		return ;
	}

	if(recv(socketfd, msg, sizeof(MSG),0) < 0){
		perror("Fail to recv.\n");
		return ;
	}

	printf("%s\n", msg->data);

	if((strcmp("admin",msg->name) == 0)&&(strcmp("123456",msg->password) == 0)){
		do_root_menu(socketfd,msg);
	}
	else if(strcmp(msg->data,"登录成功") == 0){
		do_usr_menu(socketfd,msg);
	}
	else{
		printf("请重新输入！\n");
	}
	return;
}

/*
 *  Name   :      do_usr_query 
 *  Description : 用户查询自己信息
 *  Input  : 
 *  Output :  
 */
void do_usr_query(int socketfd,MSG *msg)
{
	if(send(socketfd,msg,sizeof(MSG),0) < 0){
		perror("usr_query_send error.");
		return;
	}

	//memset(msg,'\0',sizeof(MSG));

	if(recv(socketfd,msg,sizeof(MSG),0) < 0){
		perror("usr_query_recv error.");
		return;
	}

	printf("ID   : %d\n", msg->id);
	printf("姓名 : %s\n", msg->name);
	printf("年龄 : %d\n", msg->age);
	printf("工资 : %d\n", msg->money);
	printf("职位 : %s\n", msg->pos);
	printf("邮箱 : %s\n", msg->mail);
	printf("电话 : %s\n", msg->phone);
	printf("住址 : %s\n", msg->addr);

	printf("%s\n",msg->data);

	return;
}

/*
 *  Name   : do_root_menu 
 *  Description : 管理员登录界面
 *  Input  : 
 *  Output :  
 */
void do_root_menu(int socketfd, MSG *msg)
{
	int n;
	while(1){
		printf("***************************管理员登录*************************\n");
		printf("*                                                            *\n");
		printf("* 1: 添加用户  2: 删除用户  3: 查询用户信息  4: 更新用户信息 *\n");
		printf("*                                                            *\n");
		printf("*           5: 浏览所有用户信息      6：返回                 *\n");
		printf("*                                                            *\n");
		printf("**************************************************************\n");
		printf("请选择: ");

		scanf("%d", &n);
		getchar();
		switch(n){
		case 1:
			msg->type = 'A';
			send(socketfd,msg,sizeof(MSG),0);
			do_add(socketfd,msg);    //添加用户
			break;

		case 2:
			msg->type = 'D';
			send(socketfd,msg,sizeof(MSG),0);
			do_delete(socketfd,msg);  //删除用户
			break;

		case 3:
			msg->type = 'S';
			send(socketfd,msg,sizeof(MSG),0);
			do_root_query(socketfd,msg);  //查询用户信息
			break;

		case 4:
			msg->type = 'U';
			send(socketfd,msg,sizeof(MSG),0);
			do_update(socketfd,msg);    //更新用户信息
			break;

		case 5:
			msg->type = 'P';                
			send(socketfd,msg,sizeof(MSG),0);
 			do_preview(socketfd,msg);  //浏览所有用户信息
			break;

		case 6:
			msg->type = 'E';                
			send(socketfd,msg,sizeof(MSG),0);
			return;
		
		default:
			printf("输入错误，请重新输入。\n");
			break;
		}
	}
	return;
}
	
/*
 *  Name   : do_usr_menu 
 *  Description : 普通员工登录界面
 *  Input  : 
 *  Output :  
 */
void do_usr_menu(int socketfd, MSG *msg)
{
	int n;
	while(1){
		printf("************员工登录界面************\n");
		printf("*                                  *\n");
		printf("* 1: 查询信息  2: 更新信息  3:返回 *\n");
		printf("*                                  *\n");
		printf("************************************\n");
		printf("请选择: ");

		scanf("%d", &n);
		getchar();
		switch(n){
		case 1:
			msg->type = 'S';
			send(socketfd,msg,sizeof(MSG),0);
			do_usr_query(socketfd,msg);   
			break;

		case 2:
			msg->type = 'C';
			send(socketfd,msg,sizeof(MSG),0);
			do_usr_update(socketfd,msg);  
			break;

		case 3:
			msg->type = 'O';
			send(socketfd,msg,sizeof(MSG),0);
			return;
		
		default:
			printf("输入错误,请重新输入。\n");
			break;
		}
	}
	return;
}

/*
 *  Name   : do_add 
 *  Description : 添加员工信息
 *  Input  : 
 *  Output :  
 */
void do_add(int socketfd,MSG *msg)
{
	do_register(socketfd,msg);
	return ;
}

/*
 *  Name   : do_root_query 
 *  Description : 管理员查询员工信息
 *  Input  : 
 *  Output :  
 */
void do_root_query(int socketfd,MSG *msg) 
{
	printf("请输入查询员工id:");
	scanf("%d",&msg->id);
	getchar();
    
	 if(send(socketfd,msg,sizeof(MSG),0) < 0){
		 perror("root_query send error.");
		 return;
	 }

	 if(recv(socketfd,msg,sizeof(MSG),0) < 0){
		 perror("root_query recv error.");
		 return;
	 }
	 
	 if(strcmp(msg->data,"查询成功") == 0){
		 printf("%s\n",msg->data);
		 printf("ID : %d\n", msg->id);
		 printf("姓名 : %s\n", msg->name);
		 printf("登录密码 : %s\n", msg->password);
		 printf("年龄 : %d\n", msg->age);
		 printf("工资 : %d\n", msg->money);
		 printf("职位 : %s\n", msg->pos);
		 printf("邮箱 : %s\n", msg->mail);
		 printf("住址 : %s\n", msg->addr);
	 }else{
		 printf("%s\n",msg->data);
	 }
	 return;
}

/*
 *  Name   : do_delete 
 *  Description : 删除员工信息
 *  Input  : 
 *  Output :  
 */
void do_delete(int socketfd, MSG *msg)
{
	printf("请输入删除员工id:");
	scanf("%d",&msg->id);
	getchar();           						   //吃掉垃圾字符

	if(send(socketfd,msg,sizeof(MSG),0) < 0){
		perror("root_delete_send error.)");
		return;
	}

	if(recv(socketfd,msg,sizeof(MSG),0) < 0){
		perror("root_delete_recv error");
		return;
	}

	printf("%s\n",msg->data);
	return;
}

/*
 *  Name   : do_update 
 *  Description : 修改员工信息
 *  Input  : 
 *  Output :  
 */
void do_update(int socketfd, MSG * msg)
{
	printf("输入要修改员工id:");
	scanf("%d",&msg->id);
	getchar();            				  //吃掉垃圾字符

	printf("输入要修改员工姓名:");
	scanf("%s",msg->name);
	getchar(); 							  //吃掉垃圾字符

	printf("输入登录密码:");
	scanf("%s",msg->password);
	getchar();    						 //吃掉垃圾字符

	printf("输入员工年龄:");
	scanf("%d",&msg->age);
	getchar();    				          //吃掉垃圾字符

	printf("输入员工工资:");
	scanf("%d",&msg->money);
	getchar();                            //吃掉垃圾字符

	printf("输入员工邮箱:");
	scanf("%s",msg->mail);
	getchar();    				          //吃掉垃圾字符

	printf("输入员工职位:");
	scanf("%s",msg->pos);
	getchar();    				          //吃掉垃圾字符

	printf("输入员工电话:");
	scanf("%s",msg->phone);
	getchar();    				          //吃掉垃圾字符

	printf("输入员工住址:");
	scanf("%s",msg->addr);
	getchar();    				          //吃掉垃圾字符

	if(send(socketfd,msg,sizeof(MSG),0) < 0){
		perror("update_send error.");
		return;
	}

	if(recv(socketfd,msg,sizeof(MSG),0) < 0){
		perror("update_recv error");
		return;
	}

	 if(strcmp(msg->data,"员工信息修改完成") == 0){
		 printf("%s\n",msg->data);
		 printf("更新后内容为：\n");
		 printf("ID : %d\n",msg->id);
		 printf("姓名 : %s\n", msg->name);
		 printf("登录密码 : %s\n", msg->password);
		 printf("年龄 : %d\n", msg->age);
		 printf("工资 : %d\n", msg->money);
		 printf("邮箱 : %s\n", msg->mail);
		 printf("职位 : %s\n", msg->pos);
		 printf("电话 : %s\n", msg->phone);
		 printf("住址 : %s\n", msg->addr);
	 }else{
		 printf("%s\n",msg->data);
	 }
	return;
}

/*
 *  Name   : do_usr_update 
 *  Description : 修改员工自己信息
 *  Input  : 
 *  Output :  
 */
void do_usr_update(int socketfd,MSG *msg)
{
	printf("输入修改名字:");
	scanf("%s",msg->name);
	getchar();                            //吃掉垃圾字符\

	printf("输入修改登录密码:");
	scanf("%s",msg->password);
	getchar();                            //吃掉垃圾字符

	printf("输入修改年龄:");
	scanf("%d",&msg->age);
	getchar();                           //吃掉垃圾字符
	
	printf("输入修改邮箱:");
	scanf("%s",msg->mail);
	getchar();                            //吃掉垃圾字符

	printf("输入修改电话:");
	scanf("%s",msg->phone);
	getchar();                           //吃掉垃圾字符

	printf("输入修改住址:");
	scanf("%s",msg->addr);
	getchar();                            //吃掉垃圾字符

	if(send(socketfd,msg,sizeof(MSG),0) < 0){
		perror("usr_query_send error.");
		return;
	}

	if(recv(socketfd,msg,sizeof(MSG),0) < 0){
		perror("usr_query_recv error.");
		return;
	}

	 if(strcmp(msg->data,"员工信息修改完成") == 0){
		 printf("%s\n",msg->data);
		 printf("更新后内容为：\n");
		 printf("ID : %d\n",msg->id);
		 printf("姓名 : %s\n", msg->name);
		 printf("登录密码 : %s\n", msg->password);
		 printf("年龄 : %d\n", msg->age);
		 printf("工资 : %d\n", msg->money);
		 printf("邮箱 : %s\n", msg->mail);
		 printf("职位 : %s\n", msg->pos);
		 printf("电话 : %s\n", msg->phone);
		 printf("住址 : %s\n", msg->addr);
	 }else{
		 printf("%s\n",msg->data);
	 }
	return;
}

/*
 *  Name   : do_preview 
 *  Description : 浏览所有员工信息
 *  Input  : 
 *  Output :  
 */
void do_preview(int socketfd,MSG *msg)
{	
	while(1){
		if(recv(socketfd,msg,sizeof(MSG),0) < 0){
			perror("perror error.");
			return;
		}

		if(strcmp(msg->data,"no data") == 0){
			return;
		}else{
			printf("ID   : %d\n", msg->id);
			printf("姓名 : %s\n", msg->name);
			printf("年龄 : %d\n", msg->age);
			printf("工资 : %d\n", msg->money);
			printf("邮箱 : %s\n", msg->mail);
			printf("职位 : %s\n", msg->pos);
			printf("电话 : %s\n", msg->phone);
			printf("住址 : %s\n\n", msg->addr);
		}
	}
	return;
}
 
