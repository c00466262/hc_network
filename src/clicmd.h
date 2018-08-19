#ifndef __CLICMD_H__
#define __CLICMD_H__

#define CMD_LINE_LENGTH			80/* max length of cmd line */
#define CLI_CMD_MAX_RETINFO_LEN	256

#define CLI_CMD_END_RECEVIE			
#define CLI_CMD_SEP_CHARS			" ,:"/*分割符,空格和逗号*/
#define CLI_CMD_MAX_TOKENS		12
#define CLI_CMD_MAX_KEYWORDS		5

#define CLI_PROMPT "-> "

/*动作*/
#define ACTION_NONE				0x00 /*不需要做任何动作*/
#define ACTION_CMD				0x01 /*响应一条命令*/
#define ACTION_ESC				0x02 
#define ACTION_ESC_KEY			0x03 
#define ACTION_UP				0x04
#define ACTION_DOWN				0x05
#define ACTION_RIGHT			0x06
#define ACTION_LEFT				0x07
#define ACTION_QUESTION_MARK	0x08
#define ACTION_TAB				0x09


/**********************************************************************************
	This defines the command to function mapping type.
 cmd:		command line as a char string 
 keyWMin:	min num of keywords in this command
 func:		points to the function that executes the command
**********************************************************************************/
typedef struct
{
	unsigned char argc;
	char** argv;
	char *retInfo;
} USRCMD_PARA_STRUCT;

typedef struct 
{
	char * cmd;
	char *keyWords[CLI_CMD_MAX_KEYWORDS];
	unsigned char    keyWMin;
	char    (*func)(USRCMD_PARA_STRUCT *);
} CLICMDMAP;

unsigned char cmd_input(char * cmd_line, int line_len, unsigned char escSequences[]);
#endif
