/*------------------------------------------------
* FILE: http.c

* DESCRIPTION:
	http模块，解析http的包头
	
* REVISION HISTORY:               statement                    by
	2012.9.26                                               lixingfu
--------------------------------------------------*/
#include <stdlib.h>
#include <string.h>
#include "stdio.h"
#include "config.h"
#include "tcp.h"
#include "http.h"
#include "web_server.h"

#ifdef HTTP_SUPPORT

/*------------GET 的HTTP头格式--------------------------------------*/
//	GET /index.html?T1=abc&T2=123 HTTP/1.1(CRLF)  -- 或者是GET / HTTP/1.1(CRLF)
//	Accept: application/x-shockwave-flash, image/gif, image/jpeg, image/pjpeg, image/pjpeg, application/vnd.ms-excel, application/vnd.ms-powerpoint, application/msword, application/x-ms-application, application/x-ms-xbap, application/vnd.ms-xpsdocument, application/xaml+xml, */*(CRLF)
//	Accept-Language: zh-cn(CRLF)
//	User-Agent: Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 5.1; Trident/4.0; .NET CLR 2.0.50727; .NET CLR 3.0.4506.2152; .NET CLR 3.5.30729)(CRLF)
//	Accept-Encoding: gzip, deflate(CRLF)
//	Host: 192.168.16.145(CRLF)
//	Connection: Keep-Alive(CRLF)
//	(CRLF)
/*------------------------------------------------------------------*/

/*------------POST 的头格式--------------------------------------*/
//	POST /login.cgi HTTP/1.1(CRLF)
//	Host: www.baidu.com(CRLF)
//	User-Agent: Mozilla/4.0(CRLF)
//	Content-Type: application/x-www-form-URLencoded(CRLF)
//	Content-Length: 33(CRLF)
//	Connection: Keep-Alive(CRLF)
//	(CRLF)
//	username=lixingfu&password=123456
//	(CRLF)
/*------------------------------------------------------------------*/

const char *crlf = "\r\n";/*回车换行,http包头行结束符*/
const char *crlf_crlf = "\r\n\r\n";/*回车换行回车换行，http包头结束符*/

/* -------------------------一个标准的头部-------------------------------*/
/*Content-Length和Content-Type是需要随着不同的包做不同的修改*/

UCHAR HTTP_HEADER[] = 
{
	"HTTP/1.1 "
	"200 "
	"OK\r\n"
	"Cache-control: no-cache\r\n"
	"Connection: Keep-Alive\r\n"
	"Content-Length:      \r\n"
	"Content-Type: text/html\r\n"
	"\r\n"
};

/*
	自己定义strtok
	在s中查找delim指明的字符，找到delim任何一个字符，则返回
*/
char *http_strchrs(const char *s, const char *delim)
{
	int j;
	int len = strlen(delim);

	while(*s)
	{
		for(j=0; j<len; j++)
		{
			if(*s == delim[j])
				return (char *)s;
		}
		s++;
	}

	return NULL;
}

/*
	自定义strstr
	更容易理解，效率更高
*/
char *http_strstr(const char *s1, const char *s2)
{
	int n;
	int len;
	if(!(*s2))
		return (char *)s1;

	len = strlen(s2);
	
	while (*s1)
	{
		for (n=0; n<len; n++)
		{
			if(*(s1 + n) != *(s2 + n))
				break;
		}

		if(n == len)
			return (char *)s1;
			
		s1++;
	}

	return NULL;
}

