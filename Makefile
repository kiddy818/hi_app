include ../Makefile.param

STRIP=$(CROSS)strip
CFLAGS += -g -Wall -O2

THIRD_LIBRARY_PATH=../thirdlibrary
INC_PATH += -I./
INC_PATH += -I./util/
INC_PATH += -I./json/

INC_PATH += -I./rtsp/
INC_PATH += -I./rtsp/stream/
INC_PATH += -I./rtsp/rtp_serialize/
INC_PATH += -I./rtsp/rtp_session/

INC_PATH += -I../beacon_device/

INC_PATH += -I$(THIRD_LIBRARY_PATH)/boost_1_60_0/mybuild_aarch64_mix210/include/
INC_PATH += -I$(THIRD_LIBRARY_PATH)/log4cpp/mybuild_aarch64_v01c01_linux_gnu/include/
INC_PATH += -I$(THIRD_LIBRARY_PATH)/libevent-2.0.18-stable/mybuild_aarch64_v01c01_linux_gnu/include
INC_PATH += -I$(THIRD_LIBRARY_PATH)/rtmpdump/librtmp/mybuild_aarch64_v01c01_linux_gnu/include/

LIBPATH += -L./
LIBPATH += -L./log
LIBPATH += -L../beacon_device/

SRCXX += main.cpp
SRCXX += json/jsoncpp.cpp

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
SRCXX += rtsp/rtp_session/rtp_session.cpp
SRCXX += rtsp/rtp_session/rtp_tcp_session.cpp
SRCXX += rtsp/rtp_session/rtp_udp_session.cpp
SRCXX += rtsp/rtp_serialize/h264_rtp_serialize.cpp
SRCXX += rtsp/rtp_serialize/mjpeg_rtp_serialize.cpp
SRCXX += rtsp/rtp_serialize/rtp_serialize.cpp

#rtmp
SRCXX += rtmp/session.cpp
SRCXX += rtmp/session_manager.cpp

LIBS += -Wl,--start-group

LIBS += $(THIRD_LIBRARY_PATH)/rtmpdump/librtmp/mybuild_aarch64_v01c01_linux_gnu/lib/librtmp.a
LIBS += ./log/libbeacon_log.a
LIBS += $(THIRD_LIBRARY_PATH)/log4cpp/mybuild_aarch64_v01c01_linux_gnu/lib/liblog4cpp.a
LIBS += $(THIRD_LIBRARY_PATH)/libevent-2.0.18-stable/mybuild_aarch64_v01c01_linux_gnu/lib/libevent.a
LIBS += $(THIRD_LIBRARY_PATH)/freetype-2.7.1/mybuild_aarch64_v01c01_linux_gnu/lib/libfreetype.a
LIBS += ../beacon_device/libbeacon_device.a 
LIBS += $(MPI_LIBS) $(SENSOR_LIBS) $(AUDIO_LIBA) $(REL_LIB)/libsecurec.a

LIBS+= -Wl,--end-group

target = beacon_app

all: $(target)

PROGXX_OBJ= $(SRCXX:.cpp=.o)
PROG_OBJ= $(SRC:.c=.o)

$(target):$(PROGXX_OBJ) $(PROG_OBJ)
	$(CXX) $^ -o $@ $(INC_PATH) $(LIBPATH) $(LIBS) $(CFLAGS) -lpthread
	$(STRIP) $(target)

%.o:%.cpp
	$(CXX) -c -o  $@ $< $(INC_PATH) $(CFLAGS)

%.o:%.c
	$(CC) -c -o  $@ $< $(INC_PATH) $(CFLAGS)

clean:
	-rm $(target) $(PROG_OBJ) $(PROGXX_OBJ)

install:
	cp $(target) /home/mjj/work/nfs/3516dv500/

