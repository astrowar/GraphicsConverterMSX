
#include <SDL.h>

class ColorTuple{
   public:
    int r;
    int g;
    int b;
    float labColor[3] ;
    bool has_lab = false ;
 
    ColorTuple(   );
    ColorTuple( int r_in, int g_in, int b_in );
    // indexator for hash
    bool operator<(const ColorTuple& other) const;

    ColorTuple  operator-(const ColorTuple& other) const;
 

    //get Lab color
    void get_lab( float lab_out[3] ) ;

    //get color difference
    float colorDiffCiee(   ColorTuple &other ) ; 
    float colorDiffRGB(   ColorTuple &other ) ;   
    float colorDiffYUV( ColorTuple &other) ;

    bool isDull();   
 
};


class ColorTupleInt{
  public:
    int  r;
    int  g;
    int  b;

    ColorTupleInt( int r_in, int g_in, int b_in );
    ColorTupleInt(  );
    ColorTupleInt  operator-(const ColorTupleInt& other) const;
    ColorTupleInt  operator+(const ColorTupleInt& other) const;
    ColorTupleInt  frac16(int i) const;
};

  