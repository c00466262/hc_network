/*------------------------------------------------
* FILE: web_cgi_fun.c

* DESCRIPTION:
	WEB所有CGI函数
	
* REVISION HISTORY:               statement                    by
	2012.9.26                                               lixingfu
--------------------------------------------------*/
#include "config.h"
//#include "tick.h"
#include "web_server.h"
#include "web_cgi_fun.h"
#include "net.h"
#include "auth.h"

#ifdef WEB_SERVER_SUPPORT

/*
argv[1] 是指令本身
*/

/*
	CGI中的echo命令
*/
char* cgi_echo(int argc, char* argv[], char *outbuf, HTTP_RQST_HEAD *http_rqst_head, HTTP_RESP_HEAD *http_resp_head)
{
	int len = strlen(argv[1]);
	
	memcpy(outbuf, argv[1], len);
	
	return 	outbuf+len;
}

/*跳转到首页*/
char jump_index[] =  "<script LANGUAGE=\"JavaScript\">\r\n window.top.location.href = \"./index.cgi\";\r\n</script>";

/*跳转到登陆页面*/
char jump_login[] =  "<script LANGUAGE=\"JavaScript\">\r\n window.top.location.href=\"./login.htm\";\r\n</script>";

/*跳转到修改密码页面*/
char jump_chg_psw[] =  "<script LANGUAGE=\"JavaScript\">\r\n window.top.location.href = \"./chg_psw.htm\";\r\n</script>";

char jump_get_ip[] = "<script LANGUAGE=\"JavaScript\">\r\n window.location.href = \"./get_ip.cgi\";\r\n</script>";

char jump_get_e1[] = "<script LANGUAGE=\"JavaScript\">\r\n window.location.href = \"./get_e1.cgi\";\r\n</script>";

char error_ip[] = "<script LANGUAGE=\"JavaScript\">\r\nalert(\"ip地址输入有错误，请检查后重新输入!!\");\r\n</script>";
char error_mask[] = "<script LANGUAGE=\"JavaScript\">\r\nalert(\"子网掩码输入有错误，请检查后重新输入!!\");\r\n</script>";
char error_gateway[] = "<script LANGUAGE=\"JavaScript\">\r\nalert(\"缺省网关输入有错误，请检查后重新输入!!\");\r\n</script>";

char set_ip_ok[] = "<script LANGUAGE=\"JavaScript\">\r\nalert(\"修改成功并保存，请在3秒后使用新的设置重新连接！\");\r\n</script>";
char set_ip_failed[] = "<script LANGUAGE=\"JavaScript\">\r\nalert(\"设置失败，请检查设备！！！！\");\r\n</script>";

char error_e1_clock[] = "<script LANGUAGE=\"JavaScript\">\r\nalert(\"e1时钟输入有错误，只能是'master'或者是'slave'!!\");\r\n</script>";
char error_e1_speed[] = "<script LANGUAGE=\"JavaScript\">\r\nalert(\"e1速率输入有错误，只能是1~32之间!\");\r\n</script>";

char set_e1_ok[] = "<script LANGUAGE=\"JavaScript\">\r\nalert(\"修改成功并保存！\");\r\n</script>";

char error_no_username[] = "<script LANGUAGE=\"JavaScript\">\r\nalert(\"请输入用户名！\");\r\n</script>";
char error_no_password[] = "<script LANGUAGE=\"JavaScript\">\r\nalert(\"请输入密码！\");\r\n</script>";

char error_username_password[] = "<script LANGUAGE=\"JavaScript\">\r\nalert(\"用户名或者密码输入错误，请重新输入！\");\r\n</script>";

char error_new_password[] = "<script LANGUAGE=\"JavaScript\">\r\nalert(\"请输入新密码！\");\r\n</script>";
char set_password_ok[] = "<script LANGUAGE=\"JavaScript\">\r\nalert(\"密码修改成功，请重新登陆！\");\r\n</script>";

