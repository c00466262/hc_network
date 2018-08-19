/******************************************************************************
FILE:
	clicmd.c 
DESCRIPTION:
	functions in the  command line module.
REVISION HISTORY:
	2010.8.30  lixingfu create
*********************************************************************************/ 
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "config.h"

#include "cli.h"
#include "clicmd.h"
#include "climanager.h"
#include "clicmddef.h"
#include "clicmddef.def"
#include "uart0.h"
//#include "watchdog0.h"

#define CLI_KEY_TAB		0x09

#define CLI_ESC 		0x1B
#define ESC_KEY 		0x5B
#define ESC_KEY_UP 		0x41
#define ESC_KEY_DOWN 	0x42
#define ESC_KEY_RIGHT 	0x43
#define ESC_KEY_LEFT 	0x44
#define CLI_CMD_MAX_TABLE   10



static char cmd_table[CLI_CMD_MAX_TABLE][CMD_LINE_LENGTH]; //存放历史命令的数组表
static char cmd_num = 0; //记录历史命令行
static char cur_num = 0; //记录当前命令行

/* functions */
#ifdef CLI_SUPPORT
/*转义字符*/
unsigned char escSequences[] =
{
	0x18,			/* UP    =  4 */
	0x19,			/* DOWN  =  5 */
	0
};

/*CLI所能识别的字符*/
BOOL isExpectedChar(char ch)
{
	if(isprint(ch))
		return TRUE;
	else
		return FALSE;
}

