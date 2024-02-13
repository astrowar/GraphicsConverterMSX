#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <SDL.h>
typedef struct Paramameters__{
    float compression;
    float black; // black preservation
    float dithering; // dithering
    int colorDiffMethod ; // sRGB, Ciee ou YUV 
  
    //initializator 
    Paramameters__(){
        compression = 0.2f;
        black = 0.2f;
        dithering = 1.0f;
        colorDiffMethod = 0;
 
    }
} Paramameters_t;

void convert_to( SDL_Surface *surface_in,  SDL_Surface *surface_to,   Paramameters_t* p );

#endif