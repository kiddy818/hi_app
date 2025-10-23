#### 调试记录

##### 单独编译uboot
uboot目录下运行
```
//emmc举例
1.  cp configs/hi3519dv500_emmc_defconfig .config

//修改menuconfig
2.  make ARCH=arm CROSS_COMPILE=aarch64-v01c01-linux-gnu- menuconfig

//将config保存为hi3519dv500_emmc_auto_update_defconfig(举例)
3.  cp .config configs/hi3519dv500_emmc_auto_update_defconfig 
```
bsp目录下运行
```
make BOOT_MEDIA=emmc LIB_TYPE=glibc CHIP=hi3519dv500 UBOOT_CONFIG=hi3519dv500_emmc_auto_update_defconfig gslboot_build
```
bsp/pub/hi3519dv500_emmc_image_glibc/boot_image.bin为最新的uboot

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
1.  不开启aiisp,只开启yolov5,一帧的svp_acl_mdl_execute()耗时在27ms
2.  开启aiisp(aibnr_model_denoise_priority.bin),再开启yolov5,一帧的svp_acl_mdl_execute()耗时在40ms
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

##### ddr测试
如果ddr型号未在兼容列表里，需要测试稳定性，测试方法整理如下:
1.  首先需要确保能烧录uboot

2.  参考"\ReleaseDoc\zh\02.only for reference\Hardware\Hi3519DV500 DDR DQ窗口查看方法及结果分析.pdf" 文档,运行ddr training,确保ddr窗口符合要求
    ```
    //uboot下运行

    //参考文档，打开ddr寄存器控制,如下为3519dv500下的例子
    mw 0x110200a0 0x2

    //结果需要符合文档中的窗口要求
    ddr dataeye
    ```
3.  uboot下使用mtest命令测试，mtest默认未开启，开启方法:

    1.  make ARCH=arm CROSS_COMPILE=aarch64-v01c01-linux-gnu- menuconfig

    2.  Command line interface--->Memory commands--->[*] memtest
    ```
    //uboot下使用mtest命令,如下命令运行10分钟以上时间，确保未有报错信息
    mtest 0x42000000 0x440000
    ```

4.  如果能进入kernle,可以使用开源工具memtester测试稳定性
    1.  在https://pyropus.ca./software/memtester/下载最新版本，当前最新版本为memtester-4.7.1.tar.gz

    2.  解压后修改conf-cc和conf-ld两个文件，将两个文件中的cc改为3519dv500使用的交叉编译器
    ```
    //conf-cc第一行修改为
    aarch64-v01c01-linux-gnu-gcc -O2 -DPOSIX -D_POSIX_C_SOURCE=200809L -D_FILE_OFFSET_BITS=64 -DTEST_NARROW_WRITES -c

    //conf-ld第一行修改为
    aarch64-v01c01-linux-gnu-gcc -s
    ```
    3.  make生成memtester文件，把该文件复制到板子,测试命令如下:
    ```
    //memtester <memory> [runs]
    //以下测试为申请100M,循环测试10次
    ./memtester 100M 10
    ```

5.  如果条件允许，除了常温条件，还需要在低温和高温环境下测试。

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


