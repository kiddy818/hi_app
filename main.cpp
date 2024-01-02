#if 0
#include <util/std.h>
#include <beacon_device.h>
#include <app_std.h>
#include <json/json.h>
#include <fstream>
#include <rtsp/server.h>
#include <rtsp/stream/stream_manager.h>
#include <rtmp/session_manager.h>
#include <aiisp_bnr.h>
#include <aiisp_drc.h>

extern "C"
{
#include <ot_scene.h>
#include <scene_loadparam.h>
}

LOG_HANDLE g_app_log;
LOG_HANDLE g_rtsp_log;
LOG_HANDLE g_rtmp_log;

#define BEACON_APP_VER "1.0"
static pthread_t g_thread_1s;
#define TMP_DIR_PATH "/tmp/beacon"
#define TMP_VERSION_FILE TMP_DIR_PATH"/version"
#define TMP_UPTIME_FILE TMP_DIR_PATH"/uptime"
#define TMP_UPTIME_FILE2 TMP_DIR_PATH"/uptime2"

#define UPTIME_TIMER_INTERVAL 5
static uint32_t g_uptime = 1;
static int g_uptime_timer_cnt=0;

typedef struct
{
    int enable;
    char dir_path[255];
    int mode;
    ot_scene_param scene_param;
    ot_scene_video_mode video_mode;
}scene_info_t;
static scene_info_t g_scene_info;

typedef struct
{
    int enable;
    int mode; //0:bnr 1:drc 2:3dnr
    char model_file[255];
    aiisp* obj;
}aiisp_info_t;
static aiisp_info_t g_aiisp_info;

int g_chn = 0;
static int get_etc_file(char* buf,int buf_len,const char* path)
{
    if(access(path,F_OK) < 0)
    {
        return -1;
    }

    char cmd[255];
    sprintf(cmd,"cat %s",path);
    FILE* f = popen(cmd,"r");
    if(f == NULL)
    {
        return -1;
    }

    char*line = NULL;
    size_t len = 0;
    if(getline(&line,&len,f) < 0
            || line == NULL
            || strlen(line) > buf_len)
    {
        if(line)
        {
            free(line);
        }
        pclose(f);
        return -1;
    }

    sprintf(buf,"%s",line);
    if(strlen(buf) > 0)
    {
        if(buf[strlen(buf) - 1] == '\r' || buf[strlen(buf) - 1] == '\n')
        {
            buf[strlen(buf) - 1] = '\0';
        }
    }

    free(line);
    pclose(f);

    return 0;
}

static int write_etc_file(const char* path,const char* str,...)
{
    char buf[512];
    va_list arg_ptr;
    va_start(arg_ptr, str);
    vsprintf(buf, str, arg_ptr);
    va_end(arg_ptr);

    strcat(buf,"\n");

    FILE* f = fopen(path,"w");
    if(f == NULL)
    {
        return -1;
    }

    fwrite(buf,1,strlen(buf) + 1,f); 

    fclose(f);
    return 0;
}

static int touch_file(const char* file)
{
    return write_etc_file(file,"");
}

static void update_dev_uptime()
{
    write_etc_file(TMP_UPTIME_FILE,"%d",g_uptime);

    int days = g_uptime / (24 * 3600);
    int hours = (g_uptime % (24 * 3600)) / 3600;
    int min = ((g_uptime % (24 * 3600)) % 3600) / 60;
    write_etc_file(TMP_UPTIME_FILE2,"%d days,%d hours,%d minutes",days,hours,min);
}

static void on_quit(int signal)
{
    fprintf(stderr,"on quit\n");

    if(g_aiisp_info.enable)
    {
        if(g_aiisp_info.obj)
        {
            g_aiisp_info.obj->stop();
            delete g_aiisp_info.obj;
        }

        aiisp_bnr::release();
        aiisp_drc::release();
    }

    if(g_scene_info.enable)
    {
        ot_scene_deinit();
    }

    beacon_encode_stop_capture();
    beacon_osd_date_set(g_chn,0,0,48,0,0,1,16,16,0,0);
    beacon_osd_name_set(g_chn,0,0,48,0,0,1,3840,2160,NULL);
    beacon_encode_stop(g_chn,0);

    beacon_vi_release(g_chn);

    beacon_device_release();

    beacon_stop_log(g_app_log);
    printf("quit ok\n");
    exit(0);
}

