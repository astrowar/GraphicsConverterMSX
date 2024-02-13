#include <SDL.h>
#include <algorithm>

#include <stdio.h>
#include <cmath>
#include <omp.h>

#include "parameters.h"
#include "colorCiee.hpp"
#include <filesystem>

 
#define BYTE_TO_BINARY(byte)  \
  ((byte) & 0x80 ? '1' : '0'), \
  ((byte) & 0x40 ? '1' : '0'), \
  ((byte) & 0x20 ? '1' : '0'), \
  ((byte) & 0x10 ? '1' : '0'), \
  ((byte) & 0x08 ? '1' : '0'), \
  ((byte) & 0x04 ? '1' : '0'), \
  ((byte) & 0x02 ? '1' : '0'), \
  ((byte) & 0x01 ? '1' : '0') 


unsigned char    msxdump[6144 * 2];

typedef struct _tile_generator {
    unsigned char  tile[8]  ; // 0 or 1 for each pixel
}tile_generator_t;

// typedef struct _tile_colour {
//     unsigned char   color[8][2]; // 2 colors by row 
// }tile_colour_t;


typedef struct _tile_raw {
    unsigned char  tile[8]  ; // 0 or 1 for each pixel
    unsigned char  color[8] ; // 16/16 colors  
}tile_raw_t;



#define TILES_NUM_ROWS   24
#define TILES_NUM_COLS   32

#define TILES_MAX_TABLE   256
#define TILES_NUM_TABLE   3


class ErrorLine{
  public :
  float r[256];
  float g[256];
  float b[256];
  ErrorLine(){
      for(int i = 0; i < 256; i++)
      {
          r[i] = 0;
          g[i] = 0;
          b[i] = 0;
      }
  }
} ;

 

class MSXLineFrag{
    public:
        unsigned char  tile   ; // 0 or 1 for each pixel
        unsigned char  color_a; // 2 colors by row
       unsigned  char  color_b; // 2 colors by row
    MSXLineFrag( unsigned  char  _tile,  unsigned char  _color_a, unsigned char  _color_b){
        this->tile = _tile;
        this->color_a = _color_a;
        this->color_b = _color_b; 
     }
     MSXLineFrag(){
        this->tile = 0;
        this->color_a = 0;
        this->color_b = 0;
     }
};

class MSXLine {
  public :
  MSXLineFrag frags[32]; //each line is 32 lines frags
    MSXLine()
    {
        for(int i = 0; i < 32; i++)
        {
            frags[i] = MSXLineFrag(0,0,0);
        }
    }

};


class MSXLineFragMinimization {
  public :
    MSXLineFrag  line;
    ColorTupleInt next_line_residual_min[10]; 
    ColorTupleInt horizontal_pixel_residual;
    float error;
    MSXLineFragMinimization(  ){
        this->line = MSXLineFrag(0,0,0);
        for(int i = 0; i < 10; i++)
        {
            next_line_residual_min[i] = ColorTupleInt(0,0,0);
        }
        this->horizontal_pixel_residual = ColorTupleInt(0,0,0);
        this->error = 1e40;

    } 
};

unsigned char      msx_tile_name[TILES_NUM_ROWS][TILES_NUM_COLS];
tile_generator_t    msx_tile_generator[TILES_NUM_TABLE][TILES_MAX_TABLE];
 

tile_raw_t screen_tiles[TILES_NUM_ROWS][TILES_NUM_COLS];

unsigned char  Palette_msx0[] = { 
    0,0,0,              // Transparent
    0,0,0,              // Black
    36,219,36,          // Medium Green
    109,255,109,        // Light Green
    36,36,255,          // Dark Blue
    73,109,255,         // Light Blue
    182,36,36,          // Dark Red
    73,219,255,         // Cyan
    255,36,36,          // Medium Red
    255,109,109,        // Light Red
    219,219,36,         // Dark Yellow
    219,219,146,        // Light Yellow
    36,146,36,          // Dark Green
    219,73,182,         // Magenta
    182,182,182,        // Grey
    255,255,255  };     // White

    unsigned char Palette_msx1[]={
    0x00,0x00,0x00,      // Transparent
    0x01,0x01,0x01,      // Black
    0x3e,0xb8,0x49,      // Medium Green
    0x74,0xd0,0x7d,      // Light Green
    0x59,0x55,0xe0,      // Dark Blue
    0x80,0x76,0xf1,      // Light Blue
    0xb9,0x5e,0x51,      // Dark Red
    0x65,0xdb,0xef,      // Cyan
    0xdb,0x65,0x59,      // Medium Red
    0xff,0x89,0x7d,      // Light Red
    0xcc,0xc3,0x5e,      // Dark Yellow
    0xde,0xd0,0x87,      // Light Yellow
    0x3a,0xa2,0x41,      // Dark Green
    0xb7,0x66,0xb5,      // Magenta
    0xcc,0xcc,0xcc,      // Grey
    0xff,0xff,0xff};     // White