/*
	解析HTTP带来的参数，GET或者是POST时都可能带参数
	格式名字为:application/x-www-form-urlencoded
	参数格式为:T1=abc&T2=123&T3=2v234 
	返回解析到的参数个数
	模仿PHP的urldecode
*/
UINT http_url_decode(char *line, HTTP_ARG arg[MAX_HTTP_ARG_NUM])
{
	char *token;
	char *ptr;
	int i;
	int num;
	
	/*没有参数*/
	if(line[0] == '\0')
		return 0;

/*----1. 先把line从&拆开，放到arg.name中---------------*/
	/* parse the line */
	for(i = 0; i <MAX_HTTP_ARG_NUM; i++)
	{
		arg[i].name = NULL;
		arg[i].value = NULL;
	}

	i = 0;
	token = strtok((char *)line, "&");
	while(token != NULL)
	{
		/*只能保存前MAX_HTTP_ARG_NUM个参数*/
		if(i<MAX_HTTP_ARG_NUM)
		{
			arg[i++].name = token;

		}
		
		token = strtok(NULL, "&");
	}

	num = i;
/*----2. 再把name从=拆开，后面的放到value中*/
	for(i = 0; (i <MAX_HTTP_ARG_NUM) && (arg[i].name!=NULL); i++)
	{
		ptr = strchr((char *)arg[i].name, '=');
		
		if(ptr!=NULL)
		{
			/*将name结尾*/
			*ptr = '\0';

			/*指向value*/
			ptr++;
			arg[i].value=ptr;
		}
	}
	
	return num;
}

/*
	解析收到的COOKIE.
	COOKIE的格式为:
	Cookie: username=admin; password=hctel
	返回COOKIE个数
*/
UINT http_cookie_decode(char *line, HTTP_COOKIE cookie[MAX_HTTP_COOKIE_NUM])
{
	char *ptr;
	int i;
	int num;
	
	/*没有参数*/
	if(line[0] == '\0')
		return 0;

/*----1. 先把line从"; "拆开，放到cookie.name中---------------*/
	/* parse the line */
	for(i = 0; i <MAX_HTTP_COOKIE_NUM; i++)
	{
		cookie[i].name = NULL;
		cookie[i].value = NULL;
	}
	
	/*第一个token*/
	i = 0;
	ptr = line;
	cookie[i++].name = ptr;

	while(ptr != NULL)
	{
		ptr = strstr(ptr, "; ");
		if(ptr == NULL)
			break;

		*ptr = '\0';
		ptr += 2;/*略过"; "*/
		cookie[i++].name = ptr;
	}
	

	num = i;
/*----2. 再把name从=拆开，后面的放到value中*/
	for(i = 0; (i <MAX_HTTP_COOKIE_NUM) && (cookie[i].name!=NULL); i++)
	{
		ptr = strchr((char *)cookie[i].name, '=');
		
		if(ptr!=NULL)
		{
			/*将name结尾*/
			*ptr = '\0';

			/*指向value*/
			ptr++;
			cookie[i].value=ptr;
		}
	}

	return num;
}

/*
	得到GET参数
*/
char *http_get_arg_get(HTTP_RQST_HEAD *http_rqst_head, char *name)
{
	int i;
	for(i = 0; i<MAX_HTTP_ARG_NUM; i++)
	{
		if(strcmp(http_rqst_head->$_GET[i].name, name) == 0)
		{
			return http_rqst_head->$_GET[i].value;
		}
	}

	return NULL;
}

/*
	得到POST参数
*/
char *http_post_arg_get(HTTP_RQST_HEAD *http_rqst_head, char *name)
{
	int i;
	
	for(i = 0; i<MAX_HTTP_ARG_NUM; i++)
	{
		if(strcmp(http_rqst_head->$_POST[i].name, name) == 0)
		{
			return http_rqst_head->$_POST[i].value;
		}
	}
	
	return NULL;
}

/*
	得到COOKIE参数
*/
char *http_cookie_arg_get(HTTP_RQST_HEAD *http_rqst_head, char *name)
{
	int i;
	for(i = 0; i<MAX_HTTP_COOKIE_NUM; i++)
	{
		if(strcmp(http_rqst_head->$_COOKIE[i].name, name) == 0)
		{
			return http_rqst_head->$_COOKIE[i].value;
		}
	}

	return NULL;
}

