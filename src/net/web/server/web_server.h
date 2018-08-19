#ifndef __WEB_SERVER_H__
#define __WEB_SERVER_H__

#include "http.h"

/*网页的类型*/
#define PAGE_TYPE_TEXT_HTML		0
#define PAGE_TYPE_IMAGE_JPEG	1
#define PAGE_TYPE_IMAGE_GIF		2
#define PAGE_TYPE_CGI			3
#define PAGE_TYPE_JS			4
#define PAGE_TYPE_CSS			5

/*页面文件结构体的定义*/
typedef struct
{
	const char* name;/*文件的名字*/
	UINT8 type;	/*文件的类型*/
	UINT len;	/*静态网页或者是图片的文件长度，CIG文件长度需要运行时计算*/
	const char *addr;	/*文件存储的地址*/
}WEB_PAGE;

/*登录Session定义，放置用户名等参数*/
typedef struct
{
	char *name;	/*文件的名字*/
	char type;	/*文件的类型*/
	char *addr;	/*文件存储的地址*/
}WEB_SESSION;


STATUS web_server_init(void);
STATUS web_server_rcve(HTTP_RQST_HEAD *http_rqst_head, UINT nr);

#endif
