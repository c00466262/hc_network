#ifndef __WEB_SERVER_H__
#define __WEB_SERVER_H__

#include "http.h"

/*��ҳ������*/
#define PAGE_TYPE_TEXT_HTML		0
#define PAGE_TYPE_IMAGE_JPEG	1
#define PAGE_TYPE_IMAGE_GIF		2
#define PAGE_TYPE_CGI			3
#define PAGE_TYPE_JS			4
#define PAGE_TYPE_CSS			5

/*ҳ���ļ��ṹ��Ķ���*/
typedef struct
{
	const char* name;/*�ļ�������*/
	UINT8 type;	/*�ļ�������*/
	UINT len;	/*��̬��ҳ������ͼƬ���ļ����ȣ�CIG�ļ�������Ҫ����ʱ����*/
	const char *addr;	/*�ļ��洢�ĵ�ַ*/
}WEB_PAGE;

/*��¼Session���壬�����û����Ȳ���*/
typedef struct
{
	char *name;	/*�ļ�������*/
	char type;	/*�ļ�������*/
	char *addr;	/*�ļ��洢�ĵ�ַ*/
}WEB_SESSION;


STATUS web_server_init(void);
STATUS web_server_rcve(HTTP_RQST_HEAD *http_rqst_head, UINT nr);

#endif
