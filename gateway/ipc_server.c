#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "gateway.h"
#include "logger.h"
#include "ipc_server.h"

#define IPC_SOCKET_PATH "/tmp/gateway_ipc.sock"

int init_ipc_server(void) {
    int server_fd;
    struct sockaddr_un addr;

    // 1. 创建本地 Unix Socket
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) {
        LOG_ERR("[IPC-服务] Socket 创建失败");
        return -1;
    }

    // 2. [核心重构] 将其设置为非阻塞模式
    int flags = fcntl(server_fd, F_GETFL, 0);
    fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);

    unlink(IPC_SOCKET_PATH);
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, IPC_SOCKET_PATH, sizeof(addr.sun_path) - 1);

    // 3. 绑定并监听
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        LOG_ERR("[IPC-服务] 绑定失败");
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, 5) < 0) {
        LOG_ERR("[IPC-服务] 监听失败");
        close(server_fd);
        return -1;
    }

    LOG_INFO("[IPC-服务] 非阻塞服务初始化成功! 监听: %s (fd=%d)", IPC_SOCKET_PATH, server_fd);
    return server_fd; // 将描述符交还给主循环接管
}