static void * thread_1s(void *arg)
{
    while(1)
    {
        //uptime
        g_uptime_timer_cnt++;
        if(g_uptime_timer_cnt >= UPTIME_TIMER_INTERVAL)
        {
            update_dev_uptime();
            g_uptime_timer_cnt = 0;
        }
        g_uptime++;

        //ae
        beacon_img_value_t v;
        int ret = beacon_img_get_current_value(g_chn,&v);
        if(ret == 0)
        {
            int exp_time = v.exp_time != 0 ? v.exp_time : 1;
            APP_WRITE_LOG_DEBUG("shutter=1/%ds(%dus),again=%d,dgain=%d,ispgain=%d,iso=%d,is_max_exposure=%d,is_exposure_stable=%d",
                    1000000 / exp_time,exp_time,
                    v.again,
                    v.dgain,
                    v.ispgain,
                    v.iso,
                    v.is_max_exposure,
                    v.is_exposure_stable);
        }

        sleep(1);
    }

    return NULL;
}

void update_dev_version()
{
    write_etc_file(TMP_VERSION_FILE,"app(ver)=%s,dev(ver)=%s",BEACON_APP_VER,beacon_device_version());
}

void touch_flag_file()
{
    char flag_file[255];
    sprintf(flag_file,"%s/start",TMP_DIR_PATH);
    touch_file(flag_file);
}

static void init_tmp_files()
{
    touch_flag_file();

    update_dev_version();
}

static void on_frame(int type,const char* buf,int len)
{
}

static void on_nalu(int chn,int stream,int type,uint64_t time_stamp,const char* buf,int len,int64_t userdata)
{
    static char* g_frame_buf = NULL;
    static unsigned int g_frame_len = 0;
    if(g_frame_buf == NULL)
    {
        g_frame_buf = new char[1024 * 1024];
        g_frame_len = 0;
    }

    memcpy(g_frame_buf + g_frame_len, buf, len);
    g_frame_len += len;

    int nalu_type = buf[4] & 0x1f;
    switch(nalu_type)
    {
        case 0x1:
        case 0x5:
            {
                on_frame(nalu_type,g_frame_buf, g_frame_len);
                g_frame_len = 0;
                break;
            }
    }
}

static void on_stream(int chn,int stream,int type,uint64_t time_stamp,const char* buf,int len,int64_t userdata)
{
    if(type == 2)
    {
        //audio
        return;
    }

    //printf("chn=%d,stream=%d,type=%d,len=%d,%02x,%02x,%02x,%02x,%02x\n",chn,stream,type,len,buf[0],buf[1],buf[2],buf[3],buf[4]);

    beacon::util::stream_head sh;
    sh.len = len + sizeof(sh);
    sh.type = STREAM_NALU_SLICE;    
    sh.time_stamp = time_stamp * 1000;
    sh.tag = BEACON_TAG;
    sh.sys_time = time(NULL);  
    sh.nalu_count = 1;         
    sh.nalu[0].data = (char*)buf + 4;
    sh.nalu[0].size = len - 4; 
    sh.nalu[0].timestamp = time_stamp * 90;

    beacon::rtsp::stream_manager::instance()->process_data(chn,stream,&sh,buf,len);
    beacon::rtmp::session_manager::instance()->process_data(chn,stream,&sh,(uint8_t*)buf,len);

    on_nalu(chn,stream,type,time_stamp,buf,len,userdata);
}


#define SCENE_FILE_PATH "/opt/beacon/scene/scene.json"
static void init_scene_info()
{
    Json::Value root;

    root["scene"]["enable"] = 1;
    root["scene"]["mode"] = 0;
    root["scene"]["dir_path"] = "/opt/beacon/scene/param/sensor_os04a10";
    std::string str= root.toStyledString();
    std::ofstream ofs;
    ofs.open(SCENE_FILE_PATH);
    ofs << str;
    ofs.close();
}

static int get_scene_info()
{
    try
    {
        if(access(SCENE_FILE_PATH,F_OK) < 0)
        {
            init_scene_info();

            if(access(SCENE_FILE_PATH,F_OK) < 0)
            {
                return -1;
            }
        }

        std::ifstream ifs;
        ifs.open(SCENE_FILE_PATH);
        if(!ifs.is_open())
        {
            return -1;
        }
        Json::Reader reader;  
        Json::Value root; 
        if (!reader.parse(ifs, root, false)) 
        {
            return -1;
        }

        Json::Value node; 
        g_scene_info.enable = root["scene"]["enable"].asInt();
        g_scene_info.mode = root["scene"]["mode"].asInt();
        sprintf(g_scene_info.dir_path,"%s",root["scene"]["dir_path"].asCString());

        ifs.close();

        return 0;
    }
    catch(...)
    {
        return -1;
    }
}

