/*
	File: climanager.c
	Fun: ����CLI������������û���¼���û�֮����л���;
	create by yshs on 2014.06.03
*/
#include "config.h"
#include "string.h"
#include "cli.h"
#include "clicmd.h"

LOCAL BOOL cliLoginOk = FALSE;
LOCAL char* cliLoginID = "hctel";
LOCAL char* cliPassword = "hctel";

/*
	CLI��¼����
*/
STATUS cliLogin(void)
{
	#if 1
	return OK;
	#else
	static UCHAR actionCmdCnt = 0;
	static BOOL loginIdOk = FALSE;
	static char charBuf[20] = {0};

	/*�Ѿ����ڵ�¼״̬*/
	if(cliLoginOk == TRUE)
	{
		return OK;
	}
	
	if(cmd_input(charBuf, 20, NULL) == ACTION_CMD)
	{
		if(actionCmdCnt == 0)
		{
			if(strcmp(charBuf, cliLoginID) == 0)
			{
				loginIdOk = TRUE;
			}
			else
			{
				loginIdOk = FALSE;
			}
			
			CLI_PRINT("\r\npassword:",0,0,0,0,0,0);
			actionCmdCnt++;			
		}
		else
		if(actionCmdCnt == 1)
		{
			if((loginIdOk == TRUE) && (strcmp(charBuf, cliPassword) == 0))
			{
				cliLoginOk = TRUE;
				CLI_PRINT("\r\n%s",(int)CLI_PROMPT,0,0,0,0,0);
				return OK;
			}
			else
			{
				CLI_PRINT("\r\nuser or password error!",0,0,0,0,0,0);
				CLI_PRINT("\r\nlogin:",0,0,0,0,0,0);
			}

			actionCmdCnt = 0;
		}
	}

	return ERROR;
	#endif
}

