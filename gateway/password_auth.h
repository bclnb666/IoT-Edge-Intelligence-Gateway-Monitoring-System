#ifndef __PASSWORD_AUTH_H__
#define __PASSWORD_AUTH_H__

/******************************
* 名    称 : shadow_authenticate()
* 功    能 : 终端无回显密码认证
* 返回参数 : 1 代表认证成功，0 代表认证失败退出
******************************/
int shadow_authenticate(void);

#endif // __SHADOW_AUTH_H__
