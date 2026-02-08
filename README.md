# clusterChatServer
基于muduo库实现，可以工作在nginx tcp负责均衡环境中的集群聊天服务器和客户端源码


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

# 按照图片内容添加负载均衡配置
<img width="1165" height="718" alt="image" src="https://github.com/user-attachments/assets/51a322c6-aa85-43f1-a09c-0afffdc1108a" />

\#启动nginx

sudo /usr/local/nginx/sbin/nginx

\# 结束

sudo /usr/local/nginx/sbin/nginx -s stop

