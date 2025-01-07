# 3516(3519)dv500_app

#### 介绍
基于海思Hi3516DV500 SDK 020 glibc 版本(Hi3519DV500_SDK_V2.0.2.0)的IPC摄像头demo，支持以下功能: 
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

#### 烧录&运行
1. 烧录版本库中rootfs/rootfs_3516dv500_96M_mjj.ext4（该版本为glibc版本，需要烧录对应的uboot,kernel) 
2. 运行 
```
cd /opt/ceanic/ko
./load3519dv500 -i
cd /opt/ceanic/bin
./ceanic_app
```

#### 配置文件说明
##### vi.json
```
//sample for os04a10
{
   "sensor1" : {
      "name" : "OS04A10"
   }
}

//sample for os08a20
{
   "sensor1" : {
      "name" : "OS08A20"
   }
}

```
|  类型            | 说明                                                                                  |
|  ----            | ----                                                                                  |
| name             | sensor类型,当前支持"OS04A10","OS04A10_WDR","OS08A20","OS08A20_WDR"                    |

##### vo.json
```
{
   "vo" : {
      "enable" : 0,
      "intf_sync" : "1080P60",
      "intf_type" : "BT1120"
   }
}
```
|  类型            | 说明                                                                                  |
|  ----            | ----                                                                                  |
| enable           | 1:启用 0:不启用                                                                       |
| intf_type        | 当前支持"BT1120"                                                                      |
| intf_sync        | 当前支持"1080P60"                                                                     |

##### venc.json
```
{
   "venc1" : {
      "bitrate" : 4000,
      "fr" : 30,
      "h" : 1520,
      "name" : "H264_CBR",
      "w" : 2688
   }
}
```
|  类型            | 说明                                                                                  |
|  ----            | ----                                                                                  |
| bitrate          | 编码码率(kbps),当前支持CBR(平均码率),AVBR(最大码率)                                   |
| fr               | 编码帧率                                                                              |
| h                | 编码视频高                                                                            |
| name             | 编码类型,当前支持"H264_CBR","H264_AVBR","H265_CBR","H265_AVBR"                        |
| w                | 编码视频宽                                                                            |


##### net_service.json
```
{
   "net_service" : {
      "rtmp" : {
         "enable" : 1,
         "main_url" : "rtmp://192.168.10.97/live/stream1",
         "sub_url" : "rtmp://192.168.10.97/live/stream2"
      },
      "rtsp" : {
         "port" : 554
      }
   }
}
```
|  类型            | 说明                                                                                  |
|  ----            | ----                                                                                  |
| rtsp:port        | RTSP 侦听端口,默认554                                                                 |
| rtmp:enable      | 0:不启用rtmp 1:启用rtmp                                                               |
| rtmp:main_url    | rtmp 主编码数据url                                                                    |
| rtmp:sub_url     | rtmp 子编码数据url                                                                    |

##### aiisp.json
```
{
   "aiisp" : {
      "enable" : 1,
      "mode" : 0,
      "model_file" : "/opt/ceanic/aiisp/aibnr/model/aibnr_model_denoise_priority_lite.bin"
   }
}
```
|  类型            | 说明                                                                                  |
|  ----            | ----                                                                                  |
| enable           | 1:启用 0:不启用                                                                       |
| mode             | 0:aibnr 1:aidrc 2:ai3dnr                                                              |
| model_file       | 模型文件绝对路径，需要和mode中的类型匹配                                              |

##### scene.json
```
{
   "scene" : {
      "dir_path" : "/opt/ceanic/scene/param/sensor_os04a10",
      "enable" : 1,
      "mode" : 0
   }
}
```
|  类型            | 说明                                                                                  |
|  ----            | ----                                                                                  |
| enable           | 1:启用 0:不启用                                                                       |
| dir_path         | scene使用的配置目录路径                                                               |
| mode             | scene mode序号(见config_scenemode.ini)                                                |

##### rate_auto.json
```
{
   "rate_auto" : {
      "file" : "/opt/ceanic/etc/config_rate_auto_base_param.ini",
      "enable" : 1,
   }
}
```
|  类型            | 说明                                                                                  |
|  ----            | ----                                                                                  |
| enable           | 1:启用 0:不启用,只有AVBR编码才有效                                                    |
| file             | 编码自适应使用的文件路径                                                              |

##### mp4_save.json
```
{
   "mp4_save" : {
      "file" : "/mnt/test.mp4",
      "enable" : 0,
   }
}
```
|  类型            | 说明                                                                                  |
|  ----            | ----                                                                                  |
| enable           | 1:启用 0:不启用                                                                       |
| file             | mp4保存路径                                                                           |

