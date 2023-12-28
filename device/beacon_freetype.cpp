#include "beacon_std.h"
#include <mutex>
#include "beacon_freetype.h"
#include "beacon_time_interval.h"
#include <vector>
#include <locale>
#include <string>
#include <codecvt>

static std::vector<basic_char_t> g_basic_chars;
static std::mutex g_freetype_mutex;

beacon_freetype::beacon_freetype(const char* en_path,const char* zh_path)
	:m_en_path(en_path),m_zh_path(zh_path),m_inited(false)
{
}

beacon_freetype::~beacon_freetype()
{
	assert(!is_init());
}

bool beacon_freetype::get_glyph_char(wchar_t c,int font_size,basic_char_t* char_info)
{
    std::unique_lock<std::mutex> lock(g_freetype_mutex);

	for(unsigned int i = 0; i < g_basic_chars.size(); i++)
	{
		if(g_basic_chars[i].c == c
				&& g_basic_chars[i].font == font_size)
		{
			*char_info =  g_basic_chars[i];
			return true;
		}
	}

    FT_Face glyph_face =  c > 127 ? m_ft_face_zh : m_ft_face_en;
    //FT_Face glyph_face =  m_ft_face_en;
	FT_Error error = FT_Set_Pixel_Sizes(glyph_face,font_size,0);
    if(error)
	{
		printf("beacon_freetype:get_glyph_char#set pixel size error\n");
		return false;
	}

    FT_Glyph glyph_ori;
    FT_Glyph glyph_outline;

    if(!get_glyph(glyph_face,(FT_ULong)c,&glyph_ori,&glyph_outline))
    {
		printf("beacon_freetype:get_glyph_char#get_glyph error\n");
        return false;
    }

    basic_char_t basic_char;
    basic_char.c = c;
    basic_char.font = font_size;
    basic_char.glyph_ori = glyph_ori;
    basic_char.glyph_outline = glyph_outline;
    basic_char.w = glyph_face->glyph->metrics.horiAdvance >> 6;
    basic_char.top = font_size - (glyph_face->glyph->metrics.horiBearingY >> 6);
    if(basic_char.top > font_size)
    {
        basic_char.top = font_size;
    }
    g_basic_chars.push_back(basic_char);

    *char_info = basic_char;

    printf("update glyph %x\n",c);
	return true;
}

bool beacon_freetype::init()
{
	if(is_init())
	{
		return false;
	}

	FT_Error error = FT_Init_FreeType(&m_ft_lib);
	if(error)
	{
		return false;
	}

    error = FT_New_Face(m_ft_lib,m_en_path.c_str(),0,&m_ft_face_en);
	if(error)
	{
		return false;
	}

#if 1
    error = FT_New_Face(m_ft_lib,m_zh_path.c_str(),0,&m_ft_face_zh);
	if(error)
	{
		return false;
	}
#endif

	m_inited = true;
	return true;
}

bool beacon_freetype::is_init()
{
	return m_inited;
}

void beacon_freetype::release()
{
	if(is_init())
	{
		FT_Done_Face(m_ft_face_en);
#if 1
		FT_Done_Face(m_ft_face_zh);
#endif
		FT_Done_FreeType(m_ft_lib);
		m_inited = false;
	}
}

bool beacon_freetype::get_width(const char* str,int font_pixel,int* w)
{
	if(!is_init())
	{
		return false;
	}
	
	*w = 0;

    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    std::wstring wstr = conv.from_bytes(str);

    basic_char_t char_info;
    
    for(auto i = 0; i < wstr.size(); i++)
    {
        get_glyph_char(wstr[i],font_pixel,&char_info);

        *w +=  char_info.w;
    }

	return true;
}

bool beacon_freetype::get_max_width(const char* str,int font_pixel,int* w)
{
    if(!is_init())
	{
		return false;
	}
	
	*w = 0;

    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    std::wstring wstr = conv.from_bytes(str);

    basic_char_t char_info;
    
    for(auto i = 0; i < wstr.size(); i++)
    {
        get_glyph_char(wstr[i],font_pixel,&char_info);

        if(char_info.w > *w)
        {
            *w = char_info.w;
        }
    }

    return true;
}

bool beacon_freetype::show_string(const char* str,int area_w,int area_h,int font_pixel,int outline,unsigned char* pdata,int data_size)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    std::wstring wstr = conv.from_bytes(str);

    basic_char_t char_info;

	int startx=0;
    int starty=0;
	for(int i = 0; i < wstr.size(); i++)
	{
		if(!get_glyph_char(wstr[i],font_pixel,&char_info))
		{
			printf("get_glphy failed\n");
			continue;
		}

        starty = char_info.top;

		draw_yuv_char(area_w,area_h,char_info.w,outline,char_info.glyph_ori,char_info.glyph_outline,startx,starty,pdata,data_size);

        startx += char_info.w;
        if(startx > area_w)
        {
            printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>c=%c,startx=%d,area_w=%d\n",str[i],startx,area_w);
            break;
        }
    }

	return true;
}