float colorDiffCiee( Uint8 r_1, Uint8 g_1, Uint8 b_1, Uint8 r_2, Uint8 g_2, Uint8 b_2 );
void convert_to_ciee( Uint8 r_1, Uint8 g_1, Uint8 b_1,  float lab_out[3] );
Uint32 get_pixel(SDL_Surface *surface, int x, int y);

float colorDiffSRGB( Uint8 r_1, Uint8 g_1, Uint8 b_1, Uint8 r_2, Uint8 g_2, Uint8 b_2 ){
    float r1 = (float)r_1 / 255.0f;
    float g1 = (float)g_1 / 255.0f;
    float b1 = (float)b_1 / 255.0f;
    float r2 = (float)r_2 / 255.0f;
    float g2 = (float)g_2 / 255.0f;
    float b2 = (float)b_2 / 255.0f;
    float dR = r1 - r2;
    float dG = g1 - g2;
    float dB = b1 - b2;
    return  std::sqrt(dR * dR + dG * dG + dB * dB);
}

float get_color_distance( ColorTuple c_target,  ColorTuple   color_x,    int colorDiffMethod )
{
     ColorTuple   color_dst = color_x;
     float error = 0;

    if ( colorDiffMethod == 0 )
    {
       error = c_target.colorDiffRGB( color_dst );
    }

    else if (colorDiffMethod ==1 )
    {
       error = c_target.colorDiffCiee( color_dst );
    }
    else {
        error = c_target.colorDiffYUV(   color_dst );
    }
    return error;

}

void get_color_distance( ColorTuple c_target, int  residual_color_in[3], ColorTuple   color_x,    float &error, int   residual_color_out[3] , bool useResidual, int colorDiffMethod )
{

    Uint8 r_1 ,g_1,b_1;
    ColorTuple   color_dst;

    if (useResidual){   
       r_1 = std::min( 255, std::max( 0, color_x.r - residual_color_in[0] ) );
       g_1 =  std::min( 255, std::max( 0, color_x.g - residual_color_in[1] ) );
       b_1 =  std::min( 255, std::max( 0, color_x.b - residual_color_in[2] ) );
       color_dst = ColorTuple( r_1, g_1, b_1 );
    }
    else{

          color_dst = color_x;
    }
    
    if ( colorDiffMethod == 0 )
    {
       error = c_target.colorDiffRGB( color_dst );
    }

    else if (colorDiffMethod ==1 )
    {
       error = c_target.colorDiffCiee( color_dst );
    }
    else {
        error = c_target.colorDiffYUV(   color_dst );
    }

    residual_color_out[0]  = (int)c_target.r - ( r_1 );
    residual_color_out[1]  = (int)c_target.g - ( g_1 );
    residual_color_out[2]  = (int)c_target.b - ( b_1 );
  
   
}

 

float compute_color_error_only( ColorTuple  line[8] ,int yi,  ColorTuple msx_color0_tuple  ,   ColorTuple msx_color1_tuple,  unsigned char local_patten[8] , Paramameters_t* p)
{ 
 
            float err = 0 ;
            int residual_color[3] = {0,0,0}; 
            int residual_color_out[3] = {0,0,0}; 
            bool useResidual = true; 
        

            for(int k= 0 ;k < 8;++k){
                int bit_value = local_patten[k];
                float err_ = 0;
                if (bit_value == 0)  get_color_distance( line[k] ,residual_color, msx_color0_tuple, err_, residual_color_out , useResidual , p->colorDiffMethod );   
                else                get_color_distance( line[k] ,residual_color, msx_color1_tuple, err_, residual_color_out , useResidual , p->colorDiffMethod); 

                if (line[k].r< 30 && line[k].g <30  && line[k].b < 30 ) err_ *=  (1.0 + 4*p->black);
                if (k>0){
                        if (local_patten[k] != local_patten[k-1] ) err_ *= (1.0 + p->compression);                         
                 }
                err += err_;
                residual_color[ 0] = residual_color_out[ 0];
                residual_color[ 1] = residual_color_out[ 1];
                residual_color[ 2] = residual_color_out[ 2]; 
            }
   return err;
            
}