/*
	设置COOKIE,暂时不支持path和domain
*/
STATUS http_resp_cookie_set(HTTP_RESP_HEAD *http_resp_head, 
		char*name, char*value, char*expire,char*domain, char*path)
{
	int i=0;
	
	while(i<MAX_HTTP_COOKIE_NUM)
	{
		/*找一个未用的COOKIE填充*/
		if(http_resp_head->$SET_COOKIE[i].name[0] == NULL)
		{
			strncpy(http_resp_head->$SET_COOKIE[i].name, name, MAX_HTTP_COOKIE_NAME_LEN);
			http_resp_head->$SET_COOKIE[i].name[MAX_HTTP_COOKIE_NAME_LEN-1] = '\0';

			if(value != NULL)
			{
				strncpy(http_resp_head->$SET_COOKIE[i].value, value, MAX_HTTP_COOKIE_VALUE_LEN);
				http_resp_head->$SET_COOKIE[i].value[MAX_HTTP_COOKIE_VALUE_LEN-1] = '\0';
			}
			else
			{
				/*如果为空，设置为deleted*/
				strcpy(http_resp_head->$SET_COOKIE[i].value, "deleted");
			}

			if(expire != NULL)
			{		
				strncpy(http_resp_head->$SET_COOKIE[i].expire, expire, MAX_HTTP_COOKIE_EXPIRE_LEN);
				http_resp_head->$SET_COOKIE[i].expire[MAX_HTTP_COOKIE_EXPIRE_LEN-1] = '\0';
			}
			else
			{
				http_resp_head->$SET_COOKIE[i].expire[0] = '\0';
			}


			if(domain != NULL)
			{			
				strncpy(http_resp_head->$SET_COOKIE[i].domain, domain, MAX_HTTP_COOKIE_DOMAIN_LEN);
				http_resp_head->$SET_COOKIE[i].domain[MAX_HTTP_COOKIE_DOMAIN_LEN-1] = '\0';
			}
			else
			{
				http_resp_head->$SET_COOKIE[i].domain[0] = '\0';
			}

			if(path != NULL)
			{							
				strncpy(http_resp_head->$SET_COOKIE[i].path, path, MAX_HTTP_COOKIE_PATH_LEN);
				http_resp_head->$SET_COOKIE[i].path[MAX_HTTP_COOKIE_PATH_LEN-1] = '\0';
			}
			else
			{
				http_resp_head->$SET_COOKIE[i].path[0] = '\0';
			}
			
			return OK;
		}
		i++;
	}

	/*没有空间放置COOKIE*/
	return ERROR;
}

/*
	得到一个数字
	len表示字符的宽度
*/
UINT http_num_decode(char *num_line)
{
	UINT j;
	char *ptr;
	UINT num;

	/*找到第一个数字字符*/
	for(j=0; num_line[j]; j++)
	{
		if('1'<=num_line[j] && num_line[j]<='9')
			break;
	}
	
	/*找到最后也没有发现数字*/
	if(num_line[j] == '\0')
		return 0;

	/*指向第一个数字字符*/
	ptr = num_line + j;  
	
	/*找到最后一个非数字字符*/
	for(j=0; num_line[j]!='\0'; j++)
	{
		if(!('1'<=ptr[j] && ptr[j]<='9'))
			break;
	}
	ptr[j] = '\0';

	/*prt已经指向数字,得到数字*/

	num = atoi(ptr);

	return num;
}