bool beacon_freetype::show_string_compare(const char* str_before,const char* str_now,int area_w,int area_h,int font_pixel,int outline,unsigned char* pdata,int data_size)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    std::wstring wstr_before = conv.from_bytes(str_before);
    std::wstring wstr_cur = conv.from_bytes(str_now);

	if(wstr_cur.size() != wstr_before.size())
	{
		return false;
	}

	int startx =0;
    int starty =0;
	bool draw_char = false;
    basic_char_t char_info;
	for(int i = 0; i < wstr_cur.size(); i++)
	{
		//从i之后的所有字符都要修改
		if(wstr_cur[i] != wstr_before[i])
		{
			draw_char = true;
		}

        if(!get_glyph_char(wstr_cur[i],font_pixel,&char_info))
        {
            printf("get_glphy failed\n");
            continue;
        }

		if(draw_char)
		{
            starty = char_info.top;

			draw_yuv_char(area_w,area_h,char_info.w,outline,char_info.glyph_ori,char_info.glyph_outline,startx,starty,pdata,data_size);
        }

        startx += char_info.w;

        if(startx > area_w)
        {
            printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>c=%c,startx=%d,area_w=%d\n",str_now[i],startx,area_w);
            break;
        }
    }

	return true;
}


bool beacon_freetype::get_glyph(FT_Face face,FT_ULong ch,FT_Glyph* orig,FT_Glyph* outline)
{
    float outline_pixel = 1.0;

	FT_Error error;
	FT_GlyphSlot slot = face->glyph;

    error = FT_Load_Char(face,ch,FT_LOAD_DEFAULT);
	if( error ) 
	{ 
		printf("here 0\n");
		return false;
	} 

	FT_Get_Glyph( slot, orig );
	FT_Glyph_Copy(*orig,outline);      

	/*  Render the glyph */
	error = FT_Glyph_To_Bitmap(orig, ft_render_mode_normal, 0, 1 );                                                   
	if( error )
	{
		printf("here 1\n");
		goto fail;
	}

	FT_Stroker stroker;
	error = FT_Stroker_New(m_ft_lib, &stroker );
	if( error )
	{
		goto fail;
	}
	FT_Stroker_Set( stroker, outline_pixel * 64, FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0 );
	FT_Glyph_Stroke( outline, stroker, 1 /*  delete the original glyph */ );
	FT_Stroker_Done( stroker );

	/*  Render the glyph */ 
	error = FT_Glyph_To_Bitmap(outline, ft_render_mode_normal, 0, 1 );
	if( error ) 
	{
		printf("here 2\n");
		goto fail;
	}

	return true;

fail:
	FT_Done_Glyph(*orig);
	FT_Done_Glyph(*outline);
	return false;
}

extern int rgb24to1555(int r,int g,int b,int a);
#define SHOW_LEVEL (5)
bool beacon_freetype::draw_yuv_char(int area_w,int area_h,int font_pixel,int with_outline,FT_Glyph orig,FT_Glyph outline,int startx,int starty,unsigned char* buf,int size)
{
	FT_Bitmap * orig_bitmap = &((FT_BitmapGlyph)orig)->bitmap; 
	FT_Bitmap * outline_bitmap = &((FT_BitmapGlyph)outline)->bitmap; 
	unsigned char *pdata;

	int total_width = font_pixel;
	//int offset = (total_width-outline_bitmap->width)>>1; /* 让字居中显示 */
	int offset=0;
	int h_offset = (outline_bitmap->width-orig_bitmap->width)>>1;
	int v_offset = (outline_bitmap->rows-orig_bitmap->rows)>>1;

#if 1
	if (startx + total_width > area_w)
	{
        //mjj need to fixed
		printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>draw here 0\n");
        //system("killall -9 osd_test.sh");
        //exit(0);
		return false;
	}
#endif

#if 0
    char hide_value = 0;//clut id=2
    char point_value = 0xff;//clut id = 0
    char outline_value = with_outline ? 1 : 2;
#else
    int hide_value = rgb24to1555(0,255,0,0);
    int point_value = rgb24to1555(255,255,255,1);
    int outline_value = rgb24to1555(0,0,0,1);
#endif

	for (int i=0; i<area_h; i++)
	{
        //printf("\n");
		for (int j=0; j<total_width; j++)
		{
			pdata = buf + i * area_w * 2 + (startx + j) * 2;//RGB1555,so need to *2
            assert(pdata < buf + size);

			//不在显示字体的区域内
			if (i<starty || (unsigned int) i>= starty+outline_bitmap->rows)
			{
				//*pdata = hide_value ; 
                memcpy(pdata,&hide_value,2);
			} 
			else 
			{
				//显示字
				if (j>=offset && (unsigned int) j < outline_bitmap->width+offset)
				{
					if (j>=offset+h_offset && (unsigned int) j<orig_bitmap->width+offset+h_offset \
						&& i>=starty+v_offset && (unsigned int) i<orig_bitmap->rows+starty+v_offset)
					{
						if (orig_bitmap->buffer[(i-starty-v_offset)*orig_bitmap->width+j-offset-h_offset]>=SHOW_LEVEL)
						{
							//点
                            //printf("#");
							//*pdata = point_value;
                            memcpy(pdata,&point_value,2);
						}
						else if (outline_bitmap->buffer[(i-starty)*outline_bitmap->width+j-offset]>=SHOW_LEVEL)
						{
							//描边点
							//*pdata = outline_value;
                            memcpy(pdata,&outline_value,2);
                            //printf("y");
						}
						else {
							//*pdata = hide_value;
                            memcpy(pdata,&hide_value,2);
                            //printf("x");
						}
					}
					else
					{
						if (outline_bitmap->buffer[(i-starty)*outline_bitmap->width+j-offset]>=SHOW_LEVEL) 
						{
							//描边点
							//*pdata = outline_value;
                            memcpy(pdata,&outline_value,2);
                            //printf("b");
						}
						else {
							//空
							//*pdata = hide_value;
                            memcpy(pdata,&hide_value,2);
                            //printf("a");
						}
					}
				}
				else 
				{
                    //printf("z");
					//*pdata = hide_value;
                    memcpy(pdata,&hide_value,2);
				}
			}
		}
	}
	
	return true;
}