float compute_color_error(ColorTuple  line[8] ,int yi,  ColorTuple msx_color0_tuple  ,   ColorTuple msx_color1_tuple  ,  ColorTuple msx_color_mix_tuple  , unsigned char local_patten[8] , Paramameters_t* p )
{
    int residual_color[3] = {0,0,0}; 
    float local_error = 0.0;
  
 
 
             for(int k =0 ; k < 8 ;++k)
             {    float err0 = 0;
                  float err1 = 0;
                  float err3 = 0;
                   int residual_color0[3] = {0,0,0};
                   int residual_color1[3] = {0,0,0};
                   int residual_color_half[3] = {0,0,0};
                   bool useResidual =   ( (k + yi)%2 == 1 ) ;
                   get_color_distance( line[k] ,residual_color, msx_color0_tuple, err0, residual_color0 , useResidual, p->colorDiffMethod );   
                   get_color_distance( line[k] ,residual_color, msx_color1_tuple, err1, residual_color1 , useResidual , p->colorDiffMethod);
                   get_color_distance( line[k] ,residual_color, msx_color_mix_tuple, err3, residual_color_half , useResidual, p->colorDiffMethod ); 

                  int  patten_bit_3 =  (k + yi ) %2 ;

                   //if line[k] an black color ?
                     if (line[k].r< 30 && line[k].g <30  && line[k].b < 30 )
                     {
                          err0 *=  (1.0 + 4*p->black);
                          err1 *=  (1.0 + 4*p->black);
                          err3 *=  (1.0 + 4*p->black);
                     }

                     //compression
                     // err3 *= (1.0 + p->compression);
              
                     if (k>0){
                        if (local_patten[k-1] != 0) err0 *= (1.0 + p->compression);
                        if (local_patten[k-1] != 1) err1 *= (1.0 + p->compression);
                        if (local_patten[k-1] != patten_bit_3 ) err3 *= (1.0 + p->compression);
                     } 

                   if (err0 <= err1 && err0 <= err3 ){
                        local_patten[k] = 0;
                        residual_color[0] =  residual_color0 [0];
                        residual_color[1] =  residual_color0 [1];
                        residual_color[2] =  residual_color0 [2];
                        local_error += err0;
                   }
                   if (err1 <= err0 && err1 <= err3 ){
                        local_patten[k] = 1;
                        residual_color[0] =  residual_color1 [0];
                        residual_color[1] =  residual_color1 [1];
                        residual_color[2] =  residual_color1 [2];
                        local_error += err1;
                   }                  
                   if (err3 <= err0 && err3 <= err1 )
                   {
                        local_patten[k] =  (k + yi ) %2 ;
                        residual_color[0] =  residual_color_half [0];
                        residual_color[1] =  residual_color_half [1];
                        residual_color[2] =  residual_color_half [2];
                        local_error += err3;
                   }

                   residual_color[0] = residual_color [0];
                   residual_color[1] = residual_color [1];
                   residual_color[2] = residual_color [2];
                  
             }
            return local_error;
}

//get 8 colors from SDL and convert 
unsigned char   get_closest_color_line(  SDL_Color  line_rgb[8] , int yi,   unsigned char &patten_out,      Paramameters_t* p ) 
{
    float min_color_dist = 99999999.0f;
    int min_color0 = 0;
    int min_color1 = 0;
    unsigned char min_patten =0;         

   ColorTuple line[8];
    for(int k =0 ; k < 8 ;++k)
    {
        line[k] = ColorTuple( line_rgb[k].r, line_rgb[k].g, line_rgb[k].b );
    }

    //scan all colors and get the closest
    for(int c0 = 1 ; c0 < 16 ; c0++)
    {
        for(int c1 = c0 ; c1 < 16 ; c1++)
        {

            //get labCiie  color1 and color2
            SDL_Color msx_color0  = { Palette_msx1[c0 * 3 + 0], Palette_msx1[c0 * 3 + 1], Palette_msx1[c0 * 3 + 2], 0 };
            SDL_Color msx_color1 =  { Palette_msx1[c1 * 3 + 0], Palette_msx1[c1 * 3 + 1], Palette_msx1[c1 * 3 + 2], 0 };
            SDL_Color msx_color_mix =  {  (unsigned char )((Palette_msx1[c0 * 3 + 0]+Palette_msx1[c1 * 3 + 0])/2) , 
                                 (unsigned char )((Palette_msx1[c0 * 3 + 1]+Palette_msx1[c1 * 3 + 1])/2) , 
                                (unsigned char ) ((Palette_msx1[c0 * 3 + 2]+Palette_msx1[c1 * 3 + 2])/2) , 0 };

            ColorTuple msx_color0_tuple( msx_color0.r, msx_color0.g, msx_color0.b );
            ColorTuple msx_color1_tuple( msx_color1.r, msx_color1.g, msx_color1.b );
            ColorTuple msx_color_mix_tuple( msx_color_mix.r, msx_color_mix.g, msx_color_mix.b );



            int residual_color[3] = {0,0,0}; 
            unsigned char local_patten[8] = {0,0,0,0,0,0,0,0};
            float local_error = compute_color_error( line , yi, msx_color0_tuple, msx_color1_tuple, msx_color_mix_tuple , local_patten , p );    
 

            //check if is min
            if (local_error < min_color_dist)
                {
                    min_color_dist = local_error;
                    min_color0 = c0;
                    min_color1 = c1;
                    min_patten = 0;
                    for(int k =0 ; k < 8 ;++k)
                    {
                        min_patten |= (local_patten[k] << (7-k));
                    }                    
                } 
        }
    }

    //store the result in the output
    patten_out = min_patten;
    return  (min_color0 << 4) | min_color1;
 
    
}

