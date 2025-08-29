#include "mp4_save.h"

namespace ceanic{namespace stream_save{

    mp4_save::mp4_save(ceanic::util::media_head mh,const char* file_path)
        :m_bopen(false),m_mh(mh),m_file_path(file_path),m_mp4_fh(NULL),m_mp4_video_track_h(NULL),m_mp4_audio_track_h(NULL),m_sbuf(512 * 1024)
    {
        memset(&m_mp4_cfg,0,sizeof(m_mp4_cfg));

        sprintf(m_mp4_cfg.aszFileName,"%s",file_path);

        m_mp4_cfg.enConfigType =  OT_MP4_CONFIG_MUXER;

       /* 1024 * 1024 : set the vbuf size for fwrite
        * set (0,5M] unit :byte
       */
        m_mp4_cfg.stMuxerConfig.u32VBufSize = 512 * 1024; 

        m_mp4_cfg.stMuxerConfig.bConstantFps = TD_TRUE;

        /* pre allocate size in bytes, set [0, 100M]
         * 0 for not use pre allocate function
         * suggest 20M, unit :byte
         */
        m_mp4_cfg.stMuxerConfig.u32PreAllocUnit = 0;

        m_mp4_cfg.stMuxerConfig.bCo64Flag = TD_FALSE;

        /* 100 * 1024 * 1024 :
         * stbl group unit[512k, 500M]
         * if 0 not use backup data and repair
         */
        m_mp4_cfg.stMuxerConfig.u32BackupUnit = 0;

        memset(&m_mp4_video_info,0,sizeof(m_mp4_video_info));
        m_mp4_video_info.enTrackType = OT_MP4_STREAM_VIDEO;
        if(m_mh.video_info.vcode == ceanic::util::STREAM_VIDEO_ENCODE_H264)
        {
            m_mp4_video_info.stVideoInfo.enCodecID = OT_MP4_CODEC_ID_H264;
        }
        else
        {
            m_mp4_video_info.stVideoInfo.enCodecID = OT_MP4_CODEC_ID_H265;
        }
        m_mp4_video_info.fSpeed = 1.0f;
        m_mp4_video_info.u32TimeScale = 120000;                 /* 120000: time scale for each trak */
        m_mp4_video_info.stVideoInfo.u32BitRate = 3000;         /* 3000: bitrate bps */
        m_mp4_video_info.stVideoInfo.frameRate =  m_mh.video_info.fr;           /* 30: frame rate fps */
        m_mp4_video_info.stVideoInfo.u32Height = m_mh.video_info.h;             /* 1080: video height */
        m_mp4_video_info.stVideoInfo.u32Width = m_mh.video_info.w;              /* 1920: video width */

        memset(&m_mp4_audio_info,0,sizeof(m_mp4_audio_info));
        m_mp4_audio_info.enTrackType = OT_MP4_STREAM_AUDIO;
        m_mp4_audio_info.fSpeed = 1.0f;
        m_mp4_audio_info.stAudioInfo.u16SampleSize = m_mh.audio_info.bit_width;
        m_mp4_audio_info.u32TimeScale = m_mh.audio_info.sample_rate;
        m_mp4_audio_info.stAudioInfo.u32SampleRate = m_mh.audio_info.sample_rate;
        m_mp4_audio_info.stAudioInfo.u32Channels = m_mh.audio_info.chn; 
        if(m_mh.audio_info.acode == ceanic::util::STREAM_AUDIO_ENCODE_AAC)
        {
            m_mp4_audio_info.stAudioInfo.enCodecID = OT_MP4_CODEC_ID_AACLC;
            m_mp4_audio_info.stAudioInfo.u32SamplePerFrame = 1024;
        }
        else
        {
            m_mp4_audio_info.stAudioInfo.enCodecID = OT_MP4_CODEC_ID_G711_U;
            m_mp4_audio_info.stAudioInfo.u32SamplePerFrame = 480;
        }
    }

