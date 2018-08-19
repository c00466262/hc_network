/*------------------------------------------------
* FILE: web_cgi_parser.c

* DESCRIPTION:
	网页解析程序，负责解析cgi网页。模仿php解析
	
* REVISION HISTORY:               statement                    by
	2012.9.26                                               lixingfu
--------------------------------------------------*/

/*-------------------------------------------------
	CGI编程规范:
	1.CGI的命令必须在<?cgi  ?>之中，否则作为普通的HTML处理.
	2.<?cgi  ?>必须要成对出现,web_cgi_parser还检测不到单个
	3.每条指令及参数以';'作为结束符.
	4.CGI的命令都放在web_cgi_funs数组中.
--------------------------------------------------*/
#include "config.h"
#include "net.h"
#include "http.h"
#include "web_server.h"
#include "web_cgi_parser.h"
#include "web_cgi_fun.h"

#ifdef WEB_SERVER_SUPPORT

/*CGI程序的起始分隔符和结束分隔符*/
#define CGI_START_DELIM	"<?cgi "
#define CGI_END_DELIM	"?>"

/*
	分割符可以是,空格,整个的字符串使用""引起来
	cmd_line 将被损坏
	返回token的个数
*/
int cgi_line2tokens(char * cgi_cmd_line, char *tokens[])
{
	int tokensNum;
	int i, j;
	BOOL sep;/*标记前面的字符是否为分隔符*/
	
	if ((cgi_cmd_line == NULL) || (strlen(cgi_cmd_line) < 1))
	  return 0;

	tokensNum = 0;
	/* parse the line */
	i = 0;
	j = 0;
	sep = TRUE;
	
	while(cgi_cmd_line[i])
	{
		/*如果是引号，看后面是否还有引号,找到后面的第一个引号,去掉引号凑成一个字符串*/
		if(cgi_cmd_line[i] == '"')
		{
			j = i+1;
			while(cgi_cmd_line[j])
			{
				if(cgi_cmd_line[j] == '"')
				{			
					/*找到一个以引号引起来的字符串,去掉引号*/
					tokens[tokensNum++] = &cgi_cmd_line[i+1];
					cgi_cmd_line[j] = '\0';

					/*i指向"后第一个字符*/
					i = j+1;
					break;
				}
				j++;
			}
		}


		/*空格和,作为普通的分隔符,全部置为0*/
		if(cgi_cmd_line[i] == ' ' || cgi_cmd_line[i] == ',')
		{
			sep = TRUE;
			cgi_cmd_line[i] = '\0';
		}
		/*普通的字符*/
		else
		{
			if(sep)
			{
				/*上一个是分隔符,所以本次作为一个token*/
				tokens[tokensNum++] = &cgi_cmd_line[i];
				sep = FALSE;
			}
		}
		
		i++;
	}

	return tokensNum;
}


/*
	解析cgi的命令行,非常类似于cmd_input
	cgi_line是以;结尾
	将数据填在outbuf中,从前向后填，返回填充到的位置
*/
char* cgi_cmd_execute(char *outbuf, 
			char *cgi_cmd_line,
			HTTP_RQST_HEAD *http_rqst_head, 
			HTTP_RESP_HEAD *http_resp_head)
{
	int i=0;
	int tokensNum;
	char * cgi_cmd_tokens[MAX_CGI_CMD_TOKENS_NUM] = {NULL};
	
/*----1. 分析出命令字及其参数----------------------*/
	tokensNum = cgi_line2tokens(cgi_cmd_line, cgi_cmd_tokens);
	if(tokensNum == 0)
		return outbuf;

	NET_LOG("cgi_cmd_execute: cgi_line2tokens=%d ", tokensNum,(int)cgi_cmd_tokens[0],0,0,0,0);

	for(i=0; i<tokensNum; i++)
	{
		NET_LOG("cmd%d=%s ", i,(int)cgi_cmd_tokens[i],0,0,0,0);
	}

	NET_LOG("\r\n", 0,0,0,0,0,0);
	
/*----2. 查找web_cgi_funs表,找到对应的指令---------*/
	i = 0;
	while(web_cgi_funs[i].name)
	{
		if(strcmp(web_cgi_funs[i].name, cgi_cmd_tokens[0]) == 0)
			break;
		i++;
	}

/*----3. 执行找到的命令--------------------*/
	if(web_cgi_funs[i].name)
	{
		NET_LOG("cgi_cmd_execute: %s\r\n", (int)web_cgi_funs[i].name,0,0,0,0,0);
		outbuf = (*web_cgi_funs[i].func)(tokensNum, cgi_cmd_tokens, outbuf, http_rqst_head, http_resp_head);
	}
		
	return 	outbuf;
}

