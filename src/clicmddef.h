#ifndef __CLICMDDEF_H__
#define __CLICMDDEF_H__

char cli_show_run(USRCMD_PARA_STRUCT *usrCmdParaStruct);
char cli_show_dot3ah(USRCMD_PARA_STRUCT *usrCmdParaStruct);
char cli_set_rmt_lb(USRCMD_PARA_STRUCT *usrCmdParaStruct);
char cli_show_version(USRCMD_PARA_STRUCT *usrCmdParaStruct);
char cli_set_mac(USRCMD_PARA_STRUCT *usrCmdParaStruct);
char cli_set_ip(USRCMD_PARA_STRUCT *usrCmdParaStruct);
char cli_set_gateway(USRCMD_PARA_STRUCT *usrCmdParaStruct);

char cli_debug_net(USRCMD_PARA_STRUCT *usrCmdParaStruct);
char cli_no_debug_net(USRCMD_PARA_STRUCT *usrCmdParaStruct);

char cli_debug_dot3ah(USRCMD_PARA_STRUCT *usrCmdParaStruct);
char cli_no_debug_dot3ah(USRCMD_PARA_STRUCT *usrCmdParaStruct);

char cli_get_mii(USRCMD_PARA_STRUCT *usrCmdParaStruct);
char cli_set_mii(USRCMD_PARA_STRUCT *usrCmdParaStruct);

char cli_show_reg_arm(USRCMD_PARA_STRUCT *usrCmdParaStruct);



#endif

