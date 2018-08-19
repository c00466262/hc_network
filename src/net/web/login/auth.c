/********************************************************************************************
* FILE:	auth.c

* DESCRIPTION:
	授权配置，包括CLI、TELNET、WEB等的密码和认证
MODIFY HISTORY:
	2012.10.30            lixingfu   create
********************************************************************************************/

#include "config.h"
#include "auth.h"

/*
	用户名是保持不变的，只有三种
	只有admin的密码是保存在文件系统中的，缺省是hctel
*/
AUTH_USERS auth_users[] = 
{
/*	{"debug", "", priv_developer},*/
	{DLFT_AUTH_USRNAME, DLFT_AUTH_PASS, priv_admin},
/*	{"guest", "guest", priv_guest},*/
	{"",  "",  priv_null},
};

/*
	校验密码是否正确
*/
STATUS auth_pass_check(char *name, char *pass)
{
	int i=0;
	
	while(auth_users[i].username[0] != '\0')
	{		
		if(strcmp(auth_users[i].username, name) == 0)
		{
			if(strcmp(auth_users[i].password, pass)==0)
			{
				return OK;
			}
			else
				return ERROR;
		}
		i++;
	}

	return ERROR;
}

/*
	得到密码
*/
char* auth_pass_get(char *name)
{
	int i=0;
	
	while(auth_users[i].username[0] != '\0')
	{
		if(strcmp(auth_users[i].username, name) == 0)
		{
			return auth_users[i].password;
		}
		i++;
	}

	return NULL;
}


/*
	设置密码
*/
STATUS auth_pass_set(char *name, char*pass)
{
	int i=0;
	while(auth_users[i].username[0] != '\0')
	{
		if(strcmp(auth_users[i].username, name) == 0)
		{
			strncpy(auth_users[i].password, pass, MAX_PASSWORD_SIZE);
			auth_users[i].password[MAX_PASSWORD_SIZE-1] = '\0';
			return OK;
		}
		i++;
	}

	return ERROR;	
}