##### jpg_save.json
```
{
   "jpg_save" : {
      "enable" : 0,
      "quality" : 90,
      "interval" : 60,
      "dir_path" : "/mnt/",
   }
}
```
|  类型            | 说明                                                                                  |
|  ----            | ----                                                                                  |
| enable           | 1:启用 0:不启用                                                                       |
| quality          | JPG图像质量[1,99]                                                                     |
| interval         | 抓拍间隔(秒)                                                                          |
| dir_path         | 保存目录路径                                                                          |

##### yolov5.json
```
{
   "yolov5" : {
      "enable" : 0,
      "model_file" : "/opt/ceanic/yolov5/yolov5.om"
      "cfg_file" : "/opt/ceanic/yolov5/acl.json"
   }
}

```
|  类型            | 说明                                                                                  |
|  ----            | ----                                                                                  |
| enable           | 1:启用 0:不启用                                                                       |
| model_file       | 模型文件路径                                                                          |
| cfg_file         | acl配置文件路径                                                                       |

#### YOLO配置文件acl.json相关

##### 默认(无任何功能)
acl.json例子如下:
```
{
}
```

##### 支持Profiling
1. 转化模型时atc命令需要加入-online_model_type=2,如下为atc 例子: 
```
atc --dump_data=0 --input_shape="images:1,3,640,640" --input_type="images:UINT8" --log_level=0 --online_model_type=2 --batch_num=1 --input_format=NCHW --output="./model/yolov5_with_profiling" --soc_version=Hi3519DV500 --insert_op_conf=./insert_op_yvu420sp.cfg --framework=5 --compile_mode=0 --save_original_model=true --model="./onnx_model/yolov5s.onnx" --image_list="images:./data/image_ref_list.txt"
```
实际测试下来，如果不做-online_mode_type=2这个操作，JOBXXXX/生成不了data目录

2. acl.json例子如下，具体请看"Profiling工具使用指南.pdf"
```
{
    "profiler":{
        "output":"/mnt/3519dv500/011/profiling",
        "aicpu":"on",
        "aic_metrics":"ArithmeticUtilization",
        "interval":"0",
        "acl_api":"on",
        "switch":"on"
    }
}
```

3. aiisp功能必须关闭(aiisp.json中enable=0),否则无法生成JOBXXXX目录 

4. 运行ceanic_app,如果开启了profiling,会在output(在acl.json中制定,例如例子中的/mnt/3519dv500/011/profiling)生成JOBXXX目录

5. 运行一段时间后，退出ceanic_app,将JOBXXX目录复制到atc命令所在的PC,执行如下命令显示模型性能信息

```
/usr/local/Ascend/ascend-toolkit/svp_latest/toolkit/tools/profiler/profiler_tool/analysis/msprof/msprof.py show -dir /root/share/3519dv500/profiling/JOBZPBSGWKFISTVRWHVRMPJYEUZOERMY/

//也可以用如下命令将信息保存到文件
/usr/local/Ascend/ascend-toolkit/svp_latest/toolkit/tools/profiler/profiler_tool/analysis/msprof/msprof.py show -dir /root/share/3519dv500/profiling/JOBZPBSGWKFISTVRWHVRMPJYEUZOERMY/ > test.txt
```
6. 根据"MindStudio 用户指南.pdf"和"Profiling工具使用指南.pdf"文档分析上一个步骤显示的信息(或文件),主要指标为每层耗时时间，占用百分比等等。

#### RTSP
##### RTSP URL
url为:   
```
//main stream
rtsp://192.168.10.98/stream1   

//sub stream
rtsp://192.168.10.98/stream2  

//yolov5 stream(需要配置文件中开启yolov5)
//因为性能限制，yolov5的帧率在8-12之间，如果使用vlc连接yolov5视频，需要开大vlc缓存(建议开到2000ms) 
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

#### thirdlibrary

##### freetype-2.7.1交叉编译
```
./configure --prefix=`pwd`/mybuild_aarch64_v01c01_linux_gnu --host=aarch64-v01c01-linux-gnu --with-zlib=no
make && make install
```
##### libevent-2.0.18-stable交叉编译
```
./configure --prefix=`pwd`/mybuild_aarch64_v01c01_linux_gnu --host=aarch64-v01c01-linux-gnu CFLAGS=-fPIC
make && make install
```

##### lob4cpp交叉编译
```
./configure --prefix=`pwd`/mybuild_aarch64_v01c01_linux_gnu --host=aarch64-v01c01-linux-gnu CXXFLAGS=-fPIC
make && make install
```

##### rtmpdump交叉编译
```
make prefix=./mybuild_aarch64_v01c01_linux_gnu SYS=posix CROSS_COMPILE=aarch64-v01c01-linux-gnu- XDEF=-DNO_SSL CRYPTO=
make && make install
```

##### mp4v2交叉编译 
1. 从gitee上下载版本
```
git clone https://gitee.com/mirrors/mp4v2.git
```
2. 从git版本库中checkout到最新的release版本
```
git tag
git checkout Release-ThirdParty-MP4v2-5.0.1
```

3. 更新autoaux下的config.guess,config.sub(因为316dv500使用的是aarch64_xxx的编译器，mp4v2很久没更新，无法正确识别此交叉编译链)
```
//ubuntu下安装最新的libtool
sudo apt-get install libtool

//将libtool下下的config.guess,config.sub替换调mp4v2下的同名文件
cp /usr/share/libtool/build-aux/config.guess autoaux/
cp /usr/share/libtool/build-aux/config.sub autoaux/
```

4. 编译并安装
```
./configure --host=aarch64-v01c01-linux-gnu --prefix=`pwd`/mybuild --disable-option-checking --disable-debug --disable-optimize --disable-fvisibility --disable-gch --disable-largefile --disable-util --disable-dependency-tracking --disable-libtool-lock CFLAGS=-fPIC CPPFLAGS=-fPIC
make && make install
```

##### gpac交叉编译 
zlib版本必须和gpac的版本匹配，当前实验下来,zlib-1.2.11和gpac v1.0.1是匹配的,其他版本未知

1. zlib编译
```
export CC=aarch64-v01c01-linux-gnu-gcc 
export CFLAGS="-fPIC"
./configure --prefix=`pwd`/mybuild_aarch64_v01c01_linux_gnu
make && make install
```
2. 下载gpac
```
git clone https://github.com/gpac/gpac.git
cd gpac
git tag
git checkout v1.0.1

//--extra-cflags和--extra-ldflags的路径需要和zlib的路径相匹配
./configure --prefix=`pwd`/mybuild_aarch64_v01c01_linux_gnu --cross-prefix=aarch64-v01c01-linux-gnu- --extra-cflags=-I/home/mjj/work/3519dv500/010/Hi3519DV500_SDK_V2.0.1.0/smp/a55_linux/source/mpp/sample/thirdlibrary/zlib-1.2.11/mybuild_aarch64_v01c01_linux_gnu/include --extra-cflags=-fPIC --extra-ldflags=-L/home/mjj/work/3519dv500/010/Hi3519DV500_SDK_V2.0.1.0/smp/a55_linux/source/mpp/sample/thirdlibrary/zlib-1.2.11/mybuild_aarch64_v01c01_linux_gnu/lib  --disable-x11-shm

//将zlib编译好的库复制到bin/gcc目录,否则编译时会有如下错误:/opt/linux/x86-arm/aarch64-v01c01-linux-gnu-gcc/bin/../lib/gcc/aarch64-linux-gnu/10.3.0/../../../../aarch64-linux-gnu/bin/ld: ../../bin/gcc/libgpac.so: undefined reference to `inflateReset'  

cp ../zlib-1.2.11/mybuild_aarch64_v01c01_linux_gnu/lib/libz.* bin/gcc/ -Rdp
make && make install
```

#### 调试记录
##### 段错误的处理 
在资源释放前，如果出现段错误,再重新启动程序，因为上次未正确关闭资源(例如vi,rgn资源未正确关闭)，导致本次资源初始化失败,程序会启动失败。
可以捕获SIGSEGV信号(段错误信号)，在信号处理中释放资源 
```
//sigsegv_handler需要确保不出现错误，资源能正常释放    
signal(SIGSEGV,sigsegv_handler);
```

##### 用core文件调试
1. 如果需要用gdb调试core文件，需要修改smp/a55_linux/source/cfg.mak,修改CONFIG_OT_DGB变量
```
export CONFIG_OT_GDB=y  //改为y,编译选项会增加-g 
```

2. 关闭信号处理(即不要处理SIGSEGV信号，否则不会生存core文件)

3. 调用ulimit -c 500 (500代表生成的core文件最大为500K)
```
ulimit -c 500
./ceanic_app  //段错误会生成core文件
./gdb ceanic_app core
```

##### vlc连接yolov5视频卡住
如果开启了AIISP,因为性能原因，yolov5的视频帧率在8-10帧之间(关闭AIISP,yolov5可以做到25帧/s)，vlc用默认方式连接,会发现视频卡在第一帧，需要增大vlc的缓存(建议设置为2000ms)

##### sshd环境搭建问题总结
1. "驱动和开发环境安装指南.pdf" 4.2.2 步骤5,6(修改/etc/passwd,/etc/group)不用做，只要增加sshd用户就可以(sshd的密码可以为空) 
```
adduser sshd