    mp4_save::~mp4_save()
    {
        assert(!m_bopen);
    }

    bool mp4_save::open()
    {
        std::unique_lock<std::mutex> lock(m_interface_mu);

        TD_S32 ret;
        TD_U64 duration;

        if(m_bopen)
        {
            return false;
        }

        ret = SS_MP4_Create(&m_mp4_fh,&m_mp4_cfg);
        if(ret != TD_SUCCESS)
        {
            printf("[%s]:SS_MP4_Create failed with 0x%x\n",__FUNCTION__,ret);
            return false;
        }
        
        ret = SS_MP4_CreateTrack(m_mp4_fh, &m_mp4_video_track_h, &m_mp4_video_info);
        if(ret != TD_SUCCESS)
        {
            printf("[%s]:SS_MP4_CreateTrack failed with 0x%x\n",__FUNCTION__,ret);
            SS_MP4_Destroy(m_mp4_fh,&duration);
            return false;
        }

        if(m_mh.audio_info.acode != ceanic::util::STREAM_AUDIO_ENCODE_NONE)
        {
            ret = SS_MP4_CreateTrack(m_mp4_fh, &m_mp4_audio_track_h, &m_mp4_audio_info);
            if(ret != TD_SUCCESS)
            {
                printf("[%s]:SS_MP4_CreateTrack failed with 0x%x\n",__FUNCTION__,ret);
                SS_MP4_Destroy(m_mp4_fh,&duration);
                return false;
            }
        }

        m_bopen = true;
        m_process_thread = std::thread(&mp4_save::on_process,this);

        return true;
    }

    void mp4_save::process_audio_frame(const char* frame_buf,int32_t frame_len)
    {
        OT_MP4_FRAME_DATA_S frame_info;
        memset(&frame_info,0,sizeof(frame_info));

        struct timeval tv;
        gettimeofday(&tv, NULL);

        frame_info.pu8DataBuffer = (TD_U8*)frame_buf;
        frame_info.u32DataLength = frame_len;
        frame_info.bKeyFrameFlag = TD_FALSE;
        frame_info.u64TimeStamp = (TD_U64)tv.tv_sec * 1000000 + (TD_U64)tv.tv_usec;

        TD_S32 ret = SS_MP4_WriteFrame(m_mp4_fh, m_mp4_audio_track_h, &frame_info);
        if(ret != TD_SUCCESS)
        {
            printf("[%s]:SS_MP4_WriteFrame failed with 0x%x\n",__FUNCTION__,ret);
        }
    }

    void mp4_save::process_video_frame(const char* frame_buf,int32_t frame_len)
    {
        OT_MP4_FRAME_DATA_S frame_info;
        memset(&frame_info,0,sizeof(frame_info));

        struct timeval tv;
        gettimeofday(&tv, NULL);

        TD_BOOL key_frame = TD_FALSE;
        if(m_mh.video_info.vcode == ceanic::util::STREAM_VIDEO_ENCODE_H264)
        {
            key_frame = ((frame_buf[4] & 0x1f) == 0x7) ? TD_TRUE: TD_FALSE;//sps
        }
        else
        {
            int32_t h265_nalu_type = (frame_buf[4] >> 1) & 0x3f;
            key_frame = (h265_nalu_type == 32) ? TD_TRUE: TD_FALSE;
        }

        frame_info.pu8DataBuffer = (TD_U8*)frame_buf;
        frame_info.u32DataLength = frame_len;
        frame_info.bKeyFrameFlag = key_frame;
        frame_info.u64TimeStamp = (TD_U64)tv.tv_sec * 1000000 + (TD_U64)tv.tv_usec;

        TD_S32 ret = SS_MP4_WriteFrame(m_mp4_fh, m_mp4_video_track_h, &frame_info);
        if(ret != TD_SUCCESS)
        {
            printf("[%s]:SS_MP4_WriteFrame failed with 0x%x\n",__FUNCTION__,ret);
        }
    }