/*
	跳转到新的IP地址
*/
char *jump_index_newip(char *ip)
{
	/*跳转到首页*/
	static char ip_index[] =  "<script LANGUAGE=\"JavaScript\">\r\n window.top.location.ref=\"http://192.168.111.111\";\r\n</script>";

	sprintf(ip_index, 
		"<script LANGUAGE=\"JavaScript\">\r\n window.top.location.ref=\"http://%s\";\r\n</script>",
		ip);

	return ip_index;
}

/*
	登陆检测，设置COOKIE
*/
char *cgi_login(int argc, char* argv[], char *outbuf, HTTP_RQST_HEAD *http_rqst_head, HTTP_RESP_HEAD *http_resp_head)
{
	int len=0,j=0;
	char *usrname=NULL, *password=NULL;
	char *action=NULL;
	
/*----1. 查找usrname-----------------------------------*/
	usrname = http_post_arg_get(http_rqst_head, "USERNAME");
	if(usrname == NULL)
	{
		len = strlen(error_no_username);

		memcpy(outbuf, error_no_username, len);

		j = len;
		len = strlen(jump_login);

		memcpy(outbuf+j, jump_login, len);	
		j += len;

		return outbuf+j;
	}
	
/*----2. 查找password------------------------------------*/
	password = http_post_arg_get(http_rqst_head, "PASSWORD");
	if(password == NULL)
	{	
		len = strlen(error_no_password);

		memcpy(outbuf, error_no_password, len);

		j = len;
		len = strlen(jump_login);

		memcpy(outbuf+j, jump_login, len);	
		j += len;

		return outbuf+j;
	}	

/*----3. 判断用户名和密码是否正确------------------------*/
	/*----3.1 跳转到index,并设置COOKIE-----------------------------*/
	if(auth_pass_check(usrname, password) == OK)
	{
		action = http_post_arg_get(http_rqst_head, "LOGIN");
		
		/*如果是登陆*/	
		if(action)
		{
			//trace(TRACE_LEVEL_WARN, "%s:user %s login.\r\n",(int)sysUpTimeGet(), (int)usrname, 0,0,0,0);
			
			http_resp_cookie_set(http_resp_head, "USERNAME", usrname,NULL,NULL,NULL);
			http_resp_cookie_set(http_resp_head, "PASSWORD", password,NULL,NULL,NULL);
			
			len = strlen(jump_index);

			memcpy(outbuf, jump_index, len);

			return outbuf+len;
		}
		/*如果是修改密码*/
		else
		{
			//trace(TRACE_LEVEL_WARN, "%s:user %s change password.\r\n",(int)sysUpTimeGet(), (int)usrname, 0,0,0,0);
						
			len = strlen(jump_chg_psw);

			memcpy(outbuf, jump_chg_psw, len);

			return outbuf+len;			
		}
	}
	else
	/*----3.2 继续登录-----------------------------*/
	{
		len = strlen(error_username_password);

		memcpy(outbuf, error_username_password, len);

		j = len;
		len = strlen(jump_login);

		memcpy(outbuf+j, jump_login, len);	
		j += len;

		return outbuf+j;		
	}
}

/*
	从COOKIE中检测，用户名和密码是否正确，如果不正确，则跳转到登陆页面
*/
char *cgi_login_check(int argc, char* argv[], char *outbuf, HTTP_RQST_HEAD *http_rqst_head, HTTP_RESP_HEAD *http_resp_head)
{
	int len=0;
	char *usrname=NULL, *password=NULL;
	
/*----1. 查找usrname-----------------------------------*/
	usrname = http_cookie_arg_get(http_rqst_head, "USERNAME");
	if(usrname == NULL)
	{
		len = strlen(jump_login);

		memcpy(outbuf, jump_login, len);

		/*退出，不再执行*/
		http_resp_head->exit = 1;

		return outbuf+len;
	}
	
/*----2. 查找password------------------------------------*/
	password = http_cookie_arg_get(http_rqst_head, "PASSWORD");
	if(password == NULL)
	{	
		len = strlen(jump_login);

		memcpy(outbuf, jump_login, len);

		/*退出，不再执行*/
		http_resp_head->exit = 1;

		return outbuf+len;
	}	

/*----3. 判断用户名和密码是否正确------------------------*/
	if(auth_pass_check(usrname, password) != OK)
	{
		len = strlen(jump_login);

		memcpy(outbuf, jump_login, len);

		/*退出，不再执行*/
		http_resp_head->exit = 1;

		return outbuf+len;
	}

	return outbuf;
}


