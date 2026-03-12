#include "redis.hpp"
#include <iostream>
#include <cstring>
#include <chrono>
#include <vector>
using namespace std;

static const char *STREAM_KEY_PREFIX = "chat:msg:";
static const size_t STREAM_KEY_PREFIX_LEN = 8;  // strlen("chat:msg:")
static const char *ROUTE_KEY_PREFIX = "route:user:";
static const size_t ROUTE_KEY_PREFIX_LEN = 11; // strlen("route:user:")

Redis::Redis()
    : _command_context(nullptr), _read_context(nullptr)
{
}

Redis::~Redis()
{
    if (_command_context != nullptr)
        redisFree(_command_context);
    if (_read_context != nullptr)
        redisFree(_read_context);
}

bool Redis::connect()
{
    _command_context = redisConnect("127.0.0.1", 6379);
    if (nullptr == _command_context)
    {
        cerr << "connect redis (command) failed!" << endl;
        return false;
    }

    _read_context = redisConnect("127.0.0.1", 6379);
    if (nullptr == _read_context)
    {
        cerr << "connect redis (read) failed!" << endl;
        redisFree(_command_context);
        _command_context = nullptr;
        return false;
    }

    thread t([this]() { observer_stream_message(); });
    t.detach();

    cout << "connect redis-server success! (Stream mode)" << endl;
    return true;
}