    void mp4_save::on_process()
    {
        bool get_data = false;
        int32_t frame_len = 0;
        int32_t frame_type = 0;
        char* frame_buf= new char[512*1024];

        while(m_bopen)
        {
            get_data = false;

            {
                std::unique_lock<std::mutex> sl(m_buf_mu);
                if(m_sbuf.get_stream_len() >= 8 
                        && m_sbuf.copy_data(&frame_len,4)
                        && m_sbuf.copy_data(&frame_type,4)
                        && m_sbuf.get_stream_len() > 8 + frame_len)
                {
                    m_sbuf.get_data(&frame_len,4);
                    m_sbuf.get_data(&frame_type,4);
                    m_sbuf.get_data(frame_buf,frame_len);
                    get_data = true;
                }
            }

            if(!get_data)
            {
                usleep(10000);
                continue;
            }

            //printf("[%s]:get frame len:%d\n)",__FUNCTION__,frame_len);
            if(frame_type == 0)
            {
                process_video_frame(frame_buf,frame_len);
            }
            else
            {
                process_audio_frame(frame_buf,frame_len);
            }
        }

        delete[] frame_buf;
    }

    void mp4_save::close()
    {
        std::unique_lock<std::mutex> lock(m_interface_mu);

        TD_U64 duration = 0;
        TD_S32 ret;

        if(!m_bopen)
        {
            return ;
        }

        m_bopen = false;
        m_process_thread.join();

        ret = SS_MP4_DestroyAllTracks(m_mp4_fh, NULL);
        if(ret != TD_SUCCESS)
        {
            printf("[%s]:SS_MP4_DEstroyAllTracks failed with 0x%x\n",__FUNCTION__,ret);
        }

        ret = SS_MP4_Destroy(m_mp4_fh,&duration);
        if(ret != TD_SUCCESS)
        {
            printf("[%s]:SS_MP4_DEstroy failed with 0x%x\n",__FUNCTION__,ret);
        }

        m_mp4_fh = NULL;
        m_mp4_video_track_h = NULL;
    }

    bool mp4_save::is_open()
    {
        std::unique_lock<std::mutex> lock(m_interface_mu);

        return m_bopen;
    }

    bool mp4_save::input_data(ceanic::util::stream_head* head,const char* buf,int32_t len)
    {
        std::unique_lock<std::mutex> lock(m_interface_mu);

        if(!m_bopen)
        {
            return false;
        }

        {
            int32_t frame_len = 0;
            int32_t frame_type = 0;//0-video 1-audio

            std::unique_lock<std::mutex> sl(m_buf_mu);
            if(IS_VIDEO_FRAME(head->type))
            {
                for(uint32_t i = 0; i < head->nalu_count; i++)
                {
                    frame_len += head->nalu[i].size;
                }

                if(m_sbuf.get_remain_len() < 8 + frame_len)
                {
                    printf("[%s]:buf is full\n",__FUNCTION__);
                    return false;
                }

                m_sbuf.input_data(&frame_len,4);
                m_sbuf.input_data(&frame_type,4);
                for(uint32_t i = 0; i < head->nalu_count; i++)
                {
                    m_sbuf.input_data((void*)head->nalu[i].data,head->nalu[i].size);
                }

                return true;
            }
            else if(IS_AUDIO_FRAME(head->type))
            {
                if(m_mh.audio_info.acode == ceanic::util::STREAM_AUDIO_ENCODE_NONE)
                {
                    return false;
                }

                frame_len = len;
                frame_type = 1;

                if(m_sbuf.get_remain_len() < 8 + frame_len)
                {
                    printf("[%s]:buf is full\n",__FUNCTION__);
                    return false;
                }
                m_sbuf.input_data(&frame_len,4);
                m_sbuf.input_data(&frame_type,4);
                m_sbuf.input_data((void*)buf,len);

                return true;
            }
        }

        return true;
    }

}}//namespace

