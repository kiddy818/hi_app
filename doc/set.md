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


