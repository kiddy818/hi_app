#ifndef beacon_device_include_h
#define beacon_device_include_h

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif

#define MAX_CHANNEL (1)
#define MAX_VENC_COUNT (2)

#define BEACON_ERR_SUCCESS						(0)
#define BEACON_ERR_NOT_SUPPORT					(-1)
#define BEACON_ERR_BUSY							(-2)
#define BEACON_ERR_INVALID_PARAM				(-3)
#define BEACON_ERR_ORDER						(-4)
#define BEACON_ERR_INTERNAL						(-5)
#define BEACON_ERR_NOMEM						(-6)

/*
 * @description: get version of this lib
 * @return: version string(eg:"1.0")
 */
const char* beacon_device_version();

/*
 * @description: enable/disable log 
 * @return: if success,BEACON_ERR_SUCCESS will be returned,otherwise,see errcodes.
 */
int beacon_device_set_log(int enable,int level,const char* file_path,unsigned int max_file_size);

enum
{
    BEACON_DEV_MODE_0 = 0,  /*only support one stream at same time*/
    BEACON_DEV_MODE_1,      /*support two stream at same time*/
};
/*
 * @description: init function
 *     must be called at the begining,and only allowed to be called by one time
 * @return: if success,BEACON_ERR_SUCCESS will be returned,otherwise,see errcodes.
 */
int beacon_device_init(int mode);

/*
 * @description: release function
 *     must be called at the end,and only allowed to be called by one time
 * @return: if success,BEACON_ERR_SUCCESS will be returned,otherwise,see errcodes.
 */
int beacon_device_release();

int beacon_vi_power_enable(int chn,int enable);
int beacon_vi_mipi_enable(int chn,int enable,int sensor_reset_flag);

typedef struct
{
	int fr;
    int w;
    int h;
    char name[32];

    int flip;
	int reserve[2];
}beacon_vin_t;
/*
 * @description: vi init function
 *     muste be called after beacon_device_init()
 * @param chn: vi channel index
 *     [0,MAX_CHANNEL) is valid
 * @param arg: vi informations
 *     fr: vi frame rate(eg,25,30),must be valid and supported by the sensor
 *     w: vi width(eg,1920,3814),must be valid and supported by the sensor
 *     h: vi height(eg,1080,2160),must be valid and supported by the sensor
 *     name: sensor name(eg,"imx290","imx334")
 * @return: if success,BEACON_ERR_SUCCESS will be returned,otherwise,see errcodes.
 */
int beacon_vi_init(int chn,beacon_vin_t* arg);

/*
 * @description: vi release function
 *     must be called after beacon_vi_init()
 * @param chn: vi channel index
 *     [0,MAX_CHANNEL) is valid
 * @return: if success,BEACON_ERR_SUCCESS will be returned,otherwise,see errcodes.
 */
int beacon_vi_release(int chn);

typedef struct
{
    int enable;
    int x;
    int y;
    int font_size;
    int reserve[16];
}vi_snap_date_t;
typedef struct
{
    int enable;
    int x;
    int y;
    int font_size;
    char name[128];
    int reserve[16];
}vi_snap_name_t;
/*
 * @description: capture the jpeg datas into the buf
 * @param chn: vi channel index
 *     [0,MAX_CHANNEL) is valid
 * @param w: width of the jpeg
 *     it may not equal to the vi width,value that less than the vi width is also valid.for example,if the vi width is set with 3840 by beacon_vi_init() function,this value can be 3840 or 1920(which is less than the vi width).
 * @param h: height of jpeg
 *     it may not equal to the vi height,value that less than the vi height is also valid.for example,if the vi height is set with 2160 by beacon_vi_init() function,this value can be 2160 or 1080(which is less than the vi height).
 * @param qualith: the quality of jpeg picture
 *     [1,99] is valid,99 means the highest quality
 * @jpg_buf: buf where the jpg saved
 *     jpg_buf are alloced by the caller,the caller make sure it has enough space to save the datas.
 * @buf_len: length of the buf
 * @jpg_len[out]: real length of the jpg datas
 * @osd_date: osd date informations
 *     enable: 1=enable osd date,0=disable osd date
 *     x: left coordinate of osd date,only valid if enable==1
 *     y: top coordinage of osd date,only valid if enable==1
 *     font_size: font size of osd date, only valid if enable==1
 *     reserve[16]:reserved,must be zero
 * @osd_name: osd name informations
 *     enable: 1=enable osd name,0=disable osd name
 *     x: left coordinage of osd name,only valid if enable==1
 *     y: top coordinage of osd date,only valid if enable==1
 *     font_size: font size of osd date, only valid if enable==1
 *     name:osd name(UTF-8),only valid if enable==1
 * @return: if success,BEACON_ERR_SUCCESS will be returned,otherwise,see errcodes.
 */
