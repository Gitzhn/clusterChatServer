#include "json.hpp"
using json = nlohmann::json; // 按照键值对的形式存储

#include <iostream>
#include <vector>
#include <map>
#include <string>
using namespace std;

void func1(){
    json js;
    js["msg_type"] = 2;
    js["from"] = "zhangsan";
    js["to"] = "lisi";
    js["msg"] = "hello,i'm zhangsan.who are you? and what are you doing?\n";
    js["error"] = "you forget to give me money";
    string sendBuf = js.dump(); // 将json对象转换成符合格式的字符串
    cout<<sendBuf.c_str()<<endl;
}

void func2(){
    json js;
    js["id"] = {1,2,3,4,5};
    js["name"] = "zhangsan";
    // js["msg"] = {{"zhangsanfeng","hello,i'm zhangsanfeng"}, {"wanger","hi,i'm wanger"}};
    cout<<js<<endl;
}

string sendMsg(string from, string to, string msg){
    json js;
    js["id"] = {1,2,3,4,5};
    js["msg_type"] = 2;
    js["from"] = from;
    js["to"] = to;
    js["msg"] = msg;
    string sendBuf = js.dump(); // 将json对象转换成符合格式的字符串
    return sendBuf;
}

json recvMsg(string recvBuf){
    json js = json::parse(recvBuf); // 将符合格式的字符串转换成json对象
    return js;
}

int main(){
    string sendBuf = sendMsg("zhangsan", "lisi", "hello,i'm zhangsan.who are you? and what are you doing?\n");
    json rcvBuf = recvMsg(sendBuf);
    cout<<rcvBuf.dump()<<endl;
    cout<<rcvBuf["id"]<<endl;
    return 0;
}