#ifndef __IO_MULTIPLEXING_H__
#define __IO_MULTIPLEXING_H__

// 启动基于 poll 的主事件循环，同时接管 UDP 和 IPC
void start_event_loop(int udp_sock, int ipc_sock);

#endif // __IO_MULTIPLEXING_H__