#define AIISP_FILE_PATH "/opt/beacon/aiisp/aiisp.json"
static void init_aiisp_info()
{
    Json::Value root;

    root["aiisp"]["enable"] = 1;
    root["aiisp"]["mode"] = 0;
    root["aiisp"]["model_file"] = "/opt/beacon/aiisp/aibnr/model/aibnr_model_denoise_priority_lite.bin";
    std::string str= root.toStyledString();
    std::ofstream ofs;
    ofs.open(AIISP_FILE_PATH);
    ofs << str;
    ofs.close();
}

static int get_aiisp_info()
{
    try
    {
        if(access(AIISP_FILE_PATH,F_OK) < 0)
        {
            init_aiisp_info();

            if(access(AIISP_FILE_PATH,F_OK) < 0)
            {
                return -1;
            }
        }

        std::ifstream ifs;
        ifs.open(AIISP_FILE_PATH);
        if(!ifs.is_open())
        {
            return -1;
        }
        Json::Reader reader;  
        Json::Value root; 
        if (!reader.parse(ifs, root, false)) 
        {
            return -1;
        }

        Json::Value node; 
        g_aiisp_info.enable = root["aiisp"]["enable"].asInt();
        g_aiisp_info.mode = root["aiisp"]["mode"].asInt();
        sprintf(g_aiisp_info.model_file,"%s",root["aiisp"]["model_file"].asCString());

        ifs.close();

        return 0;
    }
    catch(...)
    {
        return -1;
    }
}