/*
	解析位于<?cgi ?>之中的CGI程序,
	cgi_start为<?cgi 之后的第一行命令;
	结束为?>
	先分为cgi_cmd_line,然后调用cgi_cmd_parse进行处理
	返回填充后的地址
*/
char *cgi_parse(char *outbuf,
			const char *cgi_start,
			HTTP_RQST_HEAD *http_rqst_head, 
			HTTP_RESP_HEAD *http_resp_head)
{
	const char * ptr;
	static char cgi_cmd_line[MAX_CGI_CMD_LINE_LEN];

	NET_LOG("cgi_parse:\r\n",0,0,0,0,0,0);

	/*检查有没有错误,如果有,直接退出*/
	if(http_resp_head->exit != 0)
		return outbuf;

	while(1)
	{
		/*----1. 把cgi程序段按照';'分为多个行,有可能到结尾符?------*/
		ptr = http_strchrs(cgi_start, ";?");

		/*----2. 程序结束------------------------------------------*/
		if(*ptr == '?')
		{ 
			NET_LOG("cgi_parse: ? return\r\n",0,0,0,0,0,0);

			return outbuf;
		}
		else
		/*----3. 把每一行送给cgi_cmd_execute处理-------------------*/	
		if(*ptr == ';')
		{
			/*最后不需要带';'*/
			memcpy(cgi_cmd_line, cgi_start, ptr-cgi_start);
			cgi_cmd_line[ptr-cgi_start] = '\0';
				
			cgi_start = ptr+1;/*略过';'*/

			NET_LOG("cgi_parse: cgi_cmd_line %s\r\n",(int)cgi_cmd_line,0,0,0,0,0);

 			/*放到缓冲区*/
			outbuf = cgi_cmd_execute(outbuf, cgi_cmd_line, http_rqst_head, http_resp_head);

			/*检查有没有错误,如果有,直接退出*/
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
	CGI网页解析程序
	所有的CGI网页，先进入web_cgi_parser进行解析和处理,放到缓冲区中
	从前往后放,返回长度
	<?cig ?>中间的数据为CGI程序,交给cgi_parse处理
	环境变量存放在http_rqst_head中
	一些返回的数据填充在http_resp_head,如果不填充，就是用缺省
*/
UINT web_cgi_parser(char *outbuf, 
			WEB_PAGE *page, 
			HTTP_RQST_HEAD *http_rqst_head, 
			HTTP_RESP_HEAD *http_resp_head)
{
	UINT len=0;
	const char *cgi_start, *cgi_end, *pp/*page指针*/;
	char *ptr;
	
	if(page->len == 0)
		return 0;

	len = 0;
	pp = page->addr;
	
	/*中间可能有多段CGI程序*/
	while(1)
	{
		/*CGI分隔符之前和之后的数据，都要拷贝到缓冲区中*/
		cgi_start = strstr(pp, CGI_START_DELIM);

		/*----1. 找到起始分隔符,ptr1指向起始符-----------------------*/
		if(cgi_start)
		{
		/*----2. 从pp一直到cgi_start的所有数据都拷贝到op中-----------*/
			memcpy(outbuf, pp, cgi_start-pp);
			outbuf += cgi_start-pp;/*outbuf移到新的位置*/
			len += cgi_start-pp;/*总长度增加*/
			
		/*----3. 查找结尾符,cgi_end指向结束符---------------------------*/
			cgi_end = strstr(cgi_start, CGI_END_DELIM);		
			if(cgi_end)
			{		
				pp = cgi_end+strlen(CGI_END_DELIM);
				/*定位到CGI正式开始*/
				cgi_start += strlen(CGI_START_DELIM);

		/*----4. 将找到的CGI程序送给cgi_parse处理--------------------*/
				ptr = cgi_parse(outbuf, cgi_start, http_rqst_head, http_resp_head);
				len += ptr-outbuf;
				outbuf = ptr;
			}
		/*----5. 只有包头没有找到包尾,从cgi_start到文件结束,当做普通发送----*/
			else
			{
				memcpy(outbuf, cgi_start, strlen(cgi_start));
				len += strlen(cgi_start);
				return len;
			}
		}
		else
		/*找不到CGI的起始符了,把上次p到文件结尾的数据都拷贝到outbuf中*/
		{
			memcpy(outbuf, pp, strlen(pp));
			len += strlen(pp);/*总长度增加*/
			return len;
		}
	}
}
#endif