/*
	修改密码
*/
char *cgi_psw_chg(int argc, char* argv[], char *outbuf, HTTP_RQST_HEAD *http_rqst_head, HTTP_RESP_HEAD *http_resp_head)
{
	int len=0,j=0;
	char *usrname=NULL, *password=NULL;
	char *newpassword=NULL;

/*----1. 查找usrname-----------------------------------*/
	usrname = http_post_arg_get(http_rqst_head, "USERNAME");
	if(usrname == NULL)
	{
		len = strlen(jump_login);

		memcpy(outbuf, jump_login, len);

		return outbuf+len;
	}
	
/*----2. 查找password------------------------------------*/
	password = http_post_arg_get(http_rqst_head, "PASSWORD");
	if(password == NULL)
	{	
		len = strlen(jump_login);

		memcpy(outbuf, jump_login, len);

		return outbuf+len;
	}	

/*----3. 判断用户名和密码是否正确------------------------*/
	if(auth_pass_check(usrname, password) != OK)
	{
		len = strlen(error_username_password);

		memcpy(outbuf, error_username_password, len);

		j = len;
		len = strlen(jump_chg_psw);

		memcpy(outbuf+j, jump_chg_psw, len);	
		j += len;

		return outbuf+j;
	}

/*----4. 开始设置新的密码--------------------------------*/
	newpassword = http_post_arg_get(http_rqst_head, "NEWPASSWORD");
	/*设置失败，重新设置*/
	if(newpassword == NULL)
	{
		len = strlen(error_new_password);

		memcpy(outbuf, error_new_password, len);

		j = len;
		len = strlen(jump_chg_psw);

		memcpy(outbuf+j, jump_chg_psw, len);	
		j += len;

		return outbuf+j;
	}	
	else
	/*设置新密码成功*/
	{
		len = strlen(set_password_ok);

		memcpy(outbuf, set_password_ok, len);

		j = len;
		len = strlen(jump_login);

		memcpy(outbuf+j, jump_login, len);	
		j += len;

		auth_pass_set(usrname, newpassword);
		//systemConfigSave();
		
		return outbuf+j;
	}
}

char *cgi_sysname_get(int argc, char* argv[], char *outbuf, HTTP_RQST_HEAD *http_rqst_head, HTTP_RESP_HEAD *http_resp_head)
{
	int len = strlen(systemDescr);
	
	memcpy(outbuf, systemDescr, len);
	
	return 	outbuf+len;
}
char *cgi_version_get(int argc, char* argv[], char *outbuf, HTTP_RQST_HEAD *http_rqst_head, HTTP_RESP_HEAD *http_resp_head)
{
	int j=0;
	int len = 0;
	//len = strlen(softVersion);
	
	//memcpy(outbuf, softVersion, len);
	j = sprintf(outbuf,"software version %d.%d", softVersion/10, softVersion%10);

#ifdef FPGA_SUPPORT
	j = sprintf(outbuf+len,"&nbsp;&nbsp;;&nbsp;fpga version %d.%d", fpgaVersion/10, fpgaVersion%10);
#endif

	return 	outbuf+len+j;
}

char *cgi_systime_get(int argc, char* argv[], char *outbuf, HTTP_RQST_HEAD *http_rqst_head, HTTP_RESP_HEAD *http_resp_head)
{
	char *time = 0;//ticks2runningtime(tick_get());
	int len;
	
	len = strlen(time);
	
	memcpy(outbuf, time, len);
	
	return 	outbuf+len;
}

