#ifndef __CLI_MANAGER_H__
#define __CLI_MANAGER_H__

typedef enum cliLevel 
{
	cliNormal, /*��ͨ�û�*/
	cliEnable, /*��Ȩ�û�*/
	cliConfig, /*configģʽ*/
	cliPort,   /*�˿�����ģʽ*/
	cliVlan,   /*vlan����ģʽ*/
	
}cliLevel_Type;

STATUS cliLogin(void);

#endif