void generate_tile( SDL_Surface *surface_src,  int ti, int tj,  tile_raw_t &t ,Paramameters_t* p   ){
            for(int yi = 0 ; yi< 8;yi++) //each line in this tile 
            {
                //compute patten for this line
                int x = tj*8;
                int y = ti*8 + yi;

                SDL_Color line[8];
                for(int xi = 0 ; xi< 8;xi++)
                {
                  
                  SDL_GetRGB( get_pixel(surface_src, x + xi, y), surface_src->format, &line[xi].r, &line[xi].g, &line[xi].b);
                }
               t.color[yi] =  get_closest_color_line( line, yi, t.tile[yi],   p );
            }
}

float measure_line_error( int yi, SDL_Color line_rgb[8][8] , tile_raw_t &t , Paramameters_t* p  )
{
            unsigned char  color_0 = (t.color[yi] && 0xf0) >> 4;
            unsigned char color_1 = (t.color[yi]  && 0x0f);
            unsigned char line_patten[8] = {0,0,0,0,0,0,0,0};
            for(int k =0 ; k<  8 ;++k) line_patten[k] = (t.tile[yi] & (1 << (7-k))) >> (7-k);

            SDL_Color msx_color0  = { Palette_msx1[color_0 * 3 + 0], Palette_msx1[color_0 * 3 + 1], Palette_msx1[color_0 * 3 + 2], 0 };
            SDL_Color msx_color1 =  { Palette_msx1[color_1 * 3 + 0], Palette_msx1[color_1 * 3 + 1], Palette_msx1[color_1 * 3 + 2], 0 };
 
            ColorTuple msx_color0_tuple( msx_color0.r, msx_color0.g, msx_color0.b );
            ColorTuple msx_color1_tuple( msx_color1.r, msx_color1.g, msx_color1.b );

            ColorTuple line[8];
            for(int k =0 ; k < 8 ;++k)
            {
                line[k] = ColorTuple( line_rgb [k][yi].r, line_rgb [k][yi].g, line_rgb [k][yi].b );
            }

            return  compute_color_error_only( line, yi, msx_color0_tuple, msx_color1_tuple, line_patten , p );
}

float  measure_tile_error(  SDL_Color line[8][8] , tile_raw_t &t , Paramameters_t* p  )
{
     float err =0;
     for(  int yi = 0 ; yi< 8;yi++) //each line in this tile 
     { 
            err +=  measure_line_error( yi, line , t , p  );  
     }
     return err;
}

void optimize_tile( SDL_Surface *surface_src,  int ti, int tj,  tile_raw_t &t ,Paramameters_t* p   ){
     SDL_Color line[8][8];
    
     //fill line from colors
     for( int xi = 0 ; xi< 8;xi++)
     {
         for(  int yi = 0 ; yi< 8;yi++) //each line in this tile 
         {
             int x = tj*8 + xi;
             int y = ti*8 + yi;      
            SDL_GetRGB( get_pixel(surface_src, x, y), surface_src->format, &line[xi][yi].r, &line[xi][yi].g, &line[xi][yi].b);

         }
     }

   

    //  //choose one line to optimize
    //     int line_to_optimize = 4;
    //     float line_error =  measure_tile_error( line , t , p );

    //     //try change patten to previous line and chech error 
    //     float next_line_error = 0;
    //     {    int yi = line_to_optimize;
    //          int color_0 = t.color[yi][0];
    //          int color_1 = t.color[yi][1];
    //         unsigned char line_patten[8] = {0,0,0,0,0,0,0,0};
    //         for(int k =0 ; k<  8 ;++k) line_patten[k] = (t.tile[yi] & (1 << (7-k))) >> (7-k);
    //         return  compute_color_error_only( line[yi] , yi, color_0, color_1, line_patten , p )
    //     }
 

}


unsigned char bit_array_toChar( unsigned char  bit_array[8] )
{
    unsigned char c = 0;
    for(int i = 0; i < 8; i++)
    {
        c |= (bit_array[i] << (7-i));
    }
    return c;
}



