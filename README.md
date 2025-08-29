# 3516(3519)dv500_app

#### 介绍
海思3519dv500(3516dv500) demo(基于020 glibc sdk),可以在官方开发板上运行,演示如下功能:
1. H264/H265 rtsp服务 
2. H264 rtmp服务 
3. OSD(时间)功能 
4. 海思AIISP 
5. 海思图像自适应 
6. 海思编码自适应 
7. Mp4文件保存(使用海思中间件HiXuanyuanV100R001C01SPC020中的mp4库)
8. 抓拍JPG
9. Yolov5(通过rtsp视频验证)
10. vo(BT1120 1080p@60)

当前支持的sensor为: 
1. OS04A10,OS04A10_WDR,OS08A20(for 3519DV500),OS08A20_WDR(for 3519DV500)


#### 流程图
```
                   |-------->jpg保存
                   |
                   |
                   |                    |---------------->venc sub(固定720 x576)-------->rtsp stream2
                   |                    |
            (和vi相同分辨率)      (和vi相同分辨率)
mipi/vi-------->vpss grp(0)-------->vpss chn(0)---------->venc main(分辨率大小由venc.json中指定,例如1920x1080)--------->rtsp stream1
                   |                    |                     |
                   |                    |                     |-------->mp4保存
                   |                    |
                   |                    |---------------->vo
                   |
                   |               vpss缩放至(640x640)
                   |--------------->vpss chn(1)---------->yolov5-------->rtsp stream3

```


#### 编译方法
1. 按照Hi3519DV500_SDK_V2.0.2.0/smp/a55_linux/source/bsp/readme_cn.txt文档编译SDK

2. 按照如下命令，编译app
```
cd Hi3519DV500_SDK_V2.0.2.0/smp/a55_linux/source/mpp/sample
git clone https://gitee.com/shumjj/3516dv500_app.git 
cd 3516dv500_app
make
```


#### 运行
```
//step1: 复制rootfs/opt/ceanic到板子/opt/目录
cp /mnt/rootfs/opt/ceanic /opt/ -Rdp

//step2: 加载ko
cd /opt/ceanic/ko
./load3519dv500 -i

//step3: 运行app(需要确保/opt/ceanic/etc/vi.json中的sensor和板子匹配)
cd /opt/ceanic/bin
./ceanic_app
```


#### 目录结构
```
├── aiisp                   //海思AI ISP实现
├── app_std.h               //app头文件
├── device                  //海思设备相关
├── doc                     //doc
├── json                    //json库
├── log                     //log库
├── main.cpp
├── Makefile
├── README.md
├── rootfs                  //SDK rootfs修改部分
├── rtmp                    //rtmp 实现
├── rtsp                    //rtsp 实现
├── thirdlibrary            //第三方库
└── util                    //通用头文件
```


#### RTSP
##### RTSP URL
url为:   
```
//main stream
rtsp://192.168.10.98/stream1   

//sub stream
rtsp://192.168.10.98/stream2  

//yolov5 stream(需要配置文件/opt/ceanic/yolov5/yolov5.json中开启yolov5)
rtsp://192.168.10.98/stream3
```
##### VLC连接RTSP
vlc连接方法:媒体->打开网络串流->输入RTSP URL
![avatar](doc/rtsp_open.jpg)


#### RTMP
rtmp功能默认不开启,需要修改/opt/ceanic/etc/net_service.json文件 

##### RTMP测试流程
1. ubuntu下启动nginx测试服务器程序
2. 修改net_service.json中enable为1
3. 运行设备程序ceanic_app,运行成功的话,设备会connect到nginx并发布视频(发布的视频url在net_service.json中设置) 
4. pc端运行vlc,输入url,从nginx拉流，观看视频 

##### RTMP测试服务器(nginx)搭建(ubuntu20.04)
1. 按照如下命令编译nginx,需要注意的是运行nginx, -C 后面的参赛需要是全路径
```
wget http://nginx.org/download/nginx-1.21.6.tar.gz

wget https://github.com/arut/nginx-rtmp-module/archive/master.zip

tar -xf nginx-1.21.6.tar.gz

unzip master.zip

cd nginx-1.21.6/

./configure --prefix=`pwd`/mybuild --with-http_ssl_module --add-module=../nginx-rtmp-module-master

make && make install

cd mybuild/sbin

sudo ./nginx -c /home/mjj/work/nginx-1.21.6/mybuild/sbin/nginx_rtmp.conf
```
2. nginx_rtmp.conf见rtmp目录

