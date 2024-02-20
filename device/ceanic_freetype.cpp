#include <mutex>
#include "ceanic_freetype.h"
#include <vector>
#include <locale>
#include <string>
#include <codecvt>
#include "dev_log.h"

static std::vector<basic_char_t> g_basic_chars;
static std::mutex g_freetype_mutex;

ceanic_freetype::ceanic_freetype(const char* en_path,const char* zh_path)
	:m_en_path(en_path),m_zh_path(zh_path),m_inited(false)
{
}

ceanic_freetype::~ceanic_freetype()
{
	assert(!is_init());
}

bool ceanic_freetype::get_glyph_char(wchar_t c,int font_size,basic_char_t* char_info)
{
    std::unique_lock<std::mutex> lock(g_freetype_mutex);

	for(unsigned int i = 0; i < g_basic_chars.size(); i++)
	{
		if(g_basic_chars[i].c == c
				&& g_basic_chars[i].font_size == font_size)
		{
			*char_info =  g_basic_chars[i];
			return true;
		}
	}

    FT_Face glyph_face =  c > 127 ? m_ft_face_zh : m_ft_face_en;
	FT_Error error = FT_Set_Pixel_Sizes(glyph_face,font_size,0);
    if(error)
	{
        DEV_WRITE_LOG_ERROR("FT_Set_Pixel_Sizes failed");
		return false;
	}

    FT_Glyph glyph_ori;
    FT_Glyph glyph_outline;

    if(!get_glyph(glyph_face,(FT_ULong)c,&glyph_ori,&glyph_outline))
    {
        DEV_WRITE_LOG_ERROR("get_glyph failed");
        return false;
    }

    basic_char_t basic_char;
    basic_char.c = c;
    basic_char.font_size = font_size;
    basic_char.glyph_ori = glyph_ori;
    basic_char.glyph_outline = glyph_outline;
    basic_char.w = glyph_face->glyph->advance.x >> 6;
    basic_char.top = (font_size - (glyph_face->glyph->metrics.horiBearingY >> 6)) / 2;
    if(basic_char.top > font_size)
    {
        basic_char.top = font_size;
    }

    g_basic_chars.push_back(basic_char);

    *char_info = basic_char;
	return true;
}

bool ceanic_freetype::init()
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

    error = FT_New_Face(m_ft_lib,m_zh_path.c_str(),0,&m_ft_face_zh);
	if(error)
	{
		return false;
	}

	m_inited = true;
	return true;
}

bool ceanic_freetype::is_init()
{
	return m_inited;
}

void ceanic_freetype::release()
{
	if(is_init())
	{
		FT_Done_Face(m_ft_face_en);
		FT_Done_Face(m_ft_face_zh);
		FT_Done_FreeType(m_ft_lib);
		m_inited = false;
	}
}

bool ceanic_freetype::get_width(const char* str,int font_pixel,int* w)
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

bool ceanic_freetype::get_max_width(const char* str,int font_pixel,int* w)
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
            *w = char_info.w ;
        }
    }

    return true;
}

bool ceanic_freetype::show_string(const char* str,int area_w,int area_h,int font_pixel,unsigned char* pdata,int data_size,short bg_color,short fg_color,short outline_color)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    std::wstring wstr = conv.from_bytes(str);

    basic_char_t char_info;

	int startx=0;
    int starty = 0;
	for(int i = 0; i < wstr.size(); i++)
	{
		if(!get_glyph_char(wstr[i],font_pixel,&char_info))
		{
            DEV_WRITE_LOG_ERROR("get_glyph failed");
			continue;
		}

        starty = char_info.top;

		draw_rgb1555_char(area_w,
                area_h,
                char_info.w,
                char_info.glyph_ori,
                char_info.glyph_outline,
                startx,
                starty,
                pdata,
                data_size,
                bg_color,
                fg_color,
                outline_color);

        startx += char_info.w;
        if(startx > area_w)
        {
            DEV_WRITE_LOG_ERROR("unexpcepted error,startx:%d,area_w:%d",startx,area_w);
            break;
        }
    }

	return true;
}


bool ceanic_freetype::show_string_compare(const char* str_before,const char* str_now,int area_w,int area_h,int font_pixel,unsigned char* pdata,int data_size,short bg_color,short fg_color,short outline_color)
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
            DEV_WRITE_LOG_ERROR("get_glyph failed");
            continue;
        }

		if(draw_char)
		{
            starty = char_info.top;

			draw_rgb1555_char(area_w,
                    area_h,
                    char_info.w,
                    char_info.glyph_ori,
                    char_info.glyph_outline,
                    startx,
                    starty,
                    pdata,
                    data_size,
                    bg_color,
                    fg_color,
                    outline_color);
        }

        startx += char_info.w;
    }

	return true;
}


bool ceanic_freetype::get_glyph(FT_Face face,FT_ULong ch,FT_Glyph* orig,FT_Glyph* outline)
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
bool ceanic_freetype::draw_rgb1555_char(int area_w,int area_h,int font_pixel,FT_Glyph orig,FT_Glyph outline,int startx,int starty,unsigned char* buf,int size,short bg_color,short fg_color,short outline_color)
{
	FT_Bitmap * orig_bitmap = &((FT_BitmapGlyph)orig)->bitmap; 
	FT_Bitmap * outline_bitmap = &((FT_BitmapGlyph)outline)->bitmap; 
	unsigned char *pdata;

	int h_offset = (outline_bitmap->width - orig_bitmap->width) >> 1;
	int v_offset = (outline_bitmap->rows - orig_bitmap->rows) >> 1;
    int total_width = font_pixel;

    for(int i = 0; i < area_h; i++)
    {
        for (int  j = 0 ; j < total_width;  ++j)  
        {
			pdata = buf + i * area_w * 2 + (startx + j) * 2;//RGB1555,so need to *2

            *(short*)pdata = bg_color;

            if (j >= h_offset 
                    && j < orig_bitmap->width + h_offset 
                    && i >= starty + v_offset 
                    && i < orig_bitmap->rows + starty + v_offset)
            {
                if (orig_bitmap->buffer[(i-starty-v_offset)*orig_bitmap->width+j-h_offset] >= SHOW_LEVEL)
                {
                    *(short*)pdata = fg_color;
                }
                else if (outline_bitmap->buffer[(i-starty)*outline_bitmap->width+j] >= SHOW_LEVEL)
                {
                    *(short*)pdata = outline_color;
                }
            }
        }
    }
	
	return true;
}