bool Redis::addStreamMessage(int userid, const string &message)
{
    // XADD chat:msg:{userid} * msg <message>
    redisReply *reply = (redisReply *)redisCommand(_command_context,
        "XADD chat:msg:%d * msg %b", userid, message.data(), message.size());
    if (nullptr == reply)
    {
        cerr << "XADD command failed!" << endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}

void Redis::addStreamConsumer(int userid)
{
    lock_guard<mutex> lock(_consumer_mutex);
    _stream_consumer_ids.insert(userid);
}

void Redis::removeStreamConsumer(int userid)
{
    lock_guard<mutex> lock(_consumer_mutex);
    _stream_consumer_ids.erase(userid);
}

vector<string> Redis::getStreamOfflineMessages(int userid)
{
    vector<string> result;
    if (_command_context == nullptr)
    {
        return result;
    }

    // 读取该用户 stream 中所有历史消息
    redisReply *reply = (redisReply *)redisCommand(
        _command_context, "XRANGE chat:msg:%d - +", userid);
    if (reply == nullptr)
    {
        cerr << "XRANGE command failed!" << endl;
        return result;
    }

    if (reply->type != REDIS_REPLY_ARRAY)
    {
        freeReplyObject(reply);
        return result;
    }

    // XRANGE 返回: [[id, [field, value]], ...]
    for (size_t i = 0; i < reply->elements; i++)
    {
        redisReply *msgEntry = reply->element[i];
        if (msgEntry == nullptr || msgEntry->type != REDIS_REPLY_ARRAY || msgEntry->elements < 2)
            continue;

        redisReply *fv = msgEntry->element[1];
        if (fv == nullptr || fv->type != REDIS_REPLY_ARRAY || fv->elements < 2)
            continue;

        // 我们只存一个 field \"msg\"，取第一个 value
        redisReply *val = fv->element[1];
        if (val != nullptr && val->str != nullptr)
        {
            result.emplace_back(val->str);
        }
    }

    freeReplyObject(reply);

    // 读取完成后删除整个 stream，避免重复下发
    redisReply *delReply = (redisReply *)redisCommand(
        _command_context, "DEL chat:msg:%d", userid);
    if (delReply != nullptr)
    {
        freeReplyObject(delReply);
    }

    return result;
}

void Redis::observer_stream_message()
{
    while (true)
    {
        set<int> ids;
        {
            lock_guard<mutex> lock(_consumer_mutex);
            ids = _stream_consumer_ids;
        }

        if (ids.empty())
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        // 构造 XREAD BLOCK 5000 STREAMS key1 key2 ... $ $ ...
        vector<string> keys;
        for (int id : ids)
            keys.push_back(string(STREAM_KEY_PREFIX) + to_string(id));

        size_t n = keys.size();
        vector<const char *> argv;
        vector<size_t> argvlen;
        argv.push_back("XREAD");
        argvlen.push_back(5);
        argv.push_back("BLOCK");
        argvlen.push_back(5);
        argv.push_back("5000");
        argvlen.push_back(4);
        argv.push_back("STREAMS");
        argvlen.push_back(7);
        for (const string &k : keys)
        {
            argv.push_back(k.c_str());
            argvlen.push_back(k.size());
        }
        for (size_t i = 0; i < n; i++)
        {
            argv.push_back("$");
            argvlen.push_back(1);
        }

        int argc = (int)argv.size();
        redisReply *reply = (redisReply *)redisCommandArgv(_read_context, argc, argv.data(), argvlen.data());
        if (reply == nullptr)
        {
            if (_read_context->err == REDIS_ERR_EOF)
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        if (reply->type != REDIS_REPLY_ARRAY)
        {
            freeReplyObject(reply);
            continue;
        }

        // 回复: 每个元素为 [stream_key, [[id, [field, value]], ...]]
        for (size_t i = 0; i < reply->elements; i++)
        {
            redisReply *streamBlock = reply->element[i];
            if (streamBlock->type != REDIS_REPLY_ARRAY || streamBlock->elements < 2)
                continue;
            if (streamBlock->element[0] == nullptr || streamBlock->element[1] == nullptr)
                continue;
            const char *key = streamBlock->element[0]->str;
            if (key == nullptr || strncmp(key, STREAM_KEY_PREFIX, STREAM_KEY_PREFIX_LEN) != 0)
                continue;
            int userid = atoi(key + STREAM_KEY_PREFIX_LEN);
            redisReply *messages = streamBlock->element[1];
            if (messages->type != REDIS_REPLY_ARRAY)
                continue;
            for (size_t j = 0; j < messages->elements; j++)
            {
                redisReply *msgEntry = messages->element[j];
                if (msgEntry->type != REDIS_REPLY_ARRAY || msgEntry->elements < 2)
                    continue;
                redisReply *fv = msgEntry->element[1];
                if (fv->type != REDIS_REPLY_ARRAY || fv->elements < 2)
                    continue;
                // 我们只存了一个 field "msg"
                if (fv->element[1]->str != nullptr && _notify_message_handler)
                    _notify_message_handler(userid, string(fv->element[1]->str));
            }
        }

        freeReplyObject(reply);
    }
    cerr << ">>>>>>>>>>>>> observer_stream_message quit <<<<<<<<<<<<<" << endl;
}

void Redis::init_notify_handler(function<void(int, string)> fn)
{
    _notify_message_handler = std::move(fn);
}

bool Redis::setUserRoute(int userid, const string &serverId, int ttlSeconds)
{
    if (_command_context == nullptr)
        return false;
    if (ttlSeconds <= 0)
        ttlSeconds = 60;

    // SETEX route:user:{userid} ttl serverId
    redisReply *reply = (redisReply *)redisCommand(
        _command_context,
        "SETEX route:user:%d %d %b",
        userid,
        ttlSeconds,
        serverId.data(),
        serverId.size());

    if (reply == nullptr)
        return false;

    bool ok = (reply->type == REDIS_REPLY_STATUS) &&
              (reply->str != nullptr) &&
              (strcasecmp(reply->str, "OK") == 0);
    freeReplyObject(reply);
    return ok;
}

bool Redis::getUserRoute(int userid, string &outServerId)
{
    outServerId.clear();
    if (_command_context == nullptr)
        return false;

    redisReply *reply = (redisReply *)redisCommand(_command_context, "GET route:user:%d", userid);
    if (reply == nullptr)
        return false;

    bool ok = false;
    if (reply->type == REDIS_REPLY_STRING && reply->str != nullptr)
    {
        outServerId.assign(reply->str, reply->len);
        ok = true;
    }
    freeReplyObject(reply);
    return ok;
}

bool Redis::delUserRoute(int userid)
{
    if (_command_context == nullptr)
        return false;

    redisReply *reply = (redisReply *)redisCommand(_command_context, "DEL route:user:%d", userid);
    if (reply == nullptr)
        return false;

    bool ok = (reply->type == REDIS_REPLY_INTEGER);
    freeReplyObject(reply);
    return ok;
}