BOOL get_input(char *ch, unsigned char escSequences[])
{
	escSequences = escSequences;
	if(uart0_get_char(ch))
		return TRUE;
	else
		return FALSE;
}
/*
	读取一条命令,返回下一步要进行的动作,
	命令结束的标志为"\n"或者是escSequences.
	同串口接口.
*/
unsigned char cmd_input(char * cmd_line, int line_len, unsigned char escSequences[])
{
#ifdef MULTI_TASK_SUPPORT
	unsigned char i = 0;/* 指向cmd_line的当前位置*/
	char ch;
#else
	static char cmd_len = 0;/* 指向cmd_line的最终长度位置*/
	static char cur_pos = 0; //指向cmd_line的当前位置
	static UCHAR last_action = 0;

	static  char i = 0;
	char ch;
#endif


#ifdef MULTI_TASK_SUPPORT
	while(1)
#endif		
	{
		if(get_input(&ch, escSequences))/*收到一个字符*/
		{
			/*回车换行，说明输入了一条命令，需要执行*/
			if(ch == '\r' || ch == '\n')
			{
				cmd_line[cmd_len] = '\0';/*收到结尾字符,自动附加0作为结尾*/
				if(strlen(cmd_line) > 0)
                {				
					//如果当前命令不是"h","H"，则将其保存到数组表内；若是相应命令，则不保存
					if(strcmp("h", cmd_line) != 0 && strcmp("H", cmd_line) != 0)
					{
						//超过数组表存放最大数(10)后，从表内第二行历史命令起将其位置依次向前移一行
						if(cmd_num == CLI_CMD_MAX_TABLE)
						{
							//如果当前命令与上一命令不同，则将其保存到数组表内；若相同，则不保存
							if(strcmp(cmd_table[cmd_num - 1], cmd_line) != 0)
							{
								for(i = 1; i < cmd_num; i++)
								{
									strcpy(cmd_table[i - 1], cmd_table[i]);
								}

								//将当前命令存放在数组表的最后一行
								strcpy(cmd_table[cmd_num - 1], cmd_line);
							}

							cur_num = cmd_num;							
						}
						else
						{
							strcpy(cmd_table[cmd_num], cmd_line);
                            cur_num = cmd_num;
							//如果当前命令与上一命令不同，则将其保存到数组表内；若相同，则不保存
							if(strcmp(cmd_table[cmd_num - 1], cmd_line) != 0)
							{
								cmd_num++;
								cur_num = cmd_num;
							}
						
						}
					}
				}
                cmd_len = 0;
				cur_pos = 0;
				return ACTION_CMD;
			}
			else
			/*删除一个字符*/
			if(ch == '\b')
			{
				if(cur_pos > 0)
				{
					CLI_PRINT("\b \b",0,0,0,0,0,0);
					if(cur_pos < cmd_len)
    				{
    					//将当前位置以后的所有字符前移一位
    					for(i = cur_pos; i < cmd_len; i++)
    					{
    						cmd_line[i - 1] = cmd_line[i];	
    					}
    					for(i = cur_pos; i < cmd_len; i++)
						{
							CLI_PRINT("%c", cmd_line[i - 1], 0, 0, 0, 0, 0);
						}
											
						CLI_PRINT(" \b", 0, 0, 0, 0, 0, 0);
					
										
    					//将光标移到当前位置
    					for(i = cur_pos; i < cmd_len; i++)
    					{					
    						CLI_PRINT("\b", 0, 0, 0, 0, 0, 0);
    					}
					}
					
                    cur_pos--;
                    cmd_len--;
				}
				#ifndef MULTI_TASK_SUPPORT
					return ACTION_NONE;
				#endif
			}
			else
			if(ch == '?')
			{
				return ACTION_QUESTION_MARK;
			}
			else
			/*输入tab键*/
			if(ch == CLI_KEY_TAB)
			{
				return ACTION_TAB;
			}
            else
			/*输入了转义字符*/
    		if(ch == CLI_ESC) //收到转义字符
    		{
    			last_action = ACTION_ESC;
    			return ACTION_ESC;
    		}
    		else
    		if(ch == ESC_KEY)
    		{
    			if(last_action == ACTION_ESC)
    			{
    				last_action = ACTION_ESC_KEY;
    				
    				return ACTION_ESC_KEY;
    			}
    			else
    			{
    				CLI_PRINT("%c", ch, 0, 0, 0, 0, 0);
    				cmd_line[cmd_len++] = ch;
    				cur_pos++;
    				
    				return ACTION_NONE;
    			}
    		}
    		else
    		if(ch == ESC_KEY_LEFT) //判断转义字符是否为方向键: "左"
    		{
    			if(last_action == ACTION_ESC_KEY)
    			{						
    				last_action = ACTION_NONE;

    				if(cur_pos > 0)
    				{
    					cur_pos--;
    					CLI_PRINT("\b", 0, 0, 0, 0, 0, 0);
    				
    					//return ACTION_LEFT;			
    				}
    				
    				return ACTION_LEFT;	
    			}
    			else
    			{
    				CLI_PRINT("%c", ch, 0, 0, 0, 0, 0);
    				cmd_line[cmd_len++] = ch;
    				cur_pos++;

    				return ACTION_NONE;
    			}
    		}
    		else
    		if(ch == ESC_KEY_RIGHT) //判断转义字符是否为方向键: "右"
    		{
    			if(last_action == ACTION_ESC_KEY)
    			{
    				last_action = ACTION_NONE;

    				if(cur_pos < cmd_len)
    				{

    					CLI_PRINT("%c", cmd_line[cur_pos++], 0, 0, 0, 0, 0);
    					
    					return ACTION_RIGHT;
    				}
    			}
    			else
    			{
    				CLI_PRINT("%c", ch, 0, 0 ,0 ,0 ,0);
    				cmd_line[cmd_len++] = ch;
    				cur_pos++;

    				return ACTION_NONE;
    			}
    		}
    		else
    		if(ch == ESC_KEY_UP) //判断转义字符是否为方向键: "上"
    		{
    			if(last_action == ACTION_ESC_KEY)
    			{
    				last_action = ACTION_NONE;
    												
					if(cur_num > 0)
					{							
						cur_num--;

						//清除当前字符串	
						for(i = 0; i < cmd_len; i++)
						{								
							CLI_PRINT("\b \b", 0, 0, 0, 0, 0, 0 );
						}
						
						cmd_len = 0;
						cur_pos = 0;

						CLI_PRINT("%s",(int)cmd_table[cur_num], 0, 0, 0, 0, 0);

						for(i = 0; i < strlen(cmd_table[cur_num]); i++)
						{						
							cmd_line[cmd_len++] = cmd_table[cur_num][i];
							cur_pos++;
						}					
					 }
    					
    					//如果移至表的第一个命令后，则返回到表的最后一个命令
    					/*else 
        					{
        						cur_num = cmd_num;
        					}*/
    			    return ACTION_NONE;															
    			}
    			else
    			{
    				CLI_PRINT("%c", ch, 0,0,0,0,0);
    				cmd_line[cmd_len++] = ch;
    				cur_pos++;

    				return ACTION_NONE;
    			}
    		}	
    		else
    		if(ch == ESC_KEY_DOWN) //判断转义字符是否为方向键: "下"
    		{
    			if(last_action == ACTION_ESC_KEY)
    			{
    				last_action = ACTION_NONE;
    																											
					if(cur_num < cmd_num - 1)
					{										
						cur_num++;

						//清除当前字符串
						for(i = 0; i < cmd_len; i++)
						{
							CLI_PRINT("\b \b", 0, 0, 0, 0, 0, 0);
						}

						cmd_len = 0;
						cur_pos = 0;

						CLI_PRINT("%s", (int)cmd_table[cur_num], 0, 0, 0 ,0, 0);
						
						for(i = 0; i < strlen(cmd_table[cur_num]); i++)
						{						
							cmd_line[cmd_len++] = cmd_table[cur_num][i];
							cur_pos++;
						}
					}
    					
					//如果移至表的最后一个命令后，则返回到表的第一个命令
					/*else
    					{
    						cur_num = -1;
    					}*/

    			    return ACTION_NONE;		
    			}
    			else
    			{
    				CLI_PRINT("%c", ch, 0, 0, 0, 0, 0);
    				cmd_line[cmd_len++] = ch;
    				cur_pos++;

    				return ACTION_NONE;
    			}
    		}	
			else
			{
				if(cur_pos >= line_len-1)
				{
					cmd_line[cmd_len] = '\0';/*超长算结尾*/
					cmd_len = 0;
				    cur_pos = 0;
					return ACTION_CMD;				
				}
				else
				{
					if(isExpectedChar(ch))/*直接收程序可识别的字符*/
					{
    					if(cur_pos < cmd_len)
                        {
        					//CLI_PRINT("2=%d", (int)cur_pos, 0, 0, 0, 0, 0);
        					//将插入位置以后的字符从最后一字符开始依次后移一位
        					for(i = cmd_len - 1; i >= cur_pos; i--)
        					{
        						cmd_line[i + 1] = cmd_line[i];
        						
        						//---wuhongxiang 20121127
        						if(i == 0)
								break;
        					}
        					
        					CLI_PRINT("%c", ch, 0, 0, 0, 0, 0);
        					cmd_line[cur_pos++] = ch;
        					cmd_len++;
      					
        					{
        						for(i = cur_pos; i < cmd_len; i++)
        						{
        							CLI_PRINT("%c", cmd_line[i], 0, 0, 0, 0, 0);							
        						}						
        					}

        					//将光标移到当前位置
        					for(i = cur_pos; i < cmd_len; i++)
        					{						
        						CLI_PRINT("\b", 0, 0, 0, 0, 0, 0);
        					}	
        				}
        				else
        				{
    						CLI_PRINT("%c", ch,0,0,0,0,0);
    						cmd_line[cmd_len++] = tolower(ch);
    						cur_pos++;
    					 }
						
					#ifndef MULTI_TASK_SUPPORT
						return ACTION_NONE;
					}
					else
					{
						return ACTION_NONE;
					#endif
					}
				}
			}
		}/*end if(get_input(&ch, escSequences))*/
	#ifndef MULTI_TASK_SUPPORT		
		else/*没有收到任何数据*/
		{
			return ACTION_NONE;
		}
	#endif		
	}/*end while(1)*/
	
	return ACTION_NONE;
}	