#按两次回车,设置空密码
Changing password for sshd
New password:
Bad password: too short
Retype password:
passwd: password for sshd
```

2. "驱动和开发环境安装指南.pdf" 4.2.2 按照步骤8，会碰到问题，当前实验下来，只能用root登录到服务器，其他用户登录到服务器(ssh mjj@192.168.10.98)会有could not chdir to home directory /home/mjj: Permission denied错误(板子端建了mjj用户，密码已经验证成功，但是ssh客户端无法chdir到/home/mjj)，当前只能允许root用户登录，sshd_config修改如下:
```
PermitRootLogin yes
```

3. demo板子root用户密码一定要有，不能为空，否则客户端通过 ssh root@192.168.10.98(192.168.10.98为板子ip)登录，会有错误，通过如下命令修改root密码
```
passwd 

#设置有效的密码
Changing password for root
New password:
Retype password:
passwd: password for root changed by root
```

4. 服务端运行例子如下:
```
export LD_LIBRARY_PATH=/mnt/3519dv500/011/openssh
/mnt/3519dv500/011/openssh/sshd -f /mnt/3519dv500/011/openssh/etc/sshd_config -h /mnt/3519dv500/011/openssh/etc/ssh/ssh_host_rsa_key
```

5. 客户端通过ssh root@192.168.10.98(192.168.10.98为板端ip),如果成功，打印如下:
```
<SVP_Docker_GPU> [root@85d1c59779f9]:~$ ssh root@192.168.10.98
root@192.168.10.98's password:
Welcome to Linux.
~ #
```


##### mindcmd一键推理(非docker环境下)
1. 板端能运行sshd(见上面sshd环境搭建问题总结),特别注意的是，sshd_conf配置文件需要修改如下，否则mindcmd 一键推理的时候会有sftp相关错误。
```
Subsystem       sftp    internal-sftp
```

2. 当前mindcmd一键推理暂时无法在docker下使用(当前docker的ip为172.x.x.x,为内部ip,板子端无复访问，无法mount),所以mindcmd当前在pc端(非docker)下可以验证，后续再考虑docker下验证。//(已经验证通过)

3. pc端需要修改/etc/hosts文件,如下把第二行注释掉，否则mindcmd 一键推理时，板子端mount得到的pc机的ip为127.0.0.1
```
127.0.0.1	localhost
#127.0.1.1	mjj-VirtualBox
```

3. 修改mindcmd 配置文件，指定板子的交叉编译环境
```
base_config.cross_compiler=gnu
```
上述表示板子端的环境为glibc,mindcmd一键推理的时候会交叉编译一个应用程序(glibc编译环境应用程序名为gnu,可以直接在板子端运行)

4. 板子端需要将libsvp_acl.so,libss_mpi_sysmem.so,libss_mpi_km.so,libprotobuf-c.so.1.0.0,四个文件复制到板子/lib64/目录下,这个比较关键，第三部的交叉编译的程序需要依赖这些库来运行。
```
cp /mnt/3519dv500/011/out/lib/libsvp_acl.so  /lib64/
cp /mnt/3519dv500/011/out/lib/libss_mpi_sysmem.so  /lib64/
cp /mnt/3519dv500/011/out/lib/libss_mpi_km.so  /lib64/
cp /mnt/3519dv500/011/out/lib/libprotobuf-c.so.1.0.0  /lib64/

//mindcmd 交叉编译出来的gnu程序链接的库为libprotobuf-c.so.1,需要软连接下
cd /lib64
ln -s libprotobuf-c.so.1.0.0 libprotobuf-c.so.1

