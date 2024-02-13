#ifndef COLOR_ADJUST_H
#define COLOR_ADJUST_H

#include <SDL.h>
#include <filesystem>

typedef struct adjust_parameter{
    float hue;
    float saturation;
    float lightness;
    float flatness;
} adjust_parameter_t;

#define NUMCOLORSADJ 8

void set_pixel(SDL_Surface *surface, int x, int y,  unsigned char r, unsigned char g, unsigned char b);
Uint32 get_pixel(SDL_Surface *surface, int x, int y);

void color_adjust_surface(SDL_Surface *surface_in,  SDL_Surface *surface_to,   adjust_parameter_t p[NUMCOLORSADJ] );
bool color_adjust( adjust_parameter_t cadj[NUMCOLORSADJ] );
void save_tiles( std::filesystem::path   filename);
 

 void  RGBtoHSL(Uint8 r_in, Uint8 g_in, Uint8 b_in, float &h_out, float &s_out, float &l_out ) ;
 void  HSLtoRGB( float h_in, float s_in, float l_in, Uint8 &r_out, Uint8 &g_out, Uint8 &b_out );
 #endif