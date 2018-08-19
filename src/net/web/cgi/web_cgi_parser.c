/*------------------------------------------------
* FILE: web_cgi_parser.c

* DESCRIPTION:
	��ҳ�������򣬸������cgi��ҳ��ģ��php����
	
* REVISION HISTORY:               statement                    by
	2012.9.26                                               lixingfu
--------------------------------------------------*/

/*-------------------------------------------------
	CGI��̹淶:
	1.CGI�����������<?cgi  ?>֮�У�������Ϊ��ͨ��HTML����.
	2.<?cgi  ?>����Ҫ�ɶԳ���,web_cgi_parser����ⲻ������
	3.ÿ��ָ�������';'��Ϊ������.
	4.CGI���������web_cgi_funs������.
--------------------------------------------------*/
#include "config.h"
#include "net.h"
#include "http.h"
#include "web_server.h"
#include "web_cgi_parser.h"
#include "web_cgi_fun.h"

#ifdef WEB_SERVER_SUPPORT

/*CGI�������ʼ�ָ����ͽ����ָ���*/
#define CGI_START_DELIM	"<?cgi "
#define CGI_END_DELIM	"?>"

/*
	�ָ��������,�ո�,�������ַ���ʹ��""������
	cmd_line ������
	����token�ĸ���
*/
int cgi_line2tokens(char * cgi_cmd_line, char *tokens[])
{
	int tokensNum;
	int i, j;
	BOOL sep;/*���ǰ����ַ��Ƿ�Ϊ�ָ���*/
	
	if ((cgi_cmd_line == NULL) || (strlen(cgi_cmd_line) < 1))
	  return 0;

	tokensNum = 0;
	/* parse the line */
	i = 0;
	j = 0;
	sep = TRUE;
	
	while(cgi_cmd_line[i])
	{
		/*��������ţ��������Ƿ�������,�ҵ�����ĵ�һ������,ȥ�����Ŵճ�һ���ַ���*/
		if(cgi_cmd_line[i] == '"')
		{
			j = i+1;
			while(cgi_cmd_line[j])
			{
				if(cgi_cmd_line[j] == '"')
				{			
					/*�ҵ�һ�����������������ַ���,ȥ������*/
					tokens[tokensNum++] = &cgi_cmd_line[i+1];
					cgi_cmd_line[j] = '\0';

					/*iָ��"���һ���ַ�*/
					i = j+1;
					break;
				}
				j++;
			}
		}


		/*�ո��,��Ϊ��ͨ�ķָ���,ȫ����Ϊ0*/
		if(cgi_cmd_line[i] == ' ' || cgi_cmd_line[i] == ',')
		{
			sep = TRUE;
			cgi_cmd_line[i] = '\0';
		}
		/*��ͨ���ַ�*/
		else
		{
			if(sep)
			{
				/*��һ���Ƿָ���,���Ա�����Ϊһ��token*/
				tokens[tokensNum++] = &cgi_cmd_line[i];
				sep = FALSE;
			}
		}
		
		i++;
	}

	return tokensNum;
}


/*
	����cgi��������,�ǳ�������cmd_input
	cgi_line����;��β
	����������outbuf��,��ǰ����������䵽��λ��
*/
char* cgi_cmd_execute(char *outbuf, 
			char *cgi_cmd_line,
			HTTP_RQST_HEAD *http_rqst_head, 
			HTTP_RESP_HEAD *http_resp_head)
{
	int i=0;
	int tokensNum;
	char * cgi_cmd_tokens[MAX_CGI_CMD_TOKENS_NUM] = {NULL};
	
/*----1. �����������ּ������----------------------*/
	tokensNum = cgi_line2tokens(cgi_cmd_line, cgi_cmd_tokens);
	if(tokensNum == 0)
		return outbuf;

	NET_LOG("cgi_cmd_execute: cgi_line2tokens=%d ", tokensNum,(int)cgi_cmd_tokens[0],0,0,0,0);

	for(i=0; i<tokensNum; i++)
	{
		NET_LOG("cmd%d=%s ", i,(int)cgi_cmd_tokens[i],0,0,0,0);
	}

	NET_LOG("\r\n", 0,0,0,0,0,0);
	
/*----2. ����web_cgi_funs��,�ҵ���Ӧ��ָ��---------*/
	i = 0;
	while(web_cgi_funs[i].name)
	{
		if(strcmp(web_cgi_funs[i].name, cgi_cmd_tokens[0]) == 0)
			break;
		i++;
	}

/*----3. ִ���ҵ�������--------------------*/
	if(web_cgi_funs[i].name)
	{
		NET_LOG("cgi_cmd_execute: %s\r\n", (int)web_cgi_funs[i].name,0,0,0,0,0);
		outbuf = (*web_cgi_funs[i].func)(tokensNum, cgi_cmd_tokens, outbuf, http_rqst_head, http_resp_head);
	}
		
	return 	outbuf;
}

