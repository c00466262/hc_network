/********************************************************************************************
* FILE:	auth.c

* DESCRIPTION:
	��Ȩ���ã�����CLI��TELNET��WEB�ȵ��������֤
MODIFY HISTORY:
	2012.10.30            lixingfu   create
********************************************************************************************/

#include "config.h"
#include "auth.h"

/*
	�û����Ǳ��ֲ���ģ�ֻ������
	ֻ��admin�������Ǳ������ļ�ϵͳ�еģ�ȱʡ��hctel
*/
AUTH_USERS auth_users[] = 
{
/*	{"debug", "", priv_developer},*/
	{DLFT_AUTH_USRNAME, DLFT_AUTH_PASS, priv_admin},
/*	{"guest", "guest", priv_guest},*/
	{"",  "",  priv_null},
};

/*
	У�������Ƿ���ȷ
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
	�õ�����
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
	��������
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

