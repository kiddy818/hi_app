#include <app_std.h>
#include <dev_chn.h>
#include <json/json.h>
#include <fstream>
#include <rtsp/server.h>
#include <rtsp/stream/stream_manager.h>
#include <rtmp/session_manager.h>
#include <execinfo.h>

LOG_HANDLE g_app_log;
LOG_HANDLE g_rtsp_log;
LOG_HANDLE g_rtmp_log;
LOG_HANDLE g_dev_log;
std::shared_ptr<hisilicon::dev::chn> g_chn;

#define NET_SERVICE_FILE_PATH "/opt/ceanic/etc/net_service.json"
typedef struct
{
    int rtsp_port;
    int rtmp_enable;
    char rtmp_main_url[255];
    char rtmp_sub_url[255];
}net_service_t;
static net_service_t g_net_service_info;
static void init_net_service_info()
{
    Json::Value root;

    root["net_service"]["rtsp"]["port"] = 554;
    root["net_service"]["rtmp"]["enable"] = 0;
    root["net_service"]["rtmp"]["main_url"] = "rtmp://192.168.10.97/live/stream1" ;
    root["net_service"]["rtmp"]["sub_url"] = "rtmp://192.168.10.97/live/stream2" ;
    std::string str= root.toStyledString();
    std::ofstream ofs;
    ofs.open(NET_SERVICE_FILE_PATH);
    ofs << str;
    ofs.close();
}

static int get_net_service_info()
{
    try
    {
        if(access(NET_SERVICE_FILE_PATH,F_OK) < 0)
        {
            init_net_service_info();

            if(access(NET_SERVICE_FILE_PATH,F_OK) < 0)
            {
                return -1;
            }
        }

        std::ifstream ifs;
        ifs.open(NET_SERVICE_FILE_PATH);
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
        g_net_service_info.rtsp_port= root["net_service"]["rtsp"]["port"].asInt();
        g_net_service_info.rtmp_enable = root["net_service"]["rtmp"]["enable"].asInt();
        sprintf(g_net_service_info.rtmp_main_url,"%s",root["net_service"]["rtmp"]["main_url"].asCString());
        sprintf(g_net_service_info.rtmp_sub_url,"%s",root["net_service"]["rtmp"]["sub_url"].asCString());

        ifs.close();

        return 0;
    }
    catch(...)
    {
        return -1;
    }
}


#define VENC_FILE_PATH "/opt/ceanic/etc/venc.json"
typedef struct
{
    char name[32];
    int w;
    int h;
    int fr;
    int bitrate;
}venc_t;
static venc_t g_venc_info[MAX_CHANNEL];
static void init_venc_info()
{
    Json::Value root;
    for(auto i = 0; i < MAX_CHANNEL; i++)
    {
        std::string venc = "venc" + std::to_string(i + 1);

        sprintf(g_venc_info[i].name,"H264_CBR");
        g_venc_info[i].w = 2688;
        g_venc_info[i].h = 1520;
        g_venc_info[i].fr = 30;
        g_venc_info[i].bitrate = 4000;

        root[venc]["name"] = g_venc_info[i].name;
        root[venc]["w"] = g_venc_info[i].w;
        root[venc]["h"] = g_venc_info[i].h;
        root[venc]["fr"] = g_venc_info[i].fr;
        root[venc]["bitrate"] = g_venc_info[i].bitrate;
    }

    std::string str= root.toStyledString();
    std::ofstream ofs;
    ofs.open(VENC_FILE_PATH);
    ofs << str;
    ofs.close();
}

static int get_venc_info()
{
    try
    {
        if(access(VENC_FILE_PATH,F_OK) < 0)
        {
            init_venc_info();

            if(access(VENC_FILE_PATH,F_OK) < 0)
            {
                return -1;
            }
        }

        std::ifstream ifs;
        ifs.open(VENC_FILE_PATH);
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
            std::string venc= "venc" + std::to_string(i + 1);
            node = root[venc];

            sprintf(g_venc_info[i].name,"%s",node["name"].asCString());
            g_venc_info[i].w = node["w"].asInt();
            g_venc_info[i].h = node["h"].asInt();
            g_venc_info[i].fr = node["fr"].asInt();
            g_venc_info[i].bitrate = node["bitrate"].asInt();
        }

        ifs.close();

        return 0;
    }
    catch(...)
    {
        return -1;
    }
}