/*
	HTTP协议的解析,主要包括GET/POST,参数等,放到http_rqst_head中
	由于HTTP包头都是ASCII码型，所以使用char*作为参数而不是UCHAR*
*/
STATUS http_rqst_head_decode(char * inbuf, UINT len, HTTP_RQST_HEAD *http_rqst_head)
{
	char *http, *ptr, *http_t, *http_t_t;/*临时http*/
	int num;
	
	NET_LOG("http_rcve: len=%d\r\n", len,0,0,0,0,0);

	/*长度判断*/
	if(!((0<len) && (len<MAX_ETH_LEN-LINK_HEAD_LEN-IP_HEAD_LEN-TCP_HEAD_LEN)))
	{
		NET_LOG("http_rcve: len=%d error\r\n", len,0,0,0,0,0);
		return ERROR;
	}
	/*将整个HTTP数据最后一个字节置0,搜索时不至于越限*/
	http = inbuf;
	http[len] = '\0';

/*----0. 找到HTTP头尾部----------------------------------*/
	ptr = http_strstr(http, crlf_crlf);
	if(ptr == NULL)/*没有找到,是个违法包*/
	{
		NET_LOG("http_rcve: no found crlf_crlf %02x %02x %02x %02x %02x\r\n", http[len-4],http[len-3],http[len-2],http[len-1],http[len],http[len]);
		return ERROR;
	}

	/*把HTTP content之前的"\r\n"置'\0',减少搜索时间*/
	*(ptr+2) = '\0';	
	*(ptr+3) = '\0';

/*----1. 指向内容http_content----------------------------*/
	http_rqst_head->CONTENT = ptr + 4;

	if(http_rqst_head->CONTENT - http > len)/*长度超长*/
	{
		trace(TRACE_LEVEL_ERROR, "http_rcve: wrong length\r\n", 0,0,0,0,0,0);
	
		return ERROR;
	}
	
/*----2. 解析REQUEST_METHOD------------------------------*/
	ptr = strchr(http, ' ');/*REQUEST_METHOD 以空格结尾*/
	if(ptr == NULL)
	{
		trace(TRACE_LEVEL_ERROR, "http_rcve: wrong format in REQUEST_METHOD, offset=%d\r\n", http-inbuf,0,0,0,0,0);
		return ERROR;
	}

	*ptr = '\0';/*截断REQUEST_METHOD*/

	/*拷贝到http_rqst_head*/
	strncpy(http_rqst_head->REQUEST_METHOD, http, MAX_REQUEST_METHOD_LEN);
	http_rqst_head->REQUEST_METHOD[MAX_REQUEST_METHOD_LEN-1] = '\0';

/*----3. 解析URI,即要取的网页-------------------------*/	
	http = ptr + 1;	
	if(*http != '/')/*必须为'/'*/
	{
		trace(TRACE_LEVEL_ERROR, "http_rcve: wrong format in URI /, offset=%d\r\n", http-inbuf,0,0,0,0,0);
		return ERROR;
	}

	/*----查找URI尾，空格或者是问号-------------------*/
	ptr = http_strchrs(http, " ?");
	if(ptr == NULL)
	{
		trace(TRACE_LEVEL_ERROR, "http_rcve: wrong format int URT ?, offset=%d\r\n", http-inbuf,0,0,0,0,0);
		return ERROR;
	}

/*----4. 解析GET跟随的参数QUERY_STRING-----------------*/
	/*如果只有空格，没有参数，只有单文件*/
	if(*ptr == ' ')
	{
		*ptr = '\0';

		strncpy(http_rqst_head->URI, http, MAX_URI_LEN);
		http_rqst_head->URI[MAX_URI_LEN-1] = '\0';
		NET_LOG("http_rcve: URI=%s, no arg\r\n", (int)http_rqst_head->URI,0,0,0,0,0);
		
		/*http指向URI后面HTTP version*/
		ptr++;
		http = ptr;
	}
	else
	/*如果先找到了'?',说明后面有参数，继续解析参数*/
	if(*ptr == '?')
	{
		*ptr = '\0';

		strncpy(http_rqst_head->URI, http, MAX_URI_LEN);
		http_rqst_head->URI[MAX_URI_LEN-1] = '\0';
		NET_LOG("http_rcve: URI=%s\r\n", (int)http_rqst_head->URI,0,0,0,0,0);

		/*到达参数区(T1=abc&T2=123&T3=bb23423 )*/
		ptr++;
		http = ptr;

		/*寻找结尾的空格*/
		ptr = strchr(http, ' ');
		if(ptr == NULL)
			return ERROR;

		/*将QUERY_STRING截断*/
		*ptr = '\0';

		/*如果是GET,则直接解析出来放到&_GET中*/
		if(strcmp(http_rqst_head->REQUEST_METHOD, "GET") == 0)
		{
			num = http_url_decode(http, http_rqst_head->$_GET);
			NET_LOG("http_rcve: parse $_GET %d\r\n", num,0,0,0,0,0);
		}
		/*否则,缓冲区只是放到QUERY_STRING中*/
		else
		{
			http_rqst_head->QUERY_STRING = http;
			NET_LOG("http_rcve: QUERY_STRING=%s\r\n", (int)http_rqst_head->QUERY_STRING,0,0,0,0,0);
		}

		/*http指向URI后面HTTP version*/
		ptr++;
		http = ptr;
	}
	
/*----5. 解析HTTP VERSION------------------*/
	http_rqst_head->VERSION = ptr;
	/*查找"\r\n结尾符"*/
	ptr = strstr(http, crlf);
	if(ptr == NULL)
		return ERROR;

	*ptr = '\0';
	/*指向第2行*/
	http = ptr+2;
	
	/*读取一行，依次处理*/
	while(1)
	{
		ptr = http_strstr(http, crlf);
		if(ptr == NULL)
			break;

		*ptr = '\0';	/*把"\r\n"置零*/
		http_t = http;	/*暂存http*/

		http = ptr+2;	/*http指向下一行*/

		/*每一个关键字的结尾都是": "*/
		ptr = http_strstr(http_t, ": ");
		if(ptr == NULL)
			break;
		
		*ptr = '\0';		/*把": "置零*/
		http_t_t = 	http_t;	/*暂存http_t*/
		http_t = ptr+2;		/*http_t指向内容*/
		
		/*开始通过比较字符串，进行相应的处理*/
		
/*----6. 解析CONTENT_LENGTH--------------------------------------*/
		if(strcmp(http_t_t, "Content-Length") == 0)
		{
			NET_LOG("http_rcve: Searching for Content-Length\r\n", 0,0,0,0,0,0);				
			http_rqst_head->CONTENT_LENGTH = http_num_decode(http_t);
			NET_LOG("http_rcve: Content-Length=%d\r\n", http_rqst_head->CONTENT_LENGTH,0,0,0,0,0);		
		}
		else
/*----7. 解析CONTENT_TYPE---------------------------------------*/		
		if(strcmp(http_t_t, "Content-Type") == 0)				
		{
			NET_LOG("http_rcve: Searching for Content-Type\r\n", 0,0,0,0,0,0);
		
			strncpy(http_rqst_head->CONTENT_TYPE, http_t, MAX_CONTENT_TYPE_LEN);
			http_rqst_head->CONTENT_TYPE[MAX_CONTENT_TYPE_LEN-1] = '\0';

			NET_LOG("http_rcve: Content-Type=%s\r\n", (int)http_rqst_head->CONTENT_TYPE,0,0,0,0,0);
			
			/*如果CONTENT_TYPE为application/x-www-form-urlencoded，则说明CONTENT为编码格式,直接解码放到$_POST*/
			if(strcmp(http_rqst_head->CONTENT_TYPE, "application/x-www-form-urlencoded") == 0)
			{
				num = http_url_decode(http_rqst_head->CONTENT, http_rqst_head->$_POST);
				NET_LOG("http_rcve: Content urlencoded %d\r\n", num,0,0,0,0,0);				
			}			
		}
		else
/*----8. 解析COOKIE---------------------------------------------*/		
		if(strcmp(http_t_t, "Cookie") == 0)				
		{
			NET_LOG("http_rcve: Searching for Cookie\r\n", 0,0,0,0,0,0);

			num = http_cookie_decode(http_t, http_rqst_head->$_COOKIE);
			
			NET_LOG("http_rcve: Cookie %d\r\n", num,0,0,0,0,0);
		}		
	}
	
	return OK;
}

