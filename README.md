# smallhttpd
这是一个C语言写的http服务器，同时支持linux和windows
## linux下编译运行(注意关闭防火墙)
gcc httpd.c -o httpd -lpthread <br />
./httpd
## windows下编译运行（注意关闭防火墙）
gcc httpd.c -o httpd -lws2_32 <br />
httpd
## 查看结果
浏览器访问网址：http://127.0.0.1:9090 即可看到主页