#define VI_FILE_PATH "/opt/ceanic/etc/vi.json"
typedef struct
{
    char name[32];
}vi_t;
static vi_t g_vi_info[MAX_CHANNEL];
static void init_vi_info()
{
    Json::Value root;
    for(auto i = 0; i < MAX_CHANNEL; i++)
    {
        std::string sns = "sensor" + std::to_string(i + 1);

        sprintf(g_vi_info[i].name,"OS04A10");

        root[sns]["name"] = g_vi_info[i].name;
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
        }

        ifs.close();

        return 0;
    }
    catch(...)
    {
        return -1;
    }
}

typedef struct
{
    int enable;
    char dir_path[255];
    int mode;
}scene_info_t;
static scene_info_t g_scene_info;
#define SCENE_FILE_PATH "/opt/ceanic/scene/scene.json"
static void init_scene_info()
{
    Json::Value root;

    root["scene"]["enable"] = 1;
    root["scene"]["mode"] = 0;
    root["scene"]["dir_path"] = "/opt/ceanic/scene/param/sensor_os04a10";
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

typedef struct
{
    int enable;
    int mode; //0:bnr 1:drc 2:3dnr
    char model_file[255];
}aiisp_info_t;
static aiisp_info_t g_aiisp_info;
#define AIISP_FILE_PATH "/opt/ceanic/aiisp/aiisp.json"
static void init_aiisp_info()
{
    Json::Value root;

    root["aiisp"]["enable"] = 1;
    root["aiisp"]["mode"] = 0;
    root["aiisp"]["model_file"] = "/opt/ceanic/aiisp/aibnr/model/aibnr_model_denoise_priority_lite.bin";
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

typedef struct
{
    int enable;
    char file[255];
}rate_auto_info_t;
static rate_auto_info_t g_rate_auto_info;
#define RATE_AUTO_FILE_PATH "/opt/ceanic/etc/rate_auto.json"
static void init_rate_auto_info()
{
    Json::Value root;

    root["rate_auto"]["enable"] = 1;
    root["rate_auto"]["file"] = "/opt/ceanic/etc/config_rate_auto_base_param.ini";
    std::string str= root.toStyledString();
    std::ofstream ofs;
    ofs.open(RATE_AUTO_FILE_PATH);
    ofs << str;
    ofs.close();
}

static int get_rate_auto_info()
{
    try
    {
        if(access(RATE_AUTO_FILE_PATH,F_OK) < 0)
        {
            init_rate_auto_info();

            if(access(RATE_AUTO_FILE_PATH,F_OK) < 0)
            {
                return -1;
            }
        }

        std::ifstream ifs;
        ifs.open(RATE_AUTO_FILE_PATH);
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
        g_rate_auto_info.enable = root["rate_auto"]["enable"].asInt();
        sprintf(g_rate_auto_info.file,"%s",root["rate_auto"]["file"].asCString());

        ifs.close();

        return 0;
    }
    catch(...)
    {
        return -1;
    }
}

typedef struct
{
    int enable;
    char file[255];
}mp4_save_info_t;
static mp4_save_info_t g_mp4_save_info;
#define MP4_SAVE_INFO_PATH "/opt/ceanic/etc/mp4_save_info.json"
static void init_mp4_save_info()
{
    Json::Value root;

    root["mp4_save"]["enable"] = 0;
    root["mp4_save"]["file"] = "/mnt/test.mp4";
    std::string str= root.toStyledString();
    std::ofstream ofs;
    ofs.open(MP4_SAVE_INFO_PATH);
    ofs << str;
    ofs.close();
}

static int get_mp4_save_info()
{
    try
    {
        if(access(MP4_SAVE_INFO_PATH,F_OK) < 0)
        {
            init_mp4_save_info();

            if(access(MP4_SAVE_INFO_PATH,F_OK) < 0)
            {
                return -1;
            }
        }

        std::ifstream ifs;
        ifs.open(MP4_SAVE_INFO_PATH);
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
        g_mp4_save_info.enable = root["mp4_save"]["enable"].asInt();
        sprintf(g_mp4_save_info.file,"%s",root["mp4_save"]["file"].asCString());

        ifs.close();

        return 0;
    }
    catch(...)
    {
        return -1;
    }
}

typedef struct
{
    int enable;
    int quality;
    int interval;
    char dir_path[255];
}jpg_save_info_t;
static jpg_save_info_t g_jpg_save_info;
#define JPG_SAVE_INFO_PATH "/opt/ceanic/etc/jpg_save_info.json"
static void init_jpg_save_info()
{
    Json::Value root;

    root["jpg_save"]["enable"] = 0;
    root["jpg_save"]["quality"] = 90;
    root["jpg_save"]["interval"] = 60;
    root["jpg_save"]["dir_path"] = "/mnt/";
    std::string str= root.toStyledString();
    std::ofstream ofs;
    ofs.open(JPG_SAVE_INFO_PATH);
    ofs << str;
    ofs.close();
}

static int get_jpg_save_info()
{
    try
    {
        if(access(JPG_SAVE_INFO_PATH,F_OK) < 0)
        {
            init_jpg_save_info();

            if(access(JPG_SAVE_INFO_PATH,F_OK) < 0)
            {
                return -1;
            }
        }

        std::ifstream ifs;
        ifs.open(JPG_SAVE_INFO_PATH);
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
        g_jpg_save_info.enable = root["jpg_save"]["enable"].asInt();
        g_jpg_save_info.quality = root["jpg_save"]["quality"].asInt();
        g_jpg_save_info.interval = root["jpg_save"]["interval"].asInt();
        sprintf(g_jpg_save_info.dir_path,"%s",root["jpg_save"]["dir_path"].asCString());

        ifs.close();

        return 0;
    }
    catch(...)
    {
        return -1;
    }
}

typedef struct
{
    int enable;
    char model_file[255];
    char cfg_file[255];
}yolov5_info_t;
static yolov5_info_t g_yolov5_info;
#define YOLOV5_INFO_PATH "/opt/ceanic/yolov5/yolov5.json"
static void init_yolov5_info()
{
    Json::Value root;

    root["yolov5"]["enable"] = 0;
    root["yolov5"]["model_file"] = "/opt/ceanic/yolov5/yolov5.om";
    root["yolov5"]["cfg_file"] = "/opt/ceanic/yolov5/acl.json";
    std::string str= root.toStyledString();
    std::ofstream ofs;
    ofs.open(YOLOV5_INFO_PATH);
    ofs << str;
    ofs.close();
}

static int get_yolov5_info()
{
    try
    {
        if(access(YOLOV5_INFO_PATH,F_OK) < 0)
        {
            init_yolov5_info();

            if(access(YOLOV5_INFO_PATH,F_OK) < 0)
            {
                return -1;
            }
        }

        std::ifstream ifs;
        ifs.open(YOLOV5_INFO_PATH);
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
        g_yolov5_info.enable = root["yolov5"]["enable"].asInt();
        sprintf(g_yolov5_info.model_file,"%s",root["yolov5"]["model_file"].asCString());
        sprintf(g_yolov5_info.cfg_file,"%s",root["yolov5"]["cfg_file"].asCString());

        ifs.close();

        return 0;
    }
    catch(...)
    {
        return -1;
    }
}

typedef struct
{
    int enable;
    char intf_type[32];
    char intf_sync[32];
}vo_info_t;
static vo_info_t g_vo_info;
#define VO_INFO_PATH "/opt/ceanic/etc/vo.json"
static void init_vo_info()
{
    Json::Value root;

    root["vo"]["enable"] = 0;
    root["vo"]["intf_type"] = "BT1120";
    root["vo"]["intf_sync"] = "1080P60";
    std::string str= root.toStyledString();
    std::ofstream ofs;
    ofs.open(VO_INFO_PATH);
    ofs << str;
    ofs.close();
}

static int get_vo_info()
{
    try
    {
        if(access(VO_INFO_PATH,F_OK) < 0)
        {
            init_vo_info();

            if(access(VO_INFO_PATH,F_OK) < 0)
            {
                return -1;
            }
        }

        std::ifstream ifs;
        ifs.open(VO_INFO_PATH);
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
        g_vo_info.enable = root["vo"]["enable"].asInt();
        sprintf(g_vo_info.intf_type,"%s",root["vo"]["intf_type"].asCString());
        sprintf(g_vo_info.intf_sync,"%s",root["vo"]["intf_sync"].asCString());

        ifs.close();

        return 0;
    }
    catch(...)
    {
        return -1;
    }
}


static std::thread g_thread_1s;
static bool g_thread_run = false;
static void thread_1s()
{
    time_t cur_tm = time(NULL);
    time_t last_tm = cur_tm;

    while(g_thread_run 
            && g_chn && g_chn->is_start())
    {
        cur_tm = time(NULL);
        if(last_tm == cur_tm)
        {
            usleep(10000);
            continue;
        }

        last_tm = cur_tm;
        isp_exposure_t v;
        if(g_chn->get_isp_exposure_info(&v))
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

        if(g_jpg_save_info.enable 
                && cur_tm % g_jpg_save_info.interval == 0)
        {
            char snap_file[255];
            sprintf(snap_file,"%s/snap%d.jpg",g_jpg_save_info.dir_path,cur_tm);
            g_chn->trigger_jpg(snap_file,g_jpg_save_info.quality,"抓拍测试");
        }
    }

    APP_WRITE_LOG_DEBUG("thread_1s exit");
}

static void do_exit()
{
    try
    {
        g_thread_run = false;
        g_thread_1s.join();

        int chn = 0;

        if(g_vo_info.enable)
        {
            g_chn->vo_stop();
        }

        if(g_yolov5_info.enable)
        {
            g_chn->yolov5_stop();
            hisilicon::dev::svp::release();
        }

        if(g_mp4_save_info.enable)
        {
            g_chn->stop_save();
        }

        if(g_rate_auto_info.enable
                && strstr(g_venc_info[chn].name,"AVBR") != NULL)
        {
            hisilicon::dev::chn::rate_auto_release();
        }

        if(g_aiisp_info.enable)
        {
            g_chn->aiisp_stop();
        }

        if(g_scene_info.enable)
        {
            hisilicon::dev::chn::scene_release();
        }

        hisilicon::dev::chn::start_capture(false);
        if(g_chn)
        {
            g_chn->stop();
        }

        hisilicon::dev::chn::release();
    }catch(...)
    {
        printf("do exit unexcepted error\n");
    }
}

#define BACKTRACE_SIZE 16
static void show_stack(void)
{
    int i;
    void *buffer[BACKTRACE_SIZE];

    int n = backtrace(buffer, BACKTRACE_SIZE);
    char **symbols = backtrace_symbols(buffer, n);
    if(NULL == symbols){
        perror("backtrace symbols");
        return;
    }
    for (i = 0; i < n; i++)
    {
        printf("%d: %s\n", i, symbols[i]);
	}

	free(symbols);
}

void sigsegv_handler(int signo)
{
    printf("Receive SIGSEGV signal\n");
    show_stack();
    do_exit();
    exit(-1);
}
static void on_quit(int signal)
{
    fprintf(stderr,"on quit\n");
    do_exit();
    printf("quit ok\n");
    exit(0);
}

int main(int argc,char* argv[])
{
    signal(SIGINT,on_quit);
    signal(SIGQUIT,on_quit);
    signal(SIGTERM,on_quit);
    signal(SIGSEGV,sigsegv_handler);

    int chn = 0;

    g_app_log = ceanic_start_log("ceanic_app",CEANIC_LOG_MODE_CONSOLE,CEANIC_LOG_DEBUG,NULL,0);
    APP_WRITE_LOG_INFO("==========================star========================");

    g_rtsp_log = ceanic_start_log("ceanic_rtsp",CEANIC_LOG_MODE_CONSOLE,CEANIC_LOG_INFO,NULL,0);
    g_rtmp_log = ceanic_start_log("ceanic_rtmp",CEANIC_LOG_MODE_CONSOLE,CEANIC_LOG_INFO,NULL,0);
    g_dev_log = ceanic_start_log("ceanic_dev",CEANIC_LOG_MODE_CONSOLE,CEANIC_LOG_INFO,NULL,0);

    get_net_service_info();
    printf("net service info\n");
    printf("\trtsp port:%d\n",g_net_service_info.rtsp_port);
    printf("\trtmp enable:%d\n",g_net_service_info.rtmp_enable);
    printf("\trtmp main url:%s\n",g_net_service_info.rtmp_main_url);
    printf("\trtmp sub url:%s\n",g_net_service_info.rtmp_sub_url);
    ceanic::rtsp::rtsp_server rs(g_net_service_info.rtsp_port);
    if(!rs.run())
    {
        APP_WRITE_LOG_ERROR("Start rtsp server failed!!!");
        return -1;
    }

    //rtmp
    if(g_net_service_info.rtmp_enable)
    {
        ceanic::rtmp::session_manager::instance()->create_session(chn,0,g_net_service_info.rtmp_main_url);
        ceanic::rtmp::session_manager::instance()->create_session(chn,1,g_net_service_info.rtmp_sub_url);
    }

    //jpg save
    get_jpg_save_info();
    printf("jpg save info\n");
    printf("\tenable:%d\n",g_jpg_save_info.enable);
    printf("\tquality:%d\n",g_jpg_save_info.quality);
    printf("\tinterval:%d\n",g_jpg_save_info.interval);
    printf("\tdir_path:%s\n",g_jpg_save_info.dir_path);

    //aiisp
    get_aiisp_info();
    printf("aiisp info\n");
    printf("\tenable:%d\n",g_aiisp_info.enable);
    printf("\tmode:%d\n",g_aiisp_info.mode);
    printf("\tmodel_file:%s\n",g_aiisp_info.model_file);

    //chn init
    ot_vi_vpss_mode_type mode = OT_VI_ONLINE_VPSS_ONLINE;
    if(g_aiisp_info.enable)
    {
        mode = OT_VI_OFFLINE_VPSS_OFFLINE;
    }
    else if(g_jpg_save_info.enable)
    {
        mode = OT_VI_ONLINE_VPSS_OFFLINE;
    }

    hisilicon::dev::chn::init(mode);

    //vi
    get_vi_info();
    for(auto i = 0; i < MAX_CHANNEL; i++)
    {
        printf("sensor%d:\n",i + 1);
        printf("\tname:%s\n",g_vi_info[i].name);
    }

    //venc
    get_venc_info();
    for(auto i = 0; i < MAX_CHANNEL; i++)
    {
        printf("venc%d:\n",i + 1);
        printf("\tname:%s\n",g_venc_info[i].name);
        printf("\tw:%d\n",g_venc_info[i].w);
        printf("\th:%d\n",g_venc_info[i].h);
        printf("\tfr:%d\n",g_venc_info[i].fr);
        printf("\tbitrate:%d\n",g_venc_info[i].bitrate);
    }

    g_chn = std::make_shared<hisilicon::dev::chn>(g_vi_info[chn].name,g_venc_info[chn].name,chn);
    g_chn->start(g_venc_info[chn].w,g_venc_info[chn].h,g_venc_info[chn].fr,g_venc_info[chn].bitrate);
    hisilicon::dev::chn::start_capture(true);

    //scene
    get_scene_info();
    printf("scene info\n");
    printf("\tenable:%d\n",g_scene_info.enable);
    printf("\tmode:%d\n",g_scene_info.mode);
    printf("\tdir_path:%s\n",g_scene_info.dir_path);
    if(g_scene_info.enable)
    {
        hisilicon::dev::chn::scene_init(g_scene_info.dir_path);
        hisilicon::dev::chn::scene_set_mode(g_scene_info.mode);
    }

    //aiisp
    if(g_aiisp_info.enable)
    {
        g_chn->aiisp_start(g_aiisp_info.model_file,g_aiisp_info.mode);
    }

    //rate auto
    get_rate_auto_info();
    printf("rate auto info\n");
    printf("\tenable:%d\n",g_rate_auto_info.enable);
    printf("\tfile:%s\n",g_rate_auto_info.file);
    if(g_rate_auto_info.enable
            && strstr(g_venc_info[chn].name,"AVBR") != NULL)
    {
        //only avbr support rate auto
        hisilicon::dev::chn::rate_auto_init(g_rate_auto_info.file);
    }

    //mp4 save
    get_mp4_save_info();
    printf("mp4 save info\n");
    printf("\tenable:%d\n",g_mp4_save_info.enable);
    printf("\tfile:%s\n",g_mp4_save_info.file);
    if(g_mp4_save_info.enable)
    {
        g_chn->start_save(g_mp4_save_info.file);
    }

    //yolov5
    get_yolov5_info();
    printf("yolov5 info\n");
    printf("\tenable:%d\n",g_yolov5_info.enable);
    printf("\tmodel_file:%s\n",g_yolov5_info.model_file);
    printf("\tcfg_file:%s\n",g_yolov5_info.cfg_file);
    if(g_yolov5_info.enable)
    {
        hisilicon::dev::svp::init(g_yolov5_info.cfg_file);
        g_chn->yolov5_start(g_yolov5_info.model_file);
    }

    //vo
    get_vo_info();
    printf("vo info\n");
    printf("\tenable:%d\n",g_vo_info.enable);
    printf("\tintf_type:%s\n",g_vo_info.intf_type);
    printf("\tintf_sync:%s\n",g_vo_info.intf_sync);
    if(g_vo_info.enable)
    {
        g_chn->vo_start(g_vo_info.intf_type,g_vo_info.intf_sync);
    }

    g_thread_run = true;
    g_thread_1s = std::thread(thread_1s);

    while(1)
    {
        sleep(1);
    }
    return 0;
}