int beacon_vi_snap(int chn,int w,int h,int quality,char* jpg_buf,int buf_len,int* jpg_len,vi_snap_date_t* osd_date,vi_snap_name_t* osd_name);

typedef struct
{
    int w;
    int h;
    int quality;
    char* jpg_buf;
    int buf_len;
    int jpg_len;
}vi_sub_snap_t;
int beacon_vi_snap_ex(int chn,int w,int h,int quality,char* jpg_buf,int buf_len,int* jpg_len,vi_snap_date_t* osd_date,vi_snap_name_t* osd_name,vi_sub_snap_t* psub_jpg);

/*
 * @description: register callback functions for encode streams
 * @param fun: callback functions
 * @param userdata: callback user datas
 * @return: if success,BEACON_ERR_SUCCESS will be returned,otherwise,see errcodes.
 */
enum
{
    CALLBACK_FUN_NALU = 0,
    CALLBACK_FUN_FRAME = 1,
};
int beacon_encode_register_stream_callback(int fun_type,void (*fun)(int chn,int stream,int type,uint64_t time_stamp,const char* buf,int len,int64_t userdata),int64_t userdata);

typedef enum
{
	BEACON_VIDEO_ENCODE_H264 = 0,
	BEACON_VIDEO_ENCODE_MJPEG,
    BEACON_VIDEO_ENCODE_H265,
	BEACON_VIDEO_ENCODE_LAST,
}BEACON_VIDEO_ENCODE_E;
typedef enum
{
	BEACON_RESOLUTION_176_144 = 0,/*QCIF*/
	BEACON_RESOLUTION_352_288,/*CIF*/
	BEACON_RESOLUTION_640_480,/*VGA*/
	BEACON_RESOLUTION_704_576,/**/
	BEACON_RESOLUTION_720_576,/*D1*/
	Bk_RESOLUTION_1024_768,
	BEACON_RESOLUTION_1280_720,/*720P*/
	BEACON_RESOLUTION_1280_960,
	BEACON_RESOLUTION_1920_1080,/*1080P*/
	BEACON_RESOLUTION_2048_1536,/*3M 4:3*/
	BEACON_RESOLUTION_2304_1296,/*3M 16:9*/
	BEACON_RESOLUTION_2592_1944,/*5M 4:3*/
	BEACON_RESOLUTION_2976_1674,/*5M 16:9*/
	BEACON_RESOLUTION_3840_2160,
	BEACON_RESOLUTION_LAST,
}BEACON_VIDEO_RESOLUTION_E;
typedef struct
{
	int profile;/*h264 0:baseline 1:main  h265 0:main 1:main10*/
	int gop_interval;/*gop*/
	int bitrate_control;/*0:CBR 1:VBR 2:AVBR*/
	int bitrate_avg;/*CBR,Kbps*/
	int bitrate_min;/*VBR,Kbps*/
	int bitrate_max;/*VBR,Kbps*/
	int reserve[3];/*must be zero*/
}beacon_h264_config_t;
typedef beacon_h264_config_t beacon_h265_config_t;
typedef struct
{
	int quality;/*0-100*/
	int reserve[3];/*0*/
}beacon_mjpeg_config_t;
/*
 * @description: set encode params
 * @param chn: channel index[0,MAX_CHANNEL]
 * @param stream: stream index[0,MAX_VENC_COUNT]
 *     one channel can be encoded with different resolution,for example,stream 0 can be encoded with high resolution(eg,4K or 1080p),stream 1 can be encoded with low resollution(eg,d1,cif),we usually use stream 0 to save(record),while stream1 always be used to net transfer.
 * @param reso: resolution of the stream,see BEACON_VIDEO_RESOLUTION_E.
 * @param fr: frame rate of the stream,[5,max supported by sensor)
 * @param type: encode type,see BEACON_VIDEO_ENCODE_E
 *     by now,only h264 or h265 are supported.
 * @param arg: other params depends on type
 *     if type==BEACON_VIDEO_ENCODE_H264,arg is the beacon_h264_config_t structure,if type==BEACON_VIDEO_ENCODE_H265,arg is beacon_h265_config_t.
 * @return: if success,BEACON_ERR_SUCCESS will be returned,otherwise,see errcodes.
 */