/*
	获取IP
	格式为
	IP地址		:getip ip
	子网掩码	:getip mask
	缺省网关	:getip gateway
*/
char *cgi_ip_get(int argc, char* argv[], char *outbuf, HTTP_RQST_HEAD *http_rqst_head, HTTP_RESP_HEAD *http_resp_head)
{
	char *ip;
	char *mask;
	char *gateway;

	int len = 0;
	
	if(strcmp(argv[1], "ip") == 0)
	{
		ip = inet_ntoa(my_ipaddr);
		len = strlen(ip);
			
		memcpy(outbuf, ip, len);
	}
	else
	if(strcmp(argv[1], "mask") == 0)
	{
		mask = inet_ntoa(my_subnetmask);
		len = strlen(mask);

		memcpy(outbuf, mask, len);
	}
	else
	if(strcmp(argv[1], "gateway") == 0)
	{
		gateway = inet_ntoa(gateway_ipaddr);
		len = strlen(gateway);

		memcpy(outbuf, gateway, len);
	}

	return outbuf+len;	
}

/*
	设置IP相关
*/
char *cgi_ip_set(int argc, char* argv[], char *outbuf, HTTP_RQST_HEAD *http_rqst_head, HTTP_RESP_HEAD *http_resp_head)
{
	int len,i, j;
	char *ip, *mask, *gateway;
	int IP=0, MASK=0, GATEWAY=0;
	
	/*查找IP*/
	ip = http_post_arg_get(http_rqst_head, "IP");	
	if(ip != NULL)
	{
		IP = inet_network(ip);
		if(IP == ERROR)
		{
			len = strlen(error_ip);

			memcpy(outbuf, error_ip, len);

			j = len;
			len = strlen(jump_get_ip);

			memcpy(outbuf+j, jump_get_ip, len);	
			j += len;

			return outbuf+j;	
		}
	}

	/*查找MASK*/
	mask = http_post_arg_get(http_rqst_head, "MASK");	
	if(mask != NULL)
	{
		MASK = inet_network(mask);
		if(MASK == ERROR)
		{
			len = strlen(error_mask);

			memcpy(outbuf, error_mask, len);	

			j = len;
			len = strlen(jump_get_ip);

			memcpy(outbuf+j, jump_get_ip, len);	
			j += len;

			return outbuf+j;	
		}		
	}

	/*查找gateway*/
	gateway = http_post_arg_get(http_rqst_head, "GATEWAY");
	if(gateway != NULL)
	{
		GATEWAY = inet_network(gateway);
		if(GATEWAY == ERROR)
		{
			len = strlen(error_gateway);

			memcpy(outbuf, error_gateway, len);	

			j = len;
			len = strlen(jump_get_ip);

			memcpy(outbuf+j, jump_get_ip, len);	
			j += len;

			return outbuf+j;	
		}
	}

/*----1. 放到保存缓冲区--------------------------*/	
	//systemSaveBuf.ifIpAddr = IP;
	//systemSaveBuf.ifNetMask = MASK;
	//systemSaveBuf.ifGateway = GATEWAY;

	/*让MAC地址自动跟随IP，这样有助于工程*/
	for(i=0, j = 2; i<4 && j<6; i++, j++)
	{
		//systemSaveBuf.ifMacAddr[j] = ((UINT8 *)&IP)[3-i];
	}
	
/*----2. 写入文件系统----------------------------*/
	//SSIFlashSectorErase(FLASH_SYSTEM_OFFSET, TRUE);
	//SSIFlashWrite(FLASH_SYSTEM_OFFSET, sizeof(systemSaveBuf), (UCHAR *)&systemSaveBuf);
/*----3. 报告正确--------------------------------*/
	len = strlen(set_ip_ok);

	memcpy(outbuf, set_ip_ok, len);

	j = len;
	len = strlen(jump_index_newip(ip));

	memcpy(outbuf+j, jump_index_newip(ip), len);	
	j += len;

/*---清除原来的COOKIE-*/
			
	http_resp_cookie_set(http_resp_head, "USERNAME", NULL,NULL,NULL,NULL);
	http_resp_cookie_set(http_resp_head, "PASSWORD", NULL,NULL,NULL,NULL);

	SET_BIT(event, EVENT_IP_NEW);
	
	return outbuf+j;
}


