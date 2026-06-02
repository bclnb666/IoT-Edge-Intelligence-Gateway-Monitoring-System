#ifndef __TOKEN_BUCKET_H__
#define __TOKEN_BUCKET_H__

/******************************
* 名    称 : token_bucket_init()
* 功    能 : 初始化全局令牌桶
* 入口参数 : capacity - 桶的最大容量（决定允许的最大瞬间突发流量）
* rate     - 令牌生成的速率（个/秒，决定平均处理速度）
******************************/
void token_bucket_init(int capacity, int rate);

/******************************
* 名    称 : token_bucket_consume()
* 功    能 : 尝试消耗指定数量的令牌
* 入口参数 : count - 需要消耗的令牌数 (通常为 1)
* 返回参数 : 1 代表获取成功放行，0 代表令牌不足被限流
******************************/
int token_bucket_consume(int count);

#endif // __TOKEN_BUCKET_H__
