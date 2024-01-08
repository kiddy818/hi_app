#include <app_std.h>
#include <dev_chn.h>
#include <json/json.h>
#include <fstream>
#include <rtsp/server.h>
#include <rtsp/stream/stream_manager.h>
#include <rtmp/session_manager.h>

LOG_HANDLE g_app_log;
LOG_HANDLE g_rtsp_log;
LOG_HANDLE g_rtmp_log;
LOG_HANDLE g_dev_log;
std::shared_ptr<hisilicon::dev::chn> g_chn;

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

        sprintf(g_venc_info[i].name,"H264");
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

std::thread g_thread_1s;
static void thread_1s()
{
    time_t cur_tm = time(NULL);
    time_t last_tm = cur_tm;
    while(g_chn->is_start())
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
    }
}

static void on_quit(int signal)
{
    fprintf(stderr,"on quit\n");

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

    g_thread_1s.join();
    hisilicon::dev::chn::release();
    printf("quit ok\n");
    exit(0);
}

int main(int argc,char* argv[])
{
    signal(SIGINT,on_quit);
    signal(SIGQUIT,on_quit);
    signal(SIGTERM,on_quit);

    int chn = 0;

    g_app_log = ceanic_start_log("ceanic_app",CEANIC_LOG_MODE_CONSOLE,CEANIC_LOG_DEBUG,NULL,0);
    APP_WRITE_LOG_INFO("==========================star========================");

    g_rtsp_log = ceanic_start_log("ceanic_rtsp",CEANIC_LOG_MODE_CONSOLE,CEANIC_LOG_INFO,NULL,0);
    g_rtmp_log = ceanic_start_log("ceanic_rtmp",CEANIC_LOG_MODE_CONSOLE,CEANIC_LOG_INFO,NULL,0);
    g_dev_log = ceanic_start_log("ceanic_dev",CEANIC_LOG_MODE_CONSOLE,CEANIC_LOG_INFO,NULL,0);

    ceanic::rtsp::rtsp_server rs(554);
    if(!rs.run())
    {
        APP_WRITE_LOG_ERROR("Start rtsp server failed!!!");
        return -1;
    }

    //rtmp
    std::string rtmp_url = "rtmp://192.168.0.184/live/" + std::to_string(chn + 1);
    ceanic::rtmp::session_manager::instance()->create_session(chn,0,rtmp_url.c_str(),60);

    //chn init
    hisilicon::dev::chn::init();

    //vi
    get_vi_info();
    for(auto i = 0; i < MAX_CHANNEL; i++)
    {
        printf("sensor%d:\n",i + 1);
        printf("\tname:%s\n",g_vi_info[i].name);
        printf("\tw:%d\n",g_vi_info[i].w);
        printf("\th:%d\n",g_vi_info[i].h);
        printf("\tfr:%d\n",g_vi_info[i].fr);
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
    get_aiisp_info();
    printf("aiisp info\n");
    printf("\tenable:%d\n",g_aiisp_info.enable);
    printf("\tmode:%d\n",g_aiisp_info.mode);
    printf("\tmodel_file:%s\n",g_aiisp_info.model_file);
    if(g_aiisp_info.enable)
    {
        g_chn->aiisp_start(g_aiisp_info.model_file,g_aiisp_info.mode);
    }

    g_thread_1s = std::thread(thread_1s);
    while(1)
    {
        sleep(1);
    }
    return 0;
}

