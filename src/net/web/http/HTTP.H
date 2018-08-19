#ifndef __HTTP_H__
#define __HTTP_H__

#include "config.h"

/*长度定义*/
#define MAX_REQUEST_METHOD_LEN	8
#define MAX_URI_LEN				48	/*最长的为"application/x-www-form-urlencoded"*/
#define MAX_CONTENT_TYPE_LEN	64

#define MAX_STATUS_LEN			32

/*网页最多参数*/
#define MAX_HTTP_ARG_NUM		10
#define MAX_HTTP_COOKIE_NUM		10

/*COOKIE参数*/
#define MAX_HTTP_COOKIE_NAME_LEN	32
#define MAX_HTTP_COOKIE_VALUE_LEN	64
#define MAX_HTTP_COOKIE_EXPIRE_LEN	64
#define MAX_HTTP_COOKIE_PATH_LEN	64
#define MAX_HTTP_COOKIE_DOMAIN_LEN	64

/*HTTP的GET和POST参数*/
typedef struct
{
	char *name;
	char *value;
}HTTP_ARG;

/*HTTP的COOKIE*/
typedef struct
{
	char *name;
	char *value;
}HTTP_COOKIE;

/*HTTP的SET COOKIE格式*/
typedef struct
{
	char name[MAX_HTTP_COOKIE_NAME_LEN];
	char value[MAX_HTTP_COOKIE_VALUE_LEN];
	char expire[MAX_HTTP_COOKIE_EXPIRE_LEN];
	char domain[MAX_HTTP_COOKIE_DOMAIN_LEN];		
	char path[MAX_HTTP_COOKIE_PATH_LEN];
}HTTP_SET_COOKIE;

/*HTTP请求格式*/
typedef struct
{
	char REQUEST_METHOD[MAX_REQUEST_METHOD_LEN];	/*GET 或者是 POST*/
	char URI[MAX_URI_LEN];	/*URI*/
	char* QUERY_STRING;		/*GET 参数,直接指向整个字符串*/
	char *VERSION;
	
	UINT CONTENT_LENGTH;	/*HTTP内容长度*/
	char CONTENT_TYPE[MAX_CONTENT_TYPE_LEN];		/*content类型*/
	char* CONTENT;		/*指向HTTP content区*/

	/*提取出application/x-www-form-urlencoded编码的$_GET和$_POST*/
	HTTP_ARG $_GET[MAX_HTTP_ARG_NUM];
	HTTP_ARG $_POST[MAX_HTTP_ARG_NUM];
	HTTP_COOKIE $_COOKIE[MAX_HTTP_COOKIE_NUM];
}HTTP_RQST_HEAD;

/*HTTP应答格式*/
typedef struct
{
	UINT VERSION;	/*版本，恒定为1.1*/

	char STATUS[MAX_STATUS_LEN];	/*返回状态200,300,400等*/

	UINT8 CACHE_CONTROL;	/*固定为no-cache*/ 
	UINT8 CONNECTION;		/*固定为Keep-Alive*/ 

	UINT CONTENT_LENGTH;	/*HTTP内容长度*/
	char CONTENT_TYPE[MAX_CONTENT_TYPE_LEN];	/*content类型*/	

	HTTP_SET_COOKIE $SET_COOKIE[MAX_HTTP_COOKIE_NUM];		/*需要设置的*/

	UINT exit;/*退出标志,如果非0,说明有错误,不再往下处理,主要用于密码登陆*/
}HTTP_RESP_HEAD;

/*
常见HTTP状态码:
	200 OK 
	301 Moved Permanently 
	302 Found 
	304 Not Modified 
	307 Temporary Redirect 
	400 Bad Request 
	401 Unauthorized 
	403 Forbidden 
	404 Not Found 
	410 Gone 
	500 Internal Server Error 
	501 Not Implemented 
*/

/*
	http类型，格式为:
	Content-Type: text/html\r\n	
	Content-Type: text/xml; charset=utf-8\r\n
	Content-Type: image/jpeg\r\n
	Content-Type: image/gif\r\n	
	Content-Type: text/css\r\n
*/
STATUS http_init(void);
STATUS http_rcve(UCHAR * inbuf, UCHAR nr, UINT len);
STATUS http_send(char * outbuf, UINT8 nr, HTTP_RESP_HEAD* http_resp_head);


char *http_strchrs(const char *s, const char *delim);
char *http_strstr(const char *s1, const char *s2);

char *http_get_arg_get(HTTP_RQST_HEAD *http_rqst_head, char *name);
char *http_post_arg_get(HTTP_RQST_HEAD *http_rqst_head, char *name);
char *http_cookie_arg_get(HTTP_RQST_HEAD *http_rqst_head, char *name);
STATUS http_resp_cookie_set(HTTP_RESP_HEAD *http_resp_head, char*, char*, char*,char*, char*);
#endif