int main(int argc,char* argv[])
{
    signal(SIGINT,on_quit);
    signal(SIGQUIT,on_quit);
    signal(SIGTERM,on_quit);

    get_vi_info();
    for(auto i = 0; i < MAX_CHANNEL; i++)
    {
        printf("sensor%d:\n",i + 1);
        printf("\tname:%s\n",g_vi_info[i].name);
        printf("\tw:%d\n",g_vi_info[i].w);
        printf("\th:%d\n",g_vi_info[i].h);
        printf("\tflip:%d\n",g_vi_info[i].flip);
    }

    int chn = 0;
    if(argc > 1)
    {
        chn = atoi(argv[1]);
    }
    g_chn = chn;
    printf("g_chn=%d\n",g_chn);

    if(access(TMP_DIR_PATH,F_OK) != 0)
    {
        mkdir(TMP_DIR_PATH,0777);
    }

    g_app_log = beacon_start_log("beacon_app",BEACON_LOG_MODE_CONSOLE,BEACON_LOG_DEBUG,NULL,0);
    APP_WRITE_LOG_INFO("==========================star========================");

    g_rtsp_log = beacon_start_log("beacon_rtsp",BEACON_LOG_MODE_CONSOLE,BEACON_LOG_INFO,NULL,0);
    g_rtmp_log = beacon_start_log("beacon_rtmp",BEACON_LOG_MODE_CONSOLE,BEACON_LOG_INFO,NULL,0);

    init_tmp_files();

    APP_WRITE_LOG_INFO("device ver=%s",beacon_device_version());

    beacon_device_set_log(1,BEACON_LOG_DEBUG,"/tmp/log.txt",100 *1024);
   
    //device init
    int ret = 0;
    ret = beacon_device_init(BEACON_DEV_MODE_0);
    if(ret != BEACON_ERR_SUCCESS)
    {
        return -1;
    }

    //vi init
    ret = beacon_vi_init(chn,&g_vi_info[g_chn]);
    if(ret != BEACON_ERR_SUCCESS)
    {
        return -1;
    }

    //ae 
    ret = beacon_img_enable_ae(chn,1,5,10000);
    if(ret != BEACON_ERR_SUCCESS)
    {
        return -1;
    }

    //encode start
    beacon_encode_register_stream_callback(CALLBACK_FUN_NALU,on_stream,0);
    beacon_h264_config_t main_enc;
    memset(&main_enc,0,sizeof(main_enc));
    main_enc.profile = 0;
    main_enc.gop_interval = 30;
    main_enc.bitrate_control = 0;
    main_enc.bitrate_avg = 4000;
    ret = beacon_encode_video_set_arg(chn,0,BEACON_RESOLUTION_1920_1080,30,BEACON_VIDEO_ENCODE_H264,&main_enc);
    if(ret != BEACON_ERR_SUCCESS)
    {
        return -1;
    }
    ret = beacon_encode_start(chn,0);
    if(ret != BEACON_ERR_SUCCESS)
    {
        return -1;
    }
    ret = beacon_encode_start_capture();
    if(ret != BEACON_ERR_SUCCESS)
    {
        return -1;
    }

    //osd
    ret = beacon_osd_date_set(chn,0,1,48,0,0,1,16,16,0,0);
    if(ret != BEACON_ERR_SUCCESS)
    {
        return -1;
    }

    char name[32];
    sprintf(name,"通道%d",chn + 1);
    ret = beacon_osd_name_set(chn,0,1,48,0,0,1,1920,1080,name);
    if(ret != BEACON_ERR_SUCCESS)
    {
        return -1;
    }

    pthread_create(&g_thread_1s,NULL,thread_1s,NULL);

    beacon::rtsp::rtsp_server rs(554);
    if(!rs.run())
    {
        APP_WRITE_LOG_ERROR("Start rtsp server failed!!!");
        return -1;
    }

    //rtmp
    std::string rtmp_url = "rtmp://192.168.0.184/live/" + std::to_string(chn + 1);
    beacon::rtmp::session_manager::instance()->create_session(chn,0,rtmp_url.c_str(),60);
    
    //aiisp
    get_aiisp_info();
    printf("aiisp info\n");
    printf("\tenable:%d\n",g_aiisp_info.enable);
    printf("\tmode:%d\n",g_aiisp_info.mode);
    printf("\tmodel_file:%s\n",g_aiisp_info.model_file);
    int is_wdr_mode = 0;
    if(strcmp(g_vi_info[chn].name,"OS04A10_WDR") == 0)
    {
        is_wdr_mode = 1;
    }
    if(g_aiisp_info.enable)
    {
        switch(g_aiisp_info.mode)
        {
            case 0://aibnr
                {
                    aiisp_bnr::init(g_aiisp_info.model_file,g_vi_info[g_chn].w,g_vi_info[g_chn].h,is_wdr_mode);
                    g_aiisp_info.obj = new aiisp_bnr(g_chn);
                    g_aiisp_info.obj->start();
                    break;
                }
            case 1://aidrc
                {
                    aiisp_drc::init(g_aiisp_info.model_file,g_vi_info[g_chn].w,g_vi_info[g_chn].h,is_wdr_mode);
                    g_aiisp_info.obj = new aiisp_drc(g_chn);
                    g_aiisp_info.obj->start();
                    break;
                }
        }
    }

    //scene
    get_scene_info();
    printf("scene info\n");
    printf("\tenable:%d\n",g_scene_info.enable);
    printf("\tmode:%d\n",g_scene_info.mode);
    printf("\tdir_path:%s\n",g_scene_info.dir_path);
    if(g_scene_info.enable)
    {
        ret = ot_scene_create_param(g_scene_info.dir_path,&g_scene_info.scene_param,&g_scene_info.video_mode);
        if(ret != TD_SUCCESS)
        {
            APP_WRITE_LOG_ERROR("ot_scene_create_param!!!");
            return -1;
        }

        ret = ot_scene_init(&g_scene_info.scene_param);
        if(ret != TD_SUCCESS)
        {
            APP_WRITE_LOG_ERROR("ot_scene_init!!!");
            return -1;
        }

        int mode = g_scene_info.mode;
        ret = ot_scene_set_scene_mode(&(g_scene_info.video_mode.video_mode[mode]));
        if(ret != TD_SUCCESS)
        {
            APP_WRITE_LOG_ERROR("ot_scene_init!!!");
            return -1;
        }
    }

    while(1)
    {
        sleep(1);
    }
    return 0;
}

#endif

#include <app_std.h>
#include <dev_chn.h>
#include <json/json.h>
#include <fstream>

LOG_HANDLE g_app_log;
LOG_HANDLE g_rtsp_log;
LOG_HANDLE g_rtmp_log;
LOG_HANDLE g_dev_log;
hisilicon::dev::chn* g_chn = NULL;

