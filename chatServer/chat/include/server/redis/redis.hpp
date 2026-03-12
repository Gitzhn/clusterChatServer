#ifndef REDIS_H
#define REDIS_H

#include <hiredis/hiredis.h>
#include <thread>
#include <functional>
#include <set>
#include <mutex>
#include <vector>
#include <string>
using namespace std;

/*
 * 使用 Redis Stream 做跨服消息队列（替代原 Pub/Sub）：
 * - 每个用户一个 stream，key 为 chat:msg:{userid}
 * - 发往某用户：XADD chat:msg:{userid} * msg <payload>
 * - 本机只消费“当前连接在本机”的用户的 stream（XREAD BLOCK）
 * 无需改 MySQL 表结构；Redis 侧无预建表，stream 首次 XADD 时自动创建。
 */
class Redis
{
public:
    Redis();
    ~Redis();

    // 连接 redis 服务器（建立写连接 + 读连接，并启动消费线程）
    bool connect();

    // 向对应用户的 stream 追加一条消息（跨服发往 userid）
    bool addStreamMessage(int userid, const string &message);

    // 本机开始消费该用户 stream（用户登录时调用）
    void addStreamConsumer(int userid);

    // 本机不再消费该用户 stream（用户下线时调用）
    void removeStreamConsumer(int userid);

    // 读取并清理指定用户 stream 中历史未消费的消息（作为离线补偿）
    vector<string> getStreamOfflineMessages(int userid);

    // 在独立线程中阻塞读取本机负责的 stream，收到后回调业务层
    void observer_stream_message();

    // 初始化“收到 stream 消息”时上报给业务层的回调
    void init_notify_handler(function<void(int, string)> fn);

    // ---------------- 路由表：userid -> serverId(ip:port) ----------------
    // 写入路由（带 TTL，避免宕机脏数据长期存在）
    bool setUserRoute(int userid, const string &serverId, int ttlSeconds);
    // 读取路由；存在返回 true，并把结果写到 outServerId
    bool getUserRoute(int userid, string &outServerId);
    // 删除路由
    bool delUserRoute(int userid);

private:
    // 写命令用（XADD、XTRIM 等）
    redisContext *_command_context;
    // 读命令用（XREAD BLOCK），与写分离避免阻塞
    redisContext *_read_context;

    // 本机要消费的 userid 集合（对应 stream chat:msg:{userid}）
    set<int> _stream_consumer_ids;
    mutex _consumer_mutex;

    // 收到消息时上报给 ChatService 的回调
    function<void(int, string)> _notify_message_handler;
};

#endif
