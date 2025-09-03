include ../Makefile.param
CXX:=$(CROSS)g++
STRIP=$(CROSS)strip
CXXFLAGS += -std=c++17

#关闭初始化顺序警告
CXXFLAGS += -Wno-reorder

#关闭changed in GCC 7.1警告
CXXFLAGS += -Wno-psabi

THIRD_LIBRARY_PATH=./thirdlibrary
INC_PATH += -I./
INC_PATH += -I./util/
INC_PATH += -I./json/
INC_PATH += -I./log/

INC_PATH += -I./rtsp/
INC_PATH += -I./rtsp/stream/
INC_PATH += -I./rtsp/rtp_serialize/
INC_PATH += -I./rtsp/rtp_session/
INC_PATH += -I./aiisp/
INC_PATH += -I./device/

INC_PATH += -I$(THIRD_LIBRARY_PATH)/freetype-2.7.1/include/freetype2
#INC_PATH += -I$(THIRD_LIBRARY_PATH)/boost_1_60_0/include/
INC_PATH += -I$(THIRD_LIBRARY_PATH)/log4cpp/include/
INC_PATH += -I$(THIRD_LIBRARY_PATH)/libevent-2.0.18-stable/include
INC_PATH += -I$(THIRD_LIBRARY_PATH)/rtmpdump/include/

LIBPATH += -L./
LIBPATH += -L./log

SRCXX += main.cpp
SRCXX += json/json_reader.cpp
SRCXX += json/json_writer.cpp
SRCXX += json/json_value.cpp

#log
SRCXX += log/ceanic_log.cpp
SRCXX += log/ceanic_log4cpp.cpp

#rtsp server
SRCXX += rtsp/server.cpp
SRCXX += rtsp/session.cpp
SRCXX += rtsp/rtsp_session.cpp
SRCXX += rtsp/rtsp_request_handler.cpp
SRCXX += rtsp/request_parser.cpp
SRCXX += rtsp/stream/stream_handler.cpp
SRCXX += rtsp/stream/stream_manager.cpp
SRCXX += rtsp/stream/stream_stock.cpp
SRCXX += rtsp/stream/stream_video_handler.cpp
SRCXX += rtsp/stream/stream_audio_handler.cpp
SRCXX += rtsp/rtp_session/rtp_session.cpp
SRCXX += rtsp/rtp_session/rtp_tcp_session.cpp
SRCXX += rtsp/rtp_session/rtp_udp_session.cpp
SRCXX += rtsp/rtp_serialize/h264_rtp_serialize.cpp
SRCXX += rtsp/rtp_serialize/h265_rtp_serialize.cpp
SRCXX += rtsp/rtp_serialize/rtp_serialize.cpp
SRCXX += rtsp/rtp_serialize/pcmu_rtp_serialize.cpp
SRCXX += rtsp/rtp_serialize/aac_rtp_serialize.cpp
SRCXX += rtsp/rtp_serialize/rtp_serialize.cpp

#rtmp
SRCXX += rtmp/session.cpp
SRCXX += rtmp/session_manager.cpp

#aiisp
SRCXX += aiisp/aiisp.cpp
SRCXX += aiisp/aiisp_bnr.cpp
SRCXX += aiisp/aiisp_drc.cpp
SRCXX += aiisp/aiisp_3dnr.cpp

#device
DEVICE_SRC += device/dev_sys.cpp
DEVICE_SRC += device/dev_vi.cpp
DEVICE_SRC += device/dev_vi_isp.cpp
DEVICE_SRC += device/dev_vi_os04a10_liner.cpp
DEVICE_SRC += device/dev_vi_os04a10_2to1wdr.cpp
DEVICE_SRC += device/dev_vi_os08a20_liner.cpp
DEVICE_SRC += device/dev_vi_os08a20_2to1wdr.cpp
DEVICE_SRC += device/dev_venc.cpp
DEVICE_SRC += device/dev_chn.cpp
DEVICE_SRC += device/dev_osd.cpp
DEVICE_SRC += device/dev_snap.cpp
DEVICE_SRC += device/dev_svp.cpp
DEVICE_SRC += device/dev_svp_yolov5.cpp
DEVICE_SRC += device/ceanic_freetype.cpp
DEVICE_SRC += device/dev_std.cpp
DEVICE_SRC += device/dev_vo.cpp
DEVICE_SRC += device/dev_vo_bt1120.cpp