3. 查看nginx rtmp服务是否开启
```
~/work/nginx-1.21.6/mybuild/sbin$ sudo netstat -na | grep 1935
tcp        0      0 0.0.0.0:1935            0.0.0.0:*               LISTEN
```
4. 查看rtmp 日志
错误日志如下 
```
mjj@mjj-VirtualBox:~/work/nginx-1.21.6/mybuild/sbin$ tail ../logs/error.log
024/01/09 15:59:54 [info] 16432#0: *100 disconnect, client: 192.168.10.200, server: 0.0.0.0:1935
2024/01/09 15:59:54 [info] 16432#0: *100 deleteStream, client: 192.168.10.200, server: 0.0.0.0:1935
2024/01/09 16:03:57 [info] 16432#0: *98 disconnect, client: 192.168.10.98, server: 0.0.0.0:1935
2024/01/09 16:03:57 [info] 16432#0: *98 deleteStream, client: 192.168.10.98, server: 0.0.0.0:1935
2024/01/09 16:03:57 [info] 16432#0: *97 disconnect, client: 192.168.10.98, server: 0.0.0.0:1935
2024/01/09 16:03:57 [info] 16432#0: *97 deleteStream, client: 192.168.10.98, server: 0.0.0.0:1935
```
连接日志如下 
```
tail ../logs/access.log
192.168.10.200 [09/Jan/2024:15:34:32 +0800] PLAY "live" "stream2" "" - 478 31874328 "" "LNX 9,0,124,2" (4m 8s)
192.168.10.98 [09/Jan/2024:15:49:24 +0800] PUBLISH "live" "stream2" "" - 151203194 409 "" "" (19m 27s)
192.168.10.98 [09/Jan/2024:15:49:24 +0800] PUBLISH "live" "stream1" "" - 603082173 409 "" "" (19m 27s)
192.168.10.200 [09/Jan/2024:15:53:48 +0800] PLAY "live" "stream1" "" - 1549 453211948 "" "LNX 9,0,124,2" (19m 8s)
192.168.10.98 [09/Jan/2024:15:58:50 +0800] PUBLISH "live" "stream2" "" - 4021860 409 "" "" (32s)
192.168.10.98 [09/Jan/2024:15:58:50 +0800] PUBLISH "live" "stream1" "" - 16027268 409 "" "" (32s)
192.168.10.200 [09/Jan/2024:15:59:41 +0800] PLAY "live" "stream1" "" - 411 11245160 "" "LNX 9,0,124,2" (26s)
192.168.10.200 [09/Jan/2024:15:59:54 +0800] PLAY "live" "stream2" "" - 378 1600363 "" "LNX 9,0,124,2" (13s)
192.168.10.98 [09/Jan/2024:16:03:57 +0800] PUBLISH "live" "stream2" "" - 37391403 409 "" "" (4m 50s)
192.168.10.98 [09/Jan/2024:16:03:57 +0800] PUBLISH "live" "stream1" "" - 149073600 409 "" "" (4m 50s)
```
5. nginx 开启/关闭
```
//开启命令
~/work/nginx-1.21.6/mybuild/sbin$sudo ./nginx -c /home/mjj/work/nginx-1.21.6/mybuild/sbin/nginx_rtmp.conf

//关闭命令
~/work/nginx-1.21.6/mybuild/sbin$sudo ./nginx -s stop
```

##### VLC连接nginx
1. 先根据[RTMP测试服务器搭建](#####RTMP测试服务器(nginx)搭建(ubuntu20.04))章节搭建好nginx 服务器 

2. 根据[RTMP配置文件说明](#####RTMP配置文件说明)中说明获取到RTMP URL

3. 打开vlc->媒体->打开网络串流->输入RTMP URL
![avatar](doc/rtmp_open.jpg)


