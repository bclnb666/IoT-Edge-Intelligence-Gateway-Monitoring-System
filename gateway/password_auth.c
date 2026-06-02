#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include "password_auth.h"

// 测试密码硬编码
#define TEST_PASSWORD "123456"
// 最大重试次数
#define MAX_ATTEMPTS 3

int shadow_authenticate(void) {
    struct termios oldt, newt;
    char password[128];
    int attempts = MAX_ATTEMPTS;

    printf("\n====================================\n");
    printf("     物联网网关系统 - 安全启动      \n");
    printf("====================================\n");

    while (attempts > 0) {
        printf("请输入启动密码: ");
        fflush(stdout); // 确保提示语立刻输出到屏幕

        // 获取当前终端属性
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        
        // 关闭终端的回显标志 (ECHO)，实现输入无痕的 Shadow 效果
        newt.c_lflag &= ~(ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);

        // 读取用户输入
        if (fgets(password, sizeof(password), stdin) == NULL) {
            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
            return 0;
        }

        // 无论输入什么，立即恢复终端的正常回显属性
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        printf("\n"); // 补充一个换行，因为输入时的回车也被隐藏了

        // 去除字符串末尾的回车换行符
        password[strcspn(password, "\r\n")] = 0;

        // 密码校验
        if (strcmp(password, TEST_PASSWORD) == 0) {
            printf("[安全] 认证成功！允许启动服务...\n\n");
            return 1;
        } else {
            attempts--;
            if (attempts > 0) {
                printf("[安全] 密码错误！您还有 %d 次尝试机会。\n\n", attempts);
            } else {
                printf("[安全] 连续多次密码错误，系统已被锁定退出。\n");
            }
        }
    }

    return 0;
}