#define MAX_CHANNEL 1
#define VI_FILE_PATH "/opt/beacon/etc/vi.json"
typedef struct
{
    char name[32];
    int w;
    int h;
    int fr;
    int flip;
}vi_t;
static vi_t g_vi_info[MAX_CHANNEL];
static void init_vi_info()
{
    Json::Value root;
    for(auto i = 0; i < MAX_CHANNEL; i++)
    {
        std::string sns = "sensor" + std::to_string(i + 1);

        sprintf(g_vi_info[i].name,"OS04A10");
        g_vi_info[i].w = 2688;
        g_vi_info[i].h = 1520;
        g_vi_info[i].fr = 30;
        g_vi_info[i].flip = 0;

        root[sns]["name"] = g_vi_info[i].name;
        root[sns]["w"] = g_vi_info[i].w;
        root[sns]["h"] = g_vi_info[i].h;
        root[sns]["fr"] = g_vi_info[i].fr;
        root[sns]["flip"] = g_vi_info[i].flip;
    }

    std::string str= root.toStyledString();
    std::ofstream ofs;
    ofs.open(VI_FILE_PATH);
    ofs << str;
    ofs.close();
}

static int get_vi_info()
{
    try
    {
        if(access(VI_FILE_PATH,F_OK) < 0)
        {
            init_vi_info();

            if(access(VI_FILE_PATH,F_OK) < 0)
            {
                return -1;
            }
        }

        std::ifstream ifs;
        ifs.open(VI_FILE_PATH);
        if(!ifs.is_open())
        {
            return -1;
        }
        Json::Reader reader;  
        Json::Value root; 
        if (!reader.parse(ifs, root, false)) 
        {
            return -1;
        }

        Json::Value node; 
        for(auto i = 0; i < MAX_CHANNEL; i++)
        {
            std::string sns = "sensor" + std::to_string(i + 1);
            node = root[sns];

            sprintf(g_vi_info[i].name,"%s",node["name"].asCString());
            g_vi_info[i].w = node["w"].asInt();
            g_vi_info[i].h = node["h"].asInt();
            g_vi_info[i].fr = node["fr"].asInt();
            g_vi_info[i].flip = node["flip"].asInt();
        }

        ifs.close();

        return 0;
    }
    catch(...)
    {
        return -1;
    }
}


static void on_quit(int signal)
{
    fprintf(stderr,"on quit\n");

    hisilicon::dev::chn::start_capture(false);
    if(g_chn)
    {
        g_chn->stop();
        delete g_chn;
        g_chn = NULL;
    }

    hisilicon::dev::chn::sys_release();
    printf("quit ok\n");
    exit(0);
}

int main(int argc,char* argv[])
{
    signal(SIGINT,on_quit);
    signal(SIGQUIT,on_quit);
    signal(SIGTERM,on_quit);

    g_app_log = beacon_start_log("beacon_app",BEACON_LOG_MODE_CONSOLE,BEACON_LOG_DEBUG,NULL,0);
    APP_WRITE_LOG_INFO("==========================star========================");

    g_rtsp_log = beacon_start_log("beacon_rtsp",BEACON_LOG_MODE_CONSOLE,BEACON_LOG_INFO,NULL,0);
    g_rtmp_log = beacon_start_log("beacon_rtmp",BEACON_LOG_MODE_CONSOLE,BEACON_LOG_INFO,NULL,0);
    g_dev_log = beacon_start_log("beacon_dev",BEACON_LOG_MODE_CONSOLE,BEACON_LOG_INFO,NULL,0);

    hisilicon::dev::chn::sys_init();

    get_vi_info();
    for(auto i = 0; i < MAX_CHANNEL; i++)
    {
        printf("sensor%d:\n",i + 1);
        printf("\tname:%s\n",g_vi_info[i].name);
        printf("\tw:%d\n",g_vi_info[i].w);
        printf("\th:%d\n",g_vi_info[i].h);
        printf("\tfr:%d\n",g_vi_info[i].fr);
    }

    int chn = 0;
    g_chn = new hisilicon::dev::chn(g_vi_info[chn].name);
    g_chn->start(1920,1080,g_vi_info[chn].fr,4000);

    hisilicon::dev::chn::start_capture(true);
    while(1)
    {
        sleep(1);
    }
    return 0;
}

