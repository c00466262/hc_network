#ifndef __CLI_MANAGER_H__
#define __CLI_MANAGER_H__

typedef enum cliLevel 
{
	cliNormal, /*普通用户*/
	cliEnable, /*特权用户*/
	cliConfig, /*config模式*/
	cliPort,   /*端口属性模式*/
	cliVlan,   /*vlan配置模式*/
	
}cliLevel_Type;

STATUS cliLogin(void);

#endif