char *strtok( char *strTokens, const char *strDelimits)
{
	static char * next_tokens = NULL;/*用静态以保留下一个tokens的开头*/
	char *cur_strpos;
	char *token;
	
	if(strTokens == NULL)
	{
		cur_strpos = next_tokens;
	}
	else
	{
		next_tokens = strTokens;
		cur_strpos = strTokens;
	}

	if(cur_strpos == NULL || next_tokens == NULL)
		return NULL;

	/*搜索分隔符,得到后面的tokens*/
	token = NULL;

	while(*cur_strpos)
	{
		if(strchr(strDelimits, *cur_strpos))/*判断是否是分割符*/
		{
			*cur_strpos = '\0';

			cur_strpos++;

			if(token)
			{
				next_tokens = cur_strpos;/*得到后面的tokens*/
				break;
			}
		}
		else
		{
			if(token == NULL)
				token = cur_strpos;

			cur_strpos++;
		}
	}

	if(*cur_strpos == '\0')
		next_tokens = NULL;

	return token;
}


/*cmd_line 将被损坏*/
/*返回token的个数*/
unsigned char line2tokens(char * cmd_line, char ** tokens)
{
	char *token;
	unsigned char i = 0;
	
	if ((cmd_line == NULL) || (strlen(cmd_line) < 1))
	  return NULL;  
	 
	/* parse the line */
	i = 0;
	token = strtok(cmd_line, CLI_CMD_SEP_CHARS);

	while(token != NULL)
	{
		tokens[i++] = token;
	
		token = strtok(NULL, CLI_CMD_SEP_CHARS);
	}
	
	tokens[i] = NULL;
	
	return i;
}

