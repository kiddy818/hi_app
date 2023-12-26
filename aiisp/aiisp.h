#include <ot_common_aiisp.h>
#include <ss_mpi_sys_mem.h>
class aiisp
{
public:
    aiisp(){}
    virtual ~aiisp(){}

    virtual bool start() = 0;
    virtual void stop() = 0;

    static bool read_model(const char* model_file,ot_aiisp_mem_info* meminfo);
};
