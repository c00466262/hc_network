/*------------------------------------------------
* FILE: web_server.c

* DESCRIPTION:
	此文件的作用是实现网页服务器的功能，具体的网页定义放在webpages.c中，解析网页的引擎放在
	webparser.c中
	
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
#include "web_page.h"
#include "web_cgi_parser.h"

#ifdef WEB_SERVER_SUPPORT

STATUS web_server_send()
{

	return OK;	
}

/*
	WEB server的接收入口
	HTTP已经把HTTP的包头解析完毕，放到http_rqst_head中
*/
STATUS web_server_rcve(HTTP_RQST_HEAD *http_rqst_head, UINT nr)
{
	int j=0;
	UINT len;
	
	WEB_PAGE *page;/*指向网页*/

	HTTP_RESP_HEAD *http_resp_head;/*HTTP应答头*/

	NET_LOG("web_server_rcve: \r\n", 0,0,0,0,0,0);

	http_resp_head = (HTTP_RESP_HEAD *)malloc(sizeof(HTTP_RESP_HEAD));
	if(http_resp_head == NULL)
	{
		trace(TRACE_LEVEL_ERROR, "web_server_rcve: http_resp_head malloc failed\r\n",0,0,0,0,0,0);
		return ERROR;
	}
	memset(http_resp_head, 0, sizeof(HTTP_RESP_HEAD));

	NET_LOG("web_server_rcve: search for page\r\n", 0,0,0,0,0,0);

/*----1. 如果URI为'/'，则直接找到缺省网页-------*/
	if(strcmp(http_rqst_head->URI, "/") == 0)
	{
		NET_LOG("web_server_rcve: default page\r\n", 0,0,0,0,0,0);
		page = &web_pages[0];
	}
	else
	{
/*----2. 匹配URI---------------------------------*/
		j = 0;
		while(web_pages[j].name != NULL)
		{
			if(strcmp(web_pages[j].name, http_rqst_head->URI) == 0)
			{
				break;
			}
			j++;
		}

		NET_LOG("web_server_rcve: end search\r\n", 0,0,0,0,0,0);

/*----3. 找到了精确匹配的网页---------------------*/		
		if(web_pages[j].name != NULL)
		{
			NET_LOG("web_server_rcve: find page\r\n", 0,0,0,0,0,0);
			
			page = &web_pages[j];
		}
		else
/*----4. 没有找到适配的网页，返回404 NOT FOUND---*/
		{	
			NET_LOG("web_server_rcve: not find page\r\n", 0,0,0,0,0,0);

			strcpy(http_resp_head->STATUS, "404 Not Found");
			http_resp_head->CONTENT_LENGTH = 0;
			strcpy(http_resp_head->CONTENT_TYPE, "text/html");

			http_send(NULL, nr, http_resp_head);

			free(http_resp_head);

			return OK;
		}
	}

/*----5. 根据不同的网页类型，发送具体的网页--------------------------*/	
	if(page->type == PAGE_TYPE_CGI)
	{
		/*预留包头的位置，其余全部由CGI填写HTML数据*/
		char *outbuf = (char *)net_tx_buf + LINK_HEAD_LEN + IP_HEAD_LEN + TCP_HEAD_LEN;

		strcpy(http_resp_head->STATUS, "200 OK");			/*CGI里面可以修改*/
		strcpy(http_resp_head->CONTENT_TYPE, "text/html");	/*CGI里面可以修改*/
		
		/*CGI处理,返回填充的内容长度*/
		len = web_cgi_parser(outbuf, page, http_rqst_head, http_resp_head);

		/*填充长度*/
		http_resp_head->CONTENT_LENGTH = len;
		
		http_send(outbuf, nr, http_resp_head);		
	}
	else
	{
		/*预留包头的位置，其余全部填写HTML数据*/
		char *outbuf = (char *)net_tx_buf + LINK_HEAD_LEN + IP_HEAD_LEN + TCP_HEAD_LEN;

		memcpy(outbuf, page->addr, page->len);

		strcpy(http_resp_head->STATUS, "200 OK");
		http_resp_head->CONTENT_LENGTH = page->len;

		switch(page->type)
		{
			case PAGE_TYPE_TEXT_HTML:		
				strcpy(http_resp_head->CONTENT_TYPE, "text/html");
				break;
				
			case PAGE_TYPE_IMAGE_JPEG:		
				strcpy(http_resp_head->CONTENT_TYPE, "image/jpeg");
				break;
				
			case PAGE_TYPE_IMAGE_GIF:		
				strcpy(http_resp_head->CONTENT_TYPE, "text/gif");
				break;
				
			case PAGE_TYPE_JS:		
				strcpy(http_resp_head->CONTENT_TYPE, "application/x-javascript");
				break;

			case PAGE_TYPE_CSS:		
				strcpy(http_resp_head->CONTENT_TYPE, "text/css");
				break;
				
			default:
				strcpy(http_resp_head->CONTENT_TYPE, "unknow");
				break;				
		}
		
		http_send(outbuf, nr, http_resp_head);
	}
	
	free(http_resp_head);

	return OK;
}

STATUS web_server_init(void)
{
	return OK;
}
#endif
