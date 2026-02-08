# clusterChatServer
基于muduo库实现，可以工作在nginx tcp负责均衡环境中的集群聊天服务器和客户端源码

# 环境准备
## nginx安装
wget http://nginx.org/download/nginx-1.12.2.tar.gz

tar -axvf nginx-1.12.2.tar.gz

cd nginx-1.12.2

sudo apt update

sudo apt install -y libpcre3 libpcre3-dev zlib1g zlib1g-dev libssl-dev

./configure --with-stream \#激活tcp负载均衡模块

make && sudo make install

cd /usr/local/nginx/conf

sudo vim nginx.conf

#### 按照图片内容添加负载均衡配置
<img width="1165" height="718" alt="image" src="https://github.com/user-attachments/assets/51a322c6-aa85-43f1-a09c-0afffdc1108a" />

启动nginx

sudo /usr/local/nginx/sbin/nginx

结束

sudo /usr/local/nginx/sbin/nginx -s stop

## 安装hiredis，用于发布-订阅的客户端编程

git clone https://github.com/redis/hiredis

cd hiredis

make

sudo make install

sudo ldconfig /usr/local/lib

## 安装mysql，注意修改db.cpp中的数据库信息

mysql包含下图5张表(自己建一下)

<img width="319" height="285" alt="image" src="https://github.com/user-attachments/assets/cdbfd838-59a5-41e4-9349-8da0d7fea3a8" />

AllGroup: 存储群聊信息（群号、群名、群描述）

<img width="1044" height="276" alt="image" src="https://github.com/user-attachments/assets/1ba61aea-1773-4833-83fd-6a185dbfd387" />

Friend: 好友信息

<img width="762" height="243" alt="image" src="https://github.com/user-attachments/assets/35e71b58-fed3-4db6-8663-5f85845c76dc" />

GroupUser: 群成员信息（群号、用户id、群身份）

<img width="1075" height="272" alt="image" src="https://github.com/user-attachments/assets/245f238f-d147-4749-a89f-eb43ccd32894" />

OfflineMessage: 存储离线消息

<img width="868" height="263" alt="image" src="https://github.com/user-attachments/assets/5690e822-fd3d-425a-95a0-beaf9fcb6c68" />

User: 存储用户信息

<img width="1202" height="312" alt="image" src="https://github.com/user-attachments/assets/a2260aa2-abd9-400c-9de1-78cb84145235" />

## 剩下一些编译运行C++代码的配置自己配置