int get_best_colos(int y_tile , int x_tile , SDL_Surface *surface_src, Paramameters_t* p  ){
 
    float c12_error_min = 1e99 ;
    unsigned char  c1_min = 0;
    unsigned char c2_min = 1; 
    unsigned char min_patten = 0;

        #pragma omp parallel  for
        for(int c12 = 0 ; c12 <256;++c12 )
        // for (  int  c1 = 0 ; c1< 16;++c1)
        //   for (  int   c2 = c1 ; c2< 16;++c2) 
          {

            int c1 = (c12 >> 4) & 0x0F;
            int c2 = c12 & 0x0F;
            if (c2 < c1) continue;
 
            float err_frag = 0 ;  
            for(int j = 0; j < 8; j++)
            for(int i = 0; i < 8; i++)
            { 
                int x = x_tile*8 + i;
                int y = y_tile*8 + j;
       

                //get pixel at x,y
                SDL_Color line_frag;
                SDL_GetRGB( get_pixel(surface_src, x, y), surface_src->format, &line_frag.r, &line_frag.g, &line_frag.b  );

                float  err_cc1 =  colorDiffCiee( line_frag.r, line_frag.g, line_frag.b, Palette_msx1[c1 * 3 + 0], Palette_msx1[c1 * 3 + 1], Palette_msx1[c1 * 3 + 2] );
                float  err_cc2 =  colorDiffCiee( line_frag.r, line_frag.g, line_frag.b, Palette_msx1[c2 * 3 + 0], Palette_msx1[c2 * 3 + 1], Palette_msx1[c2 * 3 + 2] );
                float  err_cc3 =  colorDiffCiee( line_frag.r, line_frag.g, line_frag.b, (Palette_msx1[c1 * 3 + 0] + Palette_msx1[c2 * 3 + 0])/2, (Palette_msx1[c1 * 3 + 1] + Palette_msx1[c2 * 3 + 1])/2, (Palette_msx1[c1 * 3 + 2] + Palette_msx1[c2 * 3 + 2])/2 );
                 
                if ((err_cc1 <= err_cc2) && (err_cc1 <= err_cc3)  )
                {
                    err_frag += err_cc1;
                }
                else if ((err_cc2 <= err_cc1) && (err_cc2 <= err_cc3)  )
                {
                    err_frag += err_cc2;
                } 
                else {
                    err_frag += err_cc3;
                }
            }
            #pragma omp critical
            if (err_frag < c12_error_min)
            {
                c12_error_min = err_frag; 
                c1_min = c1;
                c2_min = c2; 
            }
         
    }
    //printf("color pair %d %d [RGB %3i %3i %3i] [RGB %3i %3i %3i]  \n", c1_min, c2_min, Palette_msx1[c1_min * 3 + 0], Palette_msx1[c1_min * 3 + 1], Palette_msx1[c1_min * 3 + 2], Palette_msx1[c2_min * 3 + 0], Palette_msx1[c2_min * 3 + 1], Palette_msx1[c2_min * 3 + 2] ); 
    return  (c1_min << 4)  | c2_min;               
}