#surpport scene
SCENE_PATH = ../scene_auto
INC_PATH += -I$(SCENE_PATH)/include
INC_PATH += -I$(SCENE_PATH)/src/sample
INC_PATH += -I$(SCENE_PATH)/../common
INC_PATH += -I$(SCENE_PATH)/tools/configaccess/include
SRC += $(SCENE_PATH)/src/core/ot_scene.c
SRC += $(SCENE_PATH)/src/core/ot_scene_setparam.c
SRC += $(SCENE_PATH)/src/core/scene_setparam_inner.c
SRC += $(SCENE_PATH)/src/sample/scene_loadparam.c
#SRC += $(SCENE_PATH)/tools/configaccess/src/ot_confaccess.c
SRC += ../common/sample_comm_vi.c
SRC += ../common/ot_confaccess.c
SRC += ../common/sample_comm_isp.c

#support rate_auto
SVC_RATE_PATH = ../svc_rate_auto
INC_PATH += -I$(SVC_RATE_PATH)/include
SRC += $(SVC_RATE_PATH)/src/ot_bitrate_auto.c
SRC += $(SVC_RATE_PATH)/src/bitrate_auto_load_param.c

#support stream_save
INC_PATH += -I$(THIRD_LIBRARY_PATH)/hisilicon_mp4/include
INC_PATH += -I./stream_save/
SRCXX += stream_save/mp4_save.cpp

LIBS += -Wl,--start-group

LIBS += $(THIRD_LIBRARY_PATH)/rtmpdump/lib/librtmp.a
LIBS += $(THIRD_LIBRARY_PATH)/log4cpp/lib/liblog4cpp.a
LIBS += $(THIRD_LIBRARY_PATH)/libevent-2.0.18-stable/lib/libevent.a
LIBS += $(THIRD_LIBRARY_PATH)/freetype-2.7.1/lib/libfreetype.a
LIBS += $(THIRD_LIBRARY_PATH)/hisilicon_mp4/lib/libmp4.a
LIBS += $(THIRD_LIBRARY_PATH)/hisilicon_mp4/lib/libuproc.a
LIBS += $(THIRD_LIBRARY_PATH)/hisilicon_mp4/lib/libmwlog.a
LIBS += $(MPI_LIBS) $(REL_LIB)/libsecurec.a $(REL_LIB)/libss_mpi_snap.a

LIBS += $(THIRD_LIBRARY_PATH)/sensor/os04a10/libsns_os04a10.a
LIBS += $(THIRD_LIBRARY_PATH)/sensor/os08a20/libsns_os08a20.a

LIBS+= -Wl,--end-group

LIBS+= -Wl,-gc-sections

target = ceanic_app

all: $(target)

PROGXX_OBJ= $(SRCXX:.cpp=.o)
PROG_OBJ= $(SRC:.c=.o)
DEVICE_OBJ= $(DEVICE_SRC:.cpp=.o)

$(target):$(PROGXX_OBJ) $(PROG_OBJ) $(DEVICE_OBJ)
	@echo "LD $(target)"
	@$(CXX) $^ -o $@ $(INC_PATH) $(LIBPATH) $(LIBS) $(CFLAGS) -lpthread

%.o:%.cpp
	@echo "CXX $<"
	@$(CXX) -c -o  $@ $< $(INC_PATH) $(CFLAGS) $(CXXFLAGS)

%.o:%.c
	@echo "CC $<"
	@$(CC) -c -o  $@ $< $(INC_PATH) $(CFLAGS)

device_clean:
	-rm $(DEVICE_OBJ)

clean:
	-rm $(target) $(PROG_OBJ) $(PROGXX_OBJ) $(DEVICE_OBJ)

strip:
	$(STRIP) $(target)

install:
	cp $(target) /home/mjj/work/nfs/3519dv500/021/

