#ifndef dev_std_include_h
#define dev_std_include_h

#include "ot_common.h"
#include "ot_math.h"
#include "ot_buffer.h"
#include "ot_defines.h"
#include "securec.h"
#include "ot_mipi_rx.h"
#include "ot_mipi_tx.h"
#include "ss_mpi_sys.h"
#include "ss_mpi_sys_bind.h"
#include "ss_mpi_vb.h"
#include "ss_mpi_vi.h"
#include "ss_mpi_isp.h"
#include "ss_mpi_vo.h"
#include "ss_mpi_vo_dev.h"
#include "ss_mpi_venc.h"
#include "ss_mpi_vdec.h"
#include "ss_mpi_vpss.h"
#include "ss_mpi_region.h"
#include "ss_mpi_audio.h"
#include "ss_mpi_vgs.h"
#include "ss_mpi_awb.h"
#include "ss_mpi_ae.h"
#include "ot_sns_ctrl.h"
#include "ss_mpi_isp.h"
#include "ss_mpi_snap.h"
#include "ss_mpi_sys_mem.h"

#include <util/std.h>
#include <string>
#include <thread>
#include <vector>
#include <functional>
#include <list>

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
}isp_exposure_t;

#ifndef ROUND_DOWN
#define ROUND_DOWN(size, align) ((size) & ~((align) - 1))
#endif

#ifndef ROUND_UP
#define ROUND_UP(size, align)   (((size) + ((align) - 1)) & ~((align) - 1))
#endif

int rgb24to1555(int r,int g,int b,int a);

class ceanic_freetype;
extern ceanic_freetype g_freetype;
extern const char* g_week_stsr[7];

#endif