/*
	形成HTTP应答头,从后向前填充，假设前面字节足够
*/
char* http_resp_head_encode(char * outbuf, HTTP_RESP_HEAD *http_resp_head)
{
	static char tmpbuf[128];
	char *ptr;
	int len;
	int i;
	
/*----1. 先填写一个\r\n----------------------------------*/
	ptr = outbuf-2;
	memcpy(ptr, "\r\n", 2);/*不能使用strcpy，因为会在后面加一个'\0'*/

/*----2. 填写content_type---------------------------------*/
	sprintf(tmpbuf, "Content-Type: %s\r\n", http_resp_head->CONTENT_TYPE);
	len = strlen(tmpbuf);
	
	ptr -= len;
	memcpy(ptr, tmpbuf, len);/*不能使用strcpy，因为会在后面加一个'\0'*/
	
/*----3. 填写content_length---------------------------------*/
	sprintf(tmpbuf, "Content-Length: %d\r\n", http_resp_head->CONTENT_LENGTH);
	len = strlen(tmpbuf);
	
	ptr -= len;
	memcpy(ptr, tmpbuf, len);/*不能使用strcpy，因为会在后面加一个'\0'*/

/*----4. 填写cache_control---------------------------------*/
	sprintf(tmpbuf, "Cache-Control: no-cache\r\n");
	len = strlen(tmpbuf);
	
	ptr -= len;
	memcpy(ptr, tmpbuf, len);/*不能使用strcpy，因为会在后面加一个'\0'*/
	
/*----5. 填写connection-------------------------------------*/
	sprintf(tmpbuf, "Connection: Keep-Alive\r\n");
	len = strlen(tmpbuf);
	
	ptr -= len;
	memcpy(ptr, tmpbuf, len);/*不能使用strcpy，因为会在后面加一个'\0'*/
	
/*----6. 填写Set-Cookie,从后向前填充-------------------------*/
	for(i = MAX_HTTP_COOKIE_NUM-1; i>=0; i--)
	{	
		if(http_resp_head->$SET_COOKIE[i].name[0] == '\0')
			continue;
			
		/*先增加一个回车换行*/
		len = strlen(crlf);
		ptr -= len;
		memcpy(ptr, crlf, len);/*不能使用strcpy，因为会在后面加一个'\0'*/

		/*将path拷贝到发送缓冲区*/
		if(http_resp_head->$SET_COOKIE[i].path[0] != '\0')
		{
			len = strlen(http_resp_head->$SET_COOKIE[i].path);
			ptr -= len;
			memcpy(ptr, http_resp_head->$SET_COOKIE[i].path, len);/*不能使用strcpy，因为会在后面加一个'\0'*/

			len = strlen("; Path=");
			ptr -= len;
			memcpy(ptr, "; Path=", len);/*不能使用strcpy，因为会在后面加一个'\0'*/			
		}

		/*将domain拷贝到发送缓冲区*/
		if(http_resp_head->$SET_COOKIE[i].domain[0] != '\0')
		{
			len = strlen(http_resp_head->$SET_COOKIE[i].domain);
			ptr -= len;
			memcpy(ptr, http_resp_head->$SET_COOKIE[i].domain, len);/*不能使用strcpy，因为会在后面加一个'\0'*/

			len = strlen("; Domain=");
			ptr -= len;
			memcpy(ptr, "; Domain=", len);/*不能使用strcpy，因为会在后面加一个'\0'*/			
		}
		
		/*将expire拷贝到发送缓冲区*/
		if(http_resp_head->$SET_COOKIE[i].expire[0] != '\0')
		{
			len = strlen(http_resp_head->$SET_COOKIE[i].expire);
			ptr -= len;
			memcpy(ptr, http_resp_head->$SET_COOKIE[i].expire, len);/*不能使用strcpy，因为会在后面加一个'\0'*/

			len = strlen("; Expire=");
			ptr -= len;
			memcpy(ptr, "; Expire=", len);/*不能使用strcpy，因为会在后面加一个'\0'*/			
		}
		
		/*将value拷贝到发送缓冲区*/
		if(http_resp_head->$SET_COOKIE[i].value[0] != '\0')
		{
			len = strlen(http_resp_head->$SET_COOKIE[i].value);
			ptr -= len;
			memcpy(ptr, http_resp_head->$SET_COOKIE[i].value, len);/*不能使用strcpy，因为会在后面加一个'\0'*/
		}
		else
		{
			len = strlen("\"\"");
			ptr -= len;
			memcpy(ptr, "\"\"", len);/*不能使用strcpy，因为会在后面加一个'\0'*/			
		}

		len = strlen("=");
		ptr -= len;
		memcpy(ptr, "=", len);/*不能使用strcpy，因为会在后面加一个'\0'*/			

		/*将name拷贝到发送缓冲区*/
		len = strlen(http_resp_head->$SET_COOKIE[i].name);
		ptr -= len;
		memcpy(ptr, http_resp_head->$SET_COOKIE[i].name, len);/*不能使用strcpy，因为会在后面加一个'\0'*/

		/*放置Set-Cookie*/
		len = strlen("Set-Cookie: ");
		ptr -= len;
		memcpy(ptr, "Set-Cookie: ", len);/*不能使用strcpy，因为会在后面加一个'\0'*/
	}


/*----7. 填写Status-------------------------------------*/
	sprintf(tmpbuf, "HTTP/1.1 %s\r\n", http_resp_head->STATUS);
	len = strlen(tmpbuf);
	
	ptr -= len;
	memcpy(ptr, tmpbuf, len);/*不能使用strcpy，因为会在后面加一个'\0'*/

/*填充完毕,ptr为新的缓冲区位置*/
	return ptr;
}