MSXLineFragMinimization generate_optimed_line_frag( int line, int x_tile , SDL_Surface *surface_src, Paramameters_t* p , ErrorLine &err_line_in , int sugestion_color  )
{

MSXLineFragMinimization result;

        int  err_init_frag_r = 0;
        int err_init_frag_g = 0;
        int err_init_frag_b = 0;
 

     SDL_Color line_frag[8];
      for(int  i = 0 ; i < 8; i++)
        {   int x = x_tile*8 + i;
       
            SDL_GetRGB( get_pixel(surface_src, x, line ), surface_src->format,  &line_frag[i].r, &line_frag[i].g, &line_frag[i].b );

        }

       float c12_error_min = 1e99 ;
       unsigned char  c1_min = 0;
       unsigned char c2_min = 1; 
       unsigned char min_patten = 0;
        ColorTupleInt hor_residual_min = ColorTupleInt( err_init_frag_r, err_init_frag_g, err_init_frag_b );
        ColorTupleInt next_line_residual_min[10]; 

        #pragma omp parallel  for
        for(int c12 = 0 ; c12 <256;++c12 )   
          {

            int c1 = ((c12 & 0xf0) >> 4) & 0x0F;
            int c2 = c12 & 0x0F;
            if (c2 < c1) continue;

            float err_compression_factor = 1.0;
            if (c12   ==  sugestion_color ) err_compression_factor = (1.0f - 0.9*p->compression);
   

            float err_frag = 0 ;  
            ColorTupleInt hor_residual = ColorTupleInt( err_init_frag_r, err_init_frag_g, err_init_frag_b );


            ColorTuple cc1 = ColorTuple( Palette_msx1[c1 * 3 + 0], Palette_msx1[c1 * 3 + 1], Palette_msx1[c1 * 3 + 2] );
            ColorTuple cc2 = ColorTuple( Palette_msx1[c2 * 3 + 0], Palette_msx1[c2 * 3 + 1], Palette_msx1[c2 * 3 + 2] );
            ColorTuple cc_mix = ColorTuple( (cc1.r + cc2.r)/2, (cc1.g + cc2.g)/2, (cc1.b + cc2.b)/2 );


            unsigned char local_patten[8] = {0,0,0,0,0,0,0,0};
            ColorTupleInt next_line_residual[10]; 

            for(int i = 0; i < 8; i++)
            { 
                int x = x_tile*8 + i;

                int acc_r = (   hor_residual.r + err_line_in.r[x] )  ; 
                int acc_g = (  hor_residual.g + err_line_in.g[x] )  ;
                int acc_b = (  hor_residual.b + err_line_in.b[x] )  ;
                acc_r = trunc(  float(acc_r) * (  p->dithering));
                acc_g = trunc(  float(acc_g) * (  p->dithering));
                acc_b = trunc(  float(acc_b) * (  p->dithering));
              
                  acc_r += (line_frag[i].r   )  ; 
                  acc_g += (line_frag[i].g  )  ;
                  acc_b += (line_frag[i].b  )  ; 
     

                acc_r = std::min( 255, std::max( 0, acc_r ) );
                acc_g = std::min( 255, std::max( 0, acc_g ) );
                acc_b = std::min( 255, std::max( 0, acc_b ) );

                ColorTuple color_target = ColorTuple( acc_r, acc_g, acc_b );
                ColorTupleInt  rgb_diff = ColorTupleInt( 0, 0, 0 );
 

                float  err_cc1 =  get_color_distance( color_target ,  cc1 , p->colorDiffMethod );
                float  err_cc2 =  get_color_distance( color_target ,  cc2 , p->colorDiffMethod );
                float  err_cc3 =  get_color_distance( color_target ,  cc_mix , p->colorDiffMethod );
 
                if (p->dithering < 0.5)   { 
                   
                    err_cc3  *=  2.0;
                }
                

 

                if ((err_cc1 <= err_cc2) && (err_cc1 <= err_cc3)  )
                {
                    local_patten[i] = 0;
                    err_frag += err_cc1;
                    rgb_diff = ColorTupleInt( color_target.r -   cc1.r, color_target.g -   cc1.g, color_target.b -   cc1.b );
      
                }
                else    if ((err_cc2 <= err_cc1) && (err_cc2 <= err_cc3)  )
                {
                    local_patten[i] = 1;
                    err_frag += err_cc2;
                    rgb_diff = ColorTupleInt( color_target.r -   cc2.r, color_target.g -   cc2.g, color_target.b -   cc2.b ); 
                } 
                else {
                     local_patten[i] = (line + i )%2;
                     err_frag += err_cc3;
                     rgb_diff = ColorTupleInt( color_target.r -   cc_mix.r, color_target.g -   cc_mix.g, color_target.b -   cc_mix.b ); 
                }


                 hor_residual = rgb_diff.frac16( 7  );
                 next_line_residual[i] = next_line_residual[i]  +  rgb_diff.frac16(3);
                 next_line_residual[i+1] = next_line_residual[i+1] + rgb_diff.frac16(5);
                 next_line_residual[i+2] = next_line_residual[i+2]  + rgb_diff.frac16(1);
             }

             err_frag*= err_compression_factor;

              #pragma omp critical
              if (err_frag < c12_error_min)
              {
                  c12_error_min = err_frag; 
                  min_patten = bit_array_toChar( local_patten );

                  result.horizontal_pixel_residual = hor_residual;
                  result.line = MSXLineFrag(min_patten ,c1,c2);
                  result.error =  result.error + c12_error_min;

                  for(int j = 0; j < 10; j++)                  
                  {
                      result.next_line_residual_min[j] = next_line_residual[j];
                  }
              }
          } 
      
     
         

        return  result;
}







