#ifndef ceanic_freetype_include_h
#define ceanic_freetype_include_h

#include "dev_std.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_STROKER_H
#include <string>
//#include <glib.h>

typedef struct
{
	wchar_t c;
	int font_size;
    FT_Face glyph_face;
    FT_Glyph glyph_ori;
    FT_Glyph glyph_outline;
    int top;
    int w;
}basic_char_t;

class ceanic_freetype
{
public:
	ceanic_freetype(const char* en_path,const char* zh_path);

	~ceanic_freetype();

	bool init();

	bool is_init();

	void release();

	bool get_width(const char* str,int font_pixel,int* w);

    bool get_max_width(const char* str,int font_pixel,int* w);

	bool show_string(const char* str,int area_w,int area_h,int font_pixel,unsigned char* pdata,int data_size,short bg_color,short fg_color,short outline_color);

	bool show_string_compare(const char* str_before,const char* str_now,int area_w,int area_h,int font_pixel,unsigned char* pdata,int data_size,short bg_clor,short fg_color,short outline_color);

private:
    bool get_glyph_char(wchar_t c,int font_size,basic_char_t* char_info);
	bool get_glyph(FT_Face face,FT_ULong ch,FT_Glyph* org,FT_Glyph* outline);

	bool draw_rgb1555_char(int area_w,
            int area_h,
            int font_pixel,
            FT_Glyph orig,
            FT_Glyph outline,
            int startx,
            int starty,
            unsigned char* buf,
            int size,
            short bg_color,
            short fg_color,
            short outline_color);

	bool get_basic_char(wchar_t c,int font_size,basic_char_t& char_info);

private:
	bool m_inited;
	std::string m_en_path;
	std::string m_zh_path;
	FT_Library  m_ft_lib;
	FT_Face		m_ft_face_en;	
	FT_Face		m_ft_face_zh;
};

#endif

