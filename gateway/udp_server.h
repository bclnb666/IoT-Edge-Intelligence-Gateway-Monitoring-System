#ifndef __UDP_SERVER_H__
#define __UDP_SERVER_H__

// 初始化并返回非阻塞的 UDP Socket
int init_udp_server(void);

// 被 poll 主循环调用的读取事件回调函数
void handle_udp_read(int sock);

#endif // __UDP_SERVER_H__