MSXLine generate_optimed_line( int line, SDL_Surface *surface_src,  Paramameters_t* p , ErrorLine &err_line_in, ErrorLine &err_line_out ,  int sugestion_color_nibbles[TILES_NUM_ROWS][TILES_NUM_COLS] )
{
    MSXLine msx_line;
    int  err_init_frag_r = 0;
    int err_init_frag_g = 0;
    int err_init_frag_b = 0;

    //clean 
    for(int  i = 0 ; i < 256; i++)
    {
        err_line_out.r[i] = 0;
        err_line_out.g[i] = 0;
        err_line_out.b[i] = 0;
    } 

    for(int x_tile  = 0 ; x_tile < 32 ; ++x_tile)
    { 
      int sugestion_color_tile = sugestion_color_nibbles[line/8][x_tile];

      MSXLineFragMinimization result = generate_optimed_line_frag( line, x_tile , surface_src, p , err_line_in ,sugestion_color_tile );
      msx_line.frags[x_tile] =  result.line;
      ColorTupleInt hor_residual_min = ColorTupleInt( err_init_frag_r, err_init_frag_g, err_init_frag_b );
      ColorTupleInt next_line_residual_min[10];  

        //add frag line down to global line error 
        for(int i = 0   ; i<  10  ;i++)
        {
            int x = x_tile*8 + (i-1);
            if (x >=0 && x < 256)
            {       
                    err_line_out.r[x] = err_line_out.r[x]  +  result.next_line_residual_min[i].r;
                    err_line_out.g[x] = err_line_out.g[x]  +  result.next_line_residual_min[i].g;
                    err_line_out.b[x] = err_line_out.b[x]  +  result.next_line_residual_min[i].b;                   
            } 
        }
    }
    return msx_line;
}

 
void generate_tiles_err_acc( SDL_Surface *surface_src, tile_raw_t  stiles[TILES_NUM_ROWS][TILES_NUM_COLS] , Paramameters_t* p )
{ 
    ErrorLine err_line_in ;
    ErrorLine err_line_out ;

    int sugestion_color_nibbles[TILES_NUM_ROWS][TILES_NUM_COLS];
    for (int i = 0; i < TILES_NUM_ROWS; i++)
    {
        for (int j = 0; j < TILES_NUM_COLS; j++)
        {
            sugestion_color_nibbles[i][j] = get_best_colos(i, j, surface_src, p);
        }
    }

    for(int  line = 0 ;line < 192;++line )
    { 
    
       MSXLine msx_line = generate_optimed_line( line,  surface_src,    p , err_line_in, err_line_out , sugestion_color_nibbles ); 

       //write to stiles
       int tile_row = line / 8;
       int tile_sub_row = line % 8;
       for (int x = 0 ;  x < 32 ; ++x){
           stiles[tile_row][x].tile[tile_sub_row] = msx_line.frags[x].tile;
           unsigned char color_nibble = (msx_line.frags[x].color_a << 4) | msx_line.frags[x].color_b;
           stiles[tile_row][x].color[tile_sub_row]= color_nibble ; 
           //stiles[tile_row][x].color[tile_sub_row][0] = msx_line.frags[x].color_a;
          // stiles[tile_row][x].color[tile_sub_row][1] = msx_line.frags[x].color_b;
       }
       std::swap(err_line_in, err_line_out);
    }   

}


void generate_tiles( SDL_Surface *surface_src, tile_raw_t  stiles[TILES_NUM_ROWS][TILES_NUM_COLS] , Paramameters_t* p )
{   

    generate_tiles_err_acc( surface_src, stiles, p );
    return ;
    
      //for(int ti = 0; ti < TILES_NUM_ROWS; ti++) 
    //    for(int tj = 0; tj < TILES_NUM_COLS; tj++)
    //convert surface_src to 8x8 tiles
    #pragma omp parallel for  
    for(int tij = 0 ;tij <  TILES_NUM_ROWS * TILES_NUM_COLS ; ++tij)
        {
            int ti = tij / TILES_NUM_COLS;
            int tj = tij % TILES_NUM_COLS;

            tile_raw_t &t = stiles[ti][tj];
            //zero it 
            for(int i = 0; i < 8; i++)
            {
                t.tile[i] = 0;
                t.color[i] = 0;
                
            }
            generate_tile( surface_src, ti, tj, t , p );
        }
   
}

  void set_pixel(SDL_Surface *surface, int x, int y,  unsigned char r, unsigned char g, unsigned char b)
{
  Uint32 * const target_pixel = (Uint32 *) ((Uint8 *) surface->pixels
                                             + y * surface->pitch
                                             + x * surface->format->BytesPerPixel);
    *target_pixel = SDL_MapRGB(surface->format, r, g, b);
}
     

    

