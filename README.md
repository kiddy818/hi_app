# 3516dv500_app

#### 介绍
基于海思Hi3516DV500 SDK 010 版本(Hi3519DV500_SDK_V2.0.1.0)的IPC摄像头demo，支持以下功能: 
1. H264/H265 rtsp服务 
2. H264 rtmp服务 
3. OSD(时间)功能 
4. AIISP 
5. 图像自适应 

当前支持的sensor为: 
1. OS04A10 


#### 编译方法
```
cd Hi3519DV500_SDK_V2.0.1.0/smp/a55_linux/source/mpp/sample
git clone https://gitee.com/shumjj/3516dv500_app.git 
cd 3516dv500_app
make
```

#### 运行
1. 需要将版本库中rootfs目录下的opt/ceanic目录复制到板子,假设nfs已经挂载,nfs目录为/mnt 
```
cp /mnt/opt/ceanic /opt/ -Rdp
```
2. 直接运行编译的程序 
```
./ceanic_app
```