BOOL matched_commands(char * cmd1[], char * cmd2[], unsigned char min)
{
	unsigned char  i = 0;

	if ((cmd1 == NULL) || (cmd2 == NULL) || (strcmp(cmd1[0], cmd2[0]) != 0))
		return 	FALSE;


	for(i = 1; i < min; i++)
	{
		if (strcmp(cmd1[i], cmd2[i]) != 0)
			return FALSE;
	}
	
	return TRUE;
}

/*寻找命令*/
int cliGetDefinedCmdIndex(char **tokens)
{
	int i, size;

	size = sizeof(cliDefinedCmdMapTab)/sizeof(cliDefinedCmdMapTab[0]);
	
	for(i=0; i<size; i++)
	{
		if (matched_commands(tokens,/*用户输入的命令,已被解析*/
			cliDefinedCmdMapTab[i].keyWords,/*关键字*/
			cliDefinedCmdMapTab[i].keyWMin))/*关键字的个数*/
		{
			return i;
		}
	}

	return -1;
}

static char cliPrintBuf[256];
void cli_print(char *fmt, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6)
{
	sprintf(cliPrintBuf, fmt, arg1, arg2, arg3, arg4, arg5, arg6);

	uart0_put_str(cliPrintBuf);
}

void cliCmdShell(void)
{
#ifdef MULTI_TASK_SUPPORT
	unsigned char		action;/*读取命令下一步要进行的动作*/
	unsigned char		tokensNum;
	int		i,num;
	int		CmdIndex;
	char cmd_line[CMD_LINE_LENGTH];
	char * cmd_tokens[CLI_CMD_MAX_TOKENS];
	char retInfo[CLI_CMD_MAX_RETINFO_LEN];
#else
	static unsigned char		action;/*读取命令下一步要进行的动作*/
	static unsigned char		tokensNum;
	int		i,num;
	int		CmdIndex;
	static char cmd_line[CMD_LINE_LENGTH];
	static char * cmd_tokens[CLI_CMD_MAX_TOKENS];
	static char  retInfo[CLI_CMD_MAX_RETINFO_LEN];
#endif

#ifdef MULTI_TASK_SUPPORT
	while(1)
#endif		
	{
		/*用户登录*/
		if(cliLogin() == ERROR)
		{	
			return;
		}
		
		action = cmd_input(cmd_line, CMD_LINE_LENGTH, escSequences);

		if(action == ACTION_NONE)
		{
		#ifdef MULTI_TASK_SUPPORT
			continue;
		#else
			return;
		#endif
		}
		else
		/*输入转义字符*/
    	if(action == ACTION_LEFT || 
			action == ACTION_RIGHT || 
			action == ACTION_UP ||
			action == ACTION_DOWN)
    	{
    		action = ACTION_CMD;
    		
    		return;
    	}
    	else
    	if(action == ACTION_QUESTION_MARK)
    	{
			i = 0;
			
			CLI_PRINT("\r\n",0,0,0,0,0,0);
			
			while (cliDefinedCmdMapTab[i].cmd != NULL) 
			//for(i=0; i<46; )
			{
				CLI_PRINT("%s\r\n", (int)cliDefinedCmdMapTab[i].cmd,0,0,0,0,0);
				i++;
			#ifdef WATCHDOG0_SUPPORT
				watchdog0Feed();
			#endif
			}

			CLI_PRINT("%s",(int)CLI_PROMPT,0,0,0,0,0);    		
    	}
		else
		/*按回车，输入了整条命令*/
		if(action == ACTION_CMD)
		{
			//CPU_LED = ~CPU_LED;
			
			if (strlen(cmd_line) == 0) 
			{
				CLI_PRINT("\r\n%s", (int)CLI_PROMPT,0,0,0,0,0);
				
			#ifdef MULTI_TASK_SUPPORT
				continue;
			#else
				return;
			#endif

			}
			
			/* parsing input command line into a list of tokens */
			tokensNum = line2tokens(cmd_line, cmd_tokens);

			/*  规定keyword必须在tokens的开头 */
		    if(strcmp("h", cmd_tokens[0]) == 0 || strcmp("H", cmd_tokens[0]) == 0)
    		{
				CLI_PRINT("\r\nThe History Command List(s):\r\n", 0, 0, 0, 0, 0, 0);

				for(num = 0; num < cmd_num; num++)
				{
					CLI_PRINT("%d: %s\r\n", (int)num, (int)cmd_table[num],0,0,0, 0);
				}
		
				CLI_PRINT("%s", (int)CLI_PROMPT, 0, 0, 0, 0, 0);
    		}
			else
			if (strcmp("help", cmd_tokens[0]) == 0 || strcmp("HELP", cmd_tokens[0]) == 0)    /* help command */
			{
				i = 0;
				
				CLI_PRINT("\r\n",0,0,0,0,0,0);
				
				while (cliDefinedCmdMapTab[i].cmd != NULL) 
				//for(i=0; i<46; )
				{
					CLI_PRINT("%s\r\n", (int)cliDefinedCmdMapTab[i].cmd,0,0,0,0,0);
					i++;
				#ifdef WATCHDOG0_SUPPORT
					watchdog0Feed();
				#endif
				}

				CLI_PRINT("%s",(int)CLI_PROMPT,0,0,0,0,0);
			}
			else
			{
				static USRCMD_PARA_STRUCT usrCmdParaStruct;
					
				CmdIndex = cliGetDefinedCmdIndex(cmd_tokens);
				if (CmdIndex >= 0) /* found a matching command. Call it. */
				{
					retInfo[0] = '\0';
					usrCmdParaStruct.argc = tokensNum;
					usrCmdParaStruct.argv = cmd_tokens;
					usrCmdParaStruct.retInfo = retInfo;
				
                    CLI_PRINT("\r\n",0,0,0,0,0,0);
					(* cliDefinedCmdMapTab[CmdIndex].func)(&usrCmdParaStruct);
					
					CLI_PRINT(retInfo,0,0,0,0,0,0);
					CLI_PRINT("\r\n%s", (int)CLI_PROMPT,0,0,0,0,0);
				}
				else
				{
					CLI_PRINT("\r\n",0,0,0,0,0,0);				
					CLI_PRINT("\r\nNo match command.Please check your syntax.\r\n",0,0,0,0,0,0);
					CLI_PRINT("\r\n%s", (int)CLI_PROMPT,0,0,0,0,0);
				}
			}
		}	/*end if(action == ACTION_CMD)*/
	}//	while(1)
}


void cliCmdShellStart(void)
{
	//char *p = 0;
	//p = cardTypes();
	
	CLI_PRINT("\r\n   %s \r\n", (int)systemDescr,0,0,0,0,0);
	CLI_PRINT("  Software Version %d.%d, CPU: K60DN512 ", (int)softVersion/10,(int)softVersion%10,0,0,0,0);
	//CLI_PRINT("Fpga Version %d.%d, %s\r\n", (int)(fpgaVersion/10),(int)(fpgaVersion%10),(int)creationDate,0,0,0);

	CLI_PRINT("\r\n",0,0,0,0,0,0);
	CLI_PRINT("%s",(int)CLI_PROMPT,0,0,0,0,0);
}
#endif