```

5. mindcmd ssh.cfg配置文件如下:   
其中:   
BOARD_IP:板子端的ip   
BOARD_MOUNT_PATH: 板子端的目录  
HOST_MOUNT_PATH:pc端的nfs目录  
USER:板子端sshd用户名  
PASSWORD:板子端sshd密码   

```
[ssh_config]
# board ip
BOARD_IP=192.168.10.98
# board work directory, automatically mount to $HOST_MOUNT_PATH
BOARD_MOUNT_PATH=/mnt_docker
# board work directory
# to avoid bottlenecks caused by copying test resources, store test resources in this path as much as
HOST_MOUNT_PATH=/home/mjj/work/docker_shared/3519dv500/SVP_NNN_PC_V3.0.0.12//Sample/samples/2_object_detection/yolo/
# board user name
USER=root
# board user's password
PASSWORD=XXXXXX
# default port is 22
PORT=22
```

mindcmd.ini
```
include.path=/usr/local/lib/python3.8/dist-packages/mindcmd/mindcmd.ini
base_config.cann_install_path=~/Ascend/ascend-toolkit/svp_latest
base_config.target_version=Hi3519DV500
base_config.default_workspace=/home/mjj/work/docker_shared/3519dv500/SVP_NNN_PC_V3.0.0.12/Sample/samples/2_object_detection/yolo/
base_config.ssh_cfg_path=/home/mjj/work/docker_shared/3519dv500/SVP_NNN_PC_V3.0.0.12/Sample/samples/2_object_detection/yolo/ssh.conf
base_config.cross_compiler=gnu
oneclick_switch.is_clean_previous_output=1
oneclick_switch.is_amct_run=1
oneclick_switch.is_open_gt_dump=1
oneclick_switch.is_npu_run=1
oneclick_switch.is_func_run=1
oneclick_switch.is_inst_run=0
oneclick_switch.is_perf_run=0
oneclick_switch.is_dump_run=1
oneclick_switch.is_open_compare=1
oneclick_switch.is_board_profiling_run=1
oneclick_switch.is_open_profile_display=0
oneclick_switch.is_print_process_detail=0
```

6. 调用如下命令一键推理(调用前，需要确保板子端的sshd已经启动,ko已经加载)
```
mindcmd config --global oneclick_switch.is_npu_run=1
mindcmd config --global base_config.ssh_cfg_path=./ssh.conf //内容见第五步
cd /home/mjj/work/docker_shared/3519dv500/SVP_NNN_PC_V3.0.0.12//Sample/samples/2_object_detection/yolo/onnx_model
mindcmd oneclick onnx -m yolov5s.onnx -i ../data/image_ref_list.txt -r ../src/yolov5_rpn.txt
```
如果有异常，可以参考log文件(路径在output/log下)


##### mindcmd一键推理(docker环境下)
1. docker启动必须: 
   1. 加入--net=host 选项 
   2. docker和pc端的共享路径必须为一样,且为pc端的nfs目录(板子端可以mount成功)
```
sudo docker run -it -v /home/mjj/work/docker_shared:/home/mjj/work/docker_shared --net=host  --name=3519dv500_011 hispark/qiankunbp:1.0.1
```

2. 其他和非docker环境一样

##### 网络发送前的编码延时情况(OS082A20 4K@30 编码)
1. VI_OFFLIE_VPSS_OFFLINE 主码流的延时为90ms左右
2. VI_ONLINE_VPSS_OFFLINE 主码流的延时为58ms左右
3. VI_ONLINE_VPSS_ONLINE  主码流的延时为32ms左右

##### aiisp和yolov5共享AI算力,以3519DV500为例(AI算力2.5T),OS082A20(VI 4k@30):
###### yolov5资源: 
1. 不开启aiisp,只开启yolov5,一帧的svp_acl_mdl_execute()耗时在27ms
2. 开启aiisp(aibnr_model_denoise_priority.bin),再开启yolov5,一帧的svp_acl_mdl_execute()耗时在40ms
###### aiisp资源: 
1. 只开启aiisp(aibnr_model_denoise_priority.bin):  
    cat /proc/umap/aiisp中,station: 87%  
    cat /proc/umap/vi中,vi_pipe_status信息中,frame_rate为30  

2. 同时开启aiisp(aibnr_model_denoise_priority.bin)和yolov5:  
    cat /proc/umap/aiisp中,station: 99%  
    cat /proc/umap/vi中,vi_pipe_status信息中,frame_rate为18  

引用自"AIISP 开发参考.pdf"章节5 FAQ
> AIISP和用户推理任务都会占用 NPU 性能， NPU 会根据处理任务的优先级高低进行任
> 务调度和抢占，数值越小优先级越高（AIISP 的优先级默认为 1 ，推理任务默认优先级为3)

可以通过svp_acl_mdl_config_attr修改优先级