/*
	HTTP接收处理
	首先解析HTTP头，然后送给web_server处理
*/
STATUS http_rcve(UCHAR * inbuf, UINT8 nr, UINT len)
{
	HTTP_RQST_HEAD *http_rqst_head;
	STATUS result;

	if (nr == NO_CONNECTION) 
		return ERROR;

	http_rqst_head = (HTTP_RQST_HEAD *)malloc(sizeof(HTTP_RQST_HEAD));
	if(http_rqst_head == NULL)
	{
		trace(TRACE_LEVEL_ERROR, "http_rcve: http_rqst_head malloc failed\r\n",0,0,0,0,0,0);
		return ERROR;
	}
	memset(http_rqst_head, 0, sizeof(HTTP_RQST_HEAD));

/*----1. HTTP协议解析----------*/	
	result = http_rqst_head_decode((char*)inbuf, len, http_rqst_head);
	if(result == ERROR)
	{
		/*释放分配的缓冲区*/
		free(http_rqst_head);
		
		/*返回错误页面*/
		return OK;
	}
	
/*----2. web_parser进行处理----*/
	web_server_rcve(http_rqst_head, nr);

	free(http_rqst_head);
	
	return OK;
}

/*
	HTTP发送
	inbuf已经填充了html信息,LEN代表填充了的长度
*/
STATUS http_send(char * outbuf, UINT8 nr, HTTP_RESP_HEAD* http_resp_head)
{
	char *http_head;
	
	/*如果为空，说明没有数据，则自动获取一个缓冲区*/
	if(outbuf == NULL)
	{
		outbuf = (char *)net_tx_buf+MAX_OF_NET_BUF-1;/*指向net_tx_buf尾*/
	}

/*----1. 创建HTTP包头,从inbuf继续向前----------------------*/
	http_head = http_resp_head_encode(outbuf, http_resp_head);
	
/*----2. 使用TCP发送---------------------------------------*/
	tcp_send((UCHAR *)http_head, nr, NULL, NULL, FLG_ACK, FALSE, (outbuf-http_head)+http_resp_head->CONTENT_LENGTH);

	return OK;
}

STATUS http_init(void)
{
	return OK;
}
#endif