void write_to_sdl_surface(   SDL_Surface *surface_dst  , tile_raw_t  stiles[TILES_NUM_ROWS][TILES_NUM_COLS]  )
{
   //use pallete
   for(int ti = 0; ti < TILES_NUM_ROWS; ti++)
   {
       for(int tj = 0; tj < TILES_NUM_COLS; tj++)
       {
           tile_raw_t &t = stiles[ti][tj];
           for(int i = 0; i < 8; i++)
           {
               for(int j = 0; j < 8; j++)
               {
                   int color_0 = (t.color[i] & 0xf0) >> 4;
                   int color_1 = (t.color[i] & 0x0f); 
                   int pixel = t.tile[i] & (1 << (7-j));
                   int x = tj*8 + j;
                    int y = ti*8 + i;

                    SDL_Color c0 = { Palette_msx1[color_0 * 3 + 0], Palette_msx1[color_0 * 3 + 1], Palette_msx1[color_0 * 3 + 2], 0 };
                    SDL_Color c1 = { Palette_msx1[color_1 * 3 + 0], Palette_msx1[color_1 * 3 + 1], Palette_msx1[color_1 * 3 + 2], 0 };

                   if (pixel)
                   {
                        set_pixel(surface_dst, x, y, c1.r, c1.g, c1.b);
                   }
                   else
                   {
                        set_pixel(surface_dst, x, y, c0.r, c0.g, c0.b);
                   }
               }
           }
       }
   }

}

void convert_to( SDL_Surface *surface_in,  SDL_Surface *surface_to,   Paramameters_t* p )
{

    //surface to must be 256x192
    if (surface_to->w != 256 || surface_to->h != 192)
    {
        printf("Error: surface_to must be 256x192\n");
        return;
    }

    printf( "surface_in->w = %d\n", surface_in->w);
    printf( "surface_in->h = %d\n", surface_in->h);

   //print  surface_to mask RGB
    printf("surface_to->format->Rmask = 0x%08x\n", surface_to->format->Rmask);
    printf("surface_to->format->Gmask = 0x%08x\n", surface_to->format->Gmask);
    printf("surface_to->format->Bmask = 0x%08x\n", surface_to->format->Bmask); 

     
    //convert surface_in to small surface
    SDL_Surface *surface_in_small = SDL_CreateRGBSurface(0, 256, 192, 32, 0,0,0,0);
    
     
    //adjust aspect ratio of Rect_dst to fit surface_in
    float scale_factor = std::max(  (float)surface_in->w / (float)256, (float)surface_in->h / (float)192 );
    while ( (scale_factor * surface_in->w > 256) || (scale_factor *  surface_in->h > 192))
    {
        scale_factor *= 0.95f;
        if (scale_factor < 0.01) break; 
    }
    SDL_Rect Rect_dst = { 0, 0,  (int)(scale_factor * surface_in->w), (int)(scale_factor * surface_in->h) };   
       

    printf("Rect_dst.w = %d\n", Rect_dst.w);
    printf("Rect_dst.h = %d\n", Rect_dst.h);

    SDL_BlitScaled(surface_in, NULL, surface_in_small, &Rect_dst);

    generate_tiles(surface_in_small, screen_tiles, p );
    write_to_sdl_surface(surface_to,  screen_tiles );
  

    //proxy, just copy the surfaces
   // SDL_BlitSurface( , NULL, surface_to, NULL);
}


void save_tiles(std::filesystem::path p){
    
 
   
    char filename_c[2000];
    wcstombs(filename_c, p.c_str(), 2000);


    int i,j;
    unsigned char MsxHeader[7]= {0xFE,0x00,0x00,0xFF,0x37,0x00,0x00};
    
    
    FILE *f = fopen(filename_c, "wb");
    if (f==NULL)
       return ;
    
    for (i=0; i<7; i++) {
        fputc(MsxHeader[i], f);
    }

  
    for( int  yt =0 ; yt < 24; ++yt){
        for(int xt= 0 ; xt < 32; ++xt){ 
        for (i=0;i<=7;i++){
            fputc(screen_tiles[yt][xt].tile[i], f);
        }
        } 
    }  
  

    // for (i=0;i<=6143;i++)
    // {
    //     fputc(msxdump[i], f);
    // }

    for (j=0;j<=2;j++)
    {
        for (i=0;i<=255;i++)
        {
            fputc((char)i, f);
        }
    }
    
    for (i=0;i<=1279;i++)
    {
        fputc(0, f);
    }

    for (i=0;i<=6143;i++)
    {
        //fputc(msxdump[i+6144], f);
    }

       for( int  yt =0 ; yt < 24; ++yt){
        for(int xt= 0 ; xt < 32; ++xt){ 
         auto t = screen_tiles[yt][xt] ;
         for (int k=0;k<=7;k++)   
         {               
                unsigned char  c12 = t.color[k] ;
                //swap nibbles 
                 c12 =  (c12 & 0x0F) << 4 | (c12 & 0xF0) >> 4;
                //if (k ==0 ) printf  ("c12 = %d\n", c12);

                fputc( c12  , f); 
         } 
        }  
    }
    fclose(f);
}


 