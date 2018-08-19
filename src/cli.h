#ifndef __CLI_H__
#define __CLI_H__

#define CLIRET_OK			0
#define CLIRET_ERROR		1
#define CLIRET_EXIT			-1
#define CLI_RET_SET_OK		2

/*�Ƿ�֧�ֶ�����,����cli�Ƿ�����ѯ��ʽ��*/
//#define MULTI_TASK_SUPPORT
void cliCmdShell(void);
void cliCmdShellStart(void);
void cli_print(char *fmt, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6);

#define CLI_PRINT(fmt, arg1, arg2, arg3, arg4, arg5, arg6) cli_print(fmt, arg1, arg2, arg3, arg4, arg5, arg6)

#endif