/*
	获取E1参数
	格式为
	时钟	:gete1 clock
	速率	:gete1 speed ts/all
*/
char *cgi_e1_get(int argc, char* argv[], char *outbuf, HTTP_RQST_HEAD *http_rqst_head, HTTP_RESP_HEAD *http_resp_head)
{
	int len=0;
	char buf[16];
	
	if(strcmp(argv[1], "clock") == 0)
	{
		//sprintf(buf, (e1ClockSpeed&E1_CLOCK_MASK) == 0?"master":"slave"); 
		len = strlen(buf);
			
		memcpy(outbuf, buf, len);
	}
	else
	if(strcmp(argv[1], "speed") == 0)
	{
		if(strcmp(argv[2], "ts") == 0)	/*时隙个数*/
		{
			//len = sprintf(buf, "%d", ((e1ClockSpeed&E1_SPEED_MASK)==0)?32:(e1ClockSpeed&E1_SPEED_MASK));
		}
		else	/*总带宽*/
		{
			//len = sprintf(buf, "%d",((e1ClockSpeed&E1_SPEED_MASK)==0)?2048:(e1ClockSpeed&E1_SPEED_MASK)*64);
		}
		
		memcpy(outbuf, buf, len);
	}

	return outbuf+len;	
}

/*
	获取E1参数
	格式为
	时钟	:gete1 clock
	速率	:gete1 speed ts/all
*/
char *cgi_e1_set(int argc, char* argv[], char *outbuf, HTTP_RQST_HEAD *http_rqst_head, HTTP_RESP_HEAD *http_resp_head)
{
	char *clock, *speed;
	int ts=0;
	int len=0,j=0;
	
	/*查找clock*/
	clock = http_post_arg_get(http_rqst_head, "E1CLOCK");	
	if(clock != NULL)
	{
		if(strcmp(clock , "master") !=0 && strcmp(clock , "slave") !=0 )
		{
			len = strlen(error_e1_clock);

			memcpy(outbuf, error_e1_clock, len);

			j = len;
			len = strlen(jump_get_e1);

			memcpy(outbuf+j, jump_get_e1, len);	
			j += len;

			return outbuf+j;	
		}
	}

	/*查找speed*/
	speed = http_post_arg_get(http_rqst_head, "E1SPEED");	
	if(speed != NULL)
	{
		ts = atoi(speed);
		if(!(1 <= ts && ts <= 32))
		{
			len = strlen(error_e1_speed);

			memcpy(outbuf, error_e1_speed, len);	

			j = len;
			len = strlen(jump_get_e1);

			memcpy(outbuf+j, jump_get_e1, len);	
			j += len;

			return outbuf+j;	
		}		
	}

	/*正式设置*/
	if(strcmp(clock, "master") == 0)
	{
		//CLR_BIT(e1ClockSpeed, E1_CLOCK_MASK);
	}
	else
	if(strcmp(clock, "slave") == 0)
	{
		//SET_BIT(e1ClockSpeed, E1_CLOCK_MASK);		
	}
	
	if(ts == 32)
	{
		//CLR_BIT(e1ClockSpeed, E1_SPEED_MASK);
	}
	else
	{
		//SET_BITS(e1ClockSpeed, E1_SPEED_MASK, ts);
	}

	/*保存*/
	//systemConfigSave();

	/*返回正确信息*/
/*----3. 报告正确--------------------------------*/
	len = strlen(set_e1_ok);

	memcpy(outbuf, set_e1_ok, len);

	j = len;
	len = strlen(jump_get_e1);

	memcpy(outbuf+j, jump_get_e1, len);	
	j += len;
	
	return outbuf+j;	
}

/*
	定义CGI的指令映射表.
	只支持单命令指令
*/
WEB_CGI_FUN web_cgi_funs[] = 
{
	{"echo", 		cgi_echo},
	{"getsysname", 	cgi_sysname_get},
	{"getversion", 	cgi_version_get},
	{"getsystime", 	cgi_systime_get},
	
	{"getip", 		cgi_ip_get},
	{"setip", 		cgi_ip_set},

	{"gete1", 		cgi_e1_get},
	{"sete1", 		cgi_e1_set},

	{"login", 		cgi_login},
	{"logincheck", 	cgi_login_check},

	{"chgpsw", 	cgi_psw_chg},
	
	{NULL, NULL},
};

#endif

