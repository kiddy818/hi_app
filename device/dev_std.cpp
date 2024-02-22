#include "dev_std.h"
#include "ceanic_freetype.h"

const char* g_week_stsr[7] = {"星期日","星期一","星期二","星期三","星期四","星期五","星期六"};
ceanic_freetype g_freetype("/opt/ceanic/fonts/Vera.ttf","/opt/ceanic/fonts/gbsn00lp.ttf");

int rgb24to1555(int r,int g,int b,int a)
{
    int r1555 = r & 0x1f;
    int g1555 = g & 0x1f; 
    int b1555 = b & 0x1f;

    if(a)
    {
        return  0x8000 | (r1555 << 10) | (g1555 << 5) | b1555;
    }
    else
    {
        return  (r1555 << 10) | (g1555 << 5) | b1555;
    }
}

