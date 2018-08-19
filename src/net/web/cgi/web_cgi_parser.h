#ifndef __WEB_CGI_PARSER_H__
#define __WEB_CGI_PARSER_H__


/*һ��CGI CMD�����*/
#define MAX_CGI_CMD_LINE_LEN		64

/*CGI֧�ֵ�tokens*/
#define MAX_CGI_CMD_TOKENS_NUM		12

typedef struct
{
	const char *name;
	char*    (*func)(int ,char **, char*, HTTP_RQST_HEAD *,HTTP_RESP_HEAD *);	
}WEB_CGI_FUN;

UINT web_cgi_parser(char *outbuf, WEB_PAGE *page, HTTP_RQST_HEAD *http_rqst_head, HTTP_RESP_HEAD *http_resp_head);

#endif