int beacon_encode_video_set_arg(int chn,int stream,int reso,int fr,int type,void* arg);

typedef enum
{
	BEACON_AUDIO_ENCODE_AAC = 0,	
	BEACON_AUDIO_ENCODE_G711A,
	BEACON_AUDIO_ENCODE_G711U ,	

	BEACON_AUDIO_ENCODE_LAST,
}BEACON_AUDIO_ENCODE_E;
typedef struct
{
	int sample;/*8000,16000...*/
	int bitrate;/*8,16...*/
	int chn;/*1,2...*/	
}beacon_audio_config_t;
int beacon_encode_audio_set_arg(int chn,int enable,int type,beacon_audio_config_t* arg);

/*
 * @description: start encode
 *     datas would not come until beacon_encode_start_capture() called.
 * @param chn: channel index[0,MAX_CHANNEL)
 * @param stream: stream index[0,MAX_VENC_COUNT)
 * @return: if success,BEACON_ERR_SUCCESS will be returned,otherwise,see errcodes.
 */
int beacon_encode_start(int chn,int stream);

/*
 * @description: stop encode
 *     beacon_encode_stop_capture() are needed to be called before this function called
 * @param chn: channel index[0,MAX_CHANNEL)
 * @param stream: stream index[0,MAX_VENC_COUNT)
 * @return: if success,BEACON_ERR_SUCCESS will be returned,otherwise,see errcodes.
 */
int beacon_encode_stop(int chn,int stream);

/**
 * @description: start to capture encode streams
 * @return: if success,BEACON_ERR_SUCCESS will be returned,otherwise,see errcodes.
 */
int beacon_encode_start_capture();

/*
 * @description: stop to capture encode streams
 * @return: if success,BEACON_ERR_SUCCESS will be returned,otherwise,see errcodes.
 */
int beacon_encode_stop_capture();

/*
 * @description: request one i frame 
 * @return: if success,BEACON_ERR_SUCCESS will be returned,otherwise,see errcodes.
 */
int beacon_encode_request_i_frame(int chn,int stream);

typedef enum
{
    BEACON_COLOR_DEFAULT = 0,
    BEACON_COLOR_RED ,
    BEACON_COLOR_BLACK ,
    BEACON_COLOR_BLUE ,
    BEACON_COLOR_GREEN ,
    BEACON_COLOR_YELLOW ,
    BEACON_COLOR_CYAN ,
    BEACON_COLOR_LAST,
}BEACON_OSD_COLOR_E;

typedef enum
{
	BEACON_DATE_FORMAT_1 = 0,/*YYYY-MM-DD HH:MM:SS*/
	BEACON_DATE_FORMAT_2,/*MM-DD-YYYY HH:MM:SS*/
	BEACON_DATE_FORMAT_LAST,
}BEACON_DATE_FORMAT_E;
/*
 * @description: add or clear date/name string to the encode stream
 * @param chn: channel index[0,MAX_CHANNEL)
 * @param stream: stream index[0,MAX_VENC_COUNT
 * @return: if success,BEACON_ERR_SUCCESS will be returned,otherwise,see errcodes.
 */
int beacon_osd_date_set(int chn,int stream,int enable,int font_size,int color,int alpha,int outline,int x,int y,int type,int show_week);
int beacon_osd_name_set(int chn,int stream,int enable,int font_size,int color,int alpha,int outline,int x,int y,const char* str_name);

typedef struct
{
    unsigned int exp_time;
    int again;
    int dgain;
    int ispgain;
    unsigned int iso;
    int is_max_exposure;
    int is_exposure_stable;
    int reserve[32];
}beacon_img_value_t;
int beacon_img_get_current_value(int chn,beacon_img_value_t* val);
int beacon_img_enable_ae(int chn,int enable,int shutter,int agc);

int beacon_scene_start();
int beacon_scene_stop();

int beacon_device_set_mode(int mode);

typedef struct
{
    int enable;

    int baspect;//[0,1] keep ratio
    int x_ratio;//[0,100],valid when baspect==1
    int y_ratio;//[0,100],valid when baspect==1
    int xy_ratio;//[0,100],valid when baspect==0

    int x_center_offset;//[-511,511]
    int y_center_offset;//[-511,511]

    int distortion_ratio;//[-300,500]
}beacon_ldc_attr_t;
int beacon_vi_set_ldc_attr(int chn,beacon_ldc_attr_t* ldc_attr);

int beacon_img_set_color(int chn,int bright,int contrast,int saturation,int hue);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