/*
	����λ��<?cgi ?>֮�е�CGI����,
	cgi_startΪ<?cgi ֮��ĵ�һ������;
	����Ϊ?>
	�ȷ�Ϊcgi_cmd_line,Ȼ�����cgi_cmd_parse���д���
	��������ĵ�ַ
*/
char *cgi_parse(char *outbuf,
			const char *cgi_start,
			HTTP_RQST_HEAD *http_rqst_head, 
			HTTP_RESP_HEAD *http_resp_head)
{
	const char * ptr;
	static char cgi_cmd_line[MAX_CGI_CMD_LINE_LEN];

	NET_LOG("cgi_parse:\r\n",0,0,0,0,0,0);

	/*�����û�д���,�����,ֱ���˳�*/
	if(http_resp_head->exit != 0)
		return outbuf;

	while(1)
	{
		/*----1. ��cgi����ΰ���';'��Ϊ�����,�п��ܵ���β��?------*/
		ptr = http_strchrs(cgi_start, ";?");

		/*----2. �������------------------------------------------*/
		if(*ptr == '?')
		{ 
			NET_LOG("cgi_parse: ? return\r\n",0,0,0,0,0,0);

			return outbuf;
		}
		else
		/*----3. ��ÿһ���͸�cgi_cmd_execute����-------------------*/	
		if(*ptr == ';')
		{
			/*�����Ҫ��';'*/
			memcpy(cgi_cmd_line, cgi_start, ptr-cgi_start);
			cgi_cmd_line[ptr-cgi_start] = '\0';
				
			cgi_start = ptr+1;/*�Թ�';'*/

			NET_LOG("cgi_parse: cgi_cmd_line %s\r\n",(int)cgi_cmd_line,0,0,0,0,0);

 			/*�ŵ�������*/
			outbuf = cgi_cmd_execute(outbuf, cgi_cmd_line, http_rqst_head, http_resp_head);

			/*�����û�д���,�����,ֱ���˳�*/
			if(http_resp_head->exit != 0)
				return outbuf;
		}
		else
		{ 
			NET_LOG("cgi_parse: return\r\n",0,0,0,0,0,0);
			
			return outbuf;	
		}
	}
}

/*
	CGI��ҳ��������
	���е�CGI��ҳ���Ƚ���web_cgi_parser���н����ʹ���,�ŵ���������
	��ǰ�����,���س���
	<?cig ?>�м������ΪCGI����,����cgi_parse����
	�������������http_rqst_head��
	һЩ���ص����������http_resp_head,�������䣬������ȱʡ
*/
UINT web_cgi_parser(char *outbuf, 
			WEB_PAGE *page, 
			HTTP_RQST_HEAD *http_rqst_head, 
			HTTP_RESP_HEAD *http_resp_head)
{
	UINT len=0;
	const char *cgi_start, *cgi_end, *pp/*pageָ��*/;
	char *ptr;
	
	if(page->len == 0)
		return 0;

	len = 0;
	pp = page->addr;
	
	/*�м�����ж��CGI����*/
	while(1)
	{
		/*CGI�ָ���֮ǰ��֮������ݣ���Ҫ��������������*/
		cgi_start = strstr(pp, CGI_START_DELIM);

		/*----1. �ҵ���ʼ�ָ���,ptr1ָ����ʼ��-----------------------*/
		if(cgi_start)
		{
		/*----2. ��ppһֱ��cgi_start���������ݶ�������op��-----------*/
			memcpy(outbuf, pp, cgi_start-pp);
			outbuf += cgi_start-pp;/*outbuf�Ƶ��µ�λ��*/
			len += cgi_start-pp;/*�ܳ�������*/
			
		/*----3. ���ҽ�β��,cgi_endָ�������---------------------------*/
			cgi_end = strstr(cgi_start, CGI_END_DELIM);		
			if(cgi_end)
			{		
				pp = cgi_end+strlen(CGI_END_DELIM);
				/*��λ��CGI��ʽ��ʼ*/
				cgi_start += strlen(CGI_START_DELIM);

		/*----4. ���ҵ���CGI�����͸�cgi_parse����--------------------*/
				ptr = cgi_parse(outbuf, cgi_start, http_rqst_head, http_resp_head);
				len += ptr-outbuf;
				outbuf = ptr;
			}
		/*----5. ֻ�а�ͷû���ҵ���β,��cgi_start���ļ�����,������ͨ����----*/
			else
			{
				memcpy(outbuf, cgi_start, strlen(cgi_start));
				len += strlen(cgi_start);
				return len;
			}
		}
		else
		/*�Ҳ���CGI����ʼ����,���ϴ�p���ļ���β�����ݶ�������outbuf��*/
		{
			memcpy(outbuf, pp, strlen(pp));
			len += strlen(pp);/*�ܳ�������*/
			return len;
		}
	}
}
#endif

