#include <cmath>
#include <SDL.h>
#include <map>
#include <array>

#include "colorCiee.hpp"

namespace color
{
    // Convert sRGB color in [0..255]^3 to CIE-XYZ.
    void srgb_to_xyz(const unsigned char* srgb, float* xyz)
    {
        float rgb[3] = { 0 };
        for (int channel = 0; channel < 3; ++channel)
        {
            float c = srgb[channel] / 255.0;
            if (c > 0.04045)
            {
                c = std::pow((c + 0.055) / 1.055, 2.4);
            }
            else
            {
                c = c / 12.92;
            }
            rgb[channel] = 100 * c;
        }
        xyz[0] = rgb[0] * 0.4124 + rgb[1] * 0.3576 + rgb[2] * 0.1805;
        xyz[1] = rgb[0] * 0.2126 + rgb[1] * 0.7152 + rgb[2] * 0.0722;
        xyz[2] = rgb[0] * 0.0193 + rgb[1] * 0.1192 + rgb[2] * 0.9505;
    }

    // Convert CIE-XYZ color to CIE-Lab.
    void xyz_to_lab(const float* xyz, float* lab)
    {
        const float delta = 6.0 / 29;
        const float reference[3] = { 95.047, 100.0, 108.883 }; // D65 (2 deg)
        float f[3] = { 0 };
        for (int channel = 0; channel < 3; ++channel)
        {
            float c = xyz[channel] / reference[channel];
            if (c > std::pow(delta, 3))
            {
                f[channel] = pow(c, 1.0 / 3);
            }
            else
            {
                f[channel] = c / (3 * delta * delta) + 4.0 / 29;
            }
        }
        lab[0] = 116 * f[1] - 16;
        lab[1] = 500 * (f[0] - f[1]);
        lab[2] = 200 * (f[1] - f[2]);
    }

    // Return CIEDE2000 color difference.
    float diff_de00(const float* lab1, const float* lab2)
    {
        const float kL = 1, kC = 1, kH = 1;
        float L1 = lab1[0], a1 = lab1[1], b1 = lab1[2];
        float L2 = lab2[0], a2 = lab2[1], b2 = lab2[2];

        float C1 = std::sqrt(a1 * a1 + b1 * b1);
        float C2 = std::sqrt(a2 * a2 + b2 * b2);
        float Cbar7 = std::pow((C1 + C2) / 2, 7);
        float G = 0.5 * (1 - std::sqrt(Cbar7 / (Cbar7 + std::pow(25.0, 7))));
        float ap1 = (1 + G) * a1;
        float ap2 = (1 + G) * a2;
        float Cp1 = std::sqrt(ap1 * ap1 + b1 * b1);
        float Cp2 = std::sqrt(ap2 * ap2 + b2 * b2);
        float hp1 = std::atan2(b1, ap1);
        const float pi = 3.141592653589793;
        if (hp1 < 0)
        {
            hp1 += 2 * pi;
        }
        float hp2 = std::atan2(b2, ap2);
        if (hp2 < 0)
        {
            hp2 += 2 * pi;
        }

        float dLp = L2 - L1;
        float dCp = Cp2 - Cp1;
        float dh = hp2 - hp1;
        if (dh > pi)
        {
            dh -= 2 * pi;
        }
        else if (dh < -pi)
        {
            dh += 2 * pi;
        }
        float dHp = 2 * std::sqrt(Cp1 * Cp2) * std::sin(dh / 2);

        float Lpbar = (L1 + L2) / 2;
        float Cpbar = (Cp1 + Cp2) / 2;
        float Hpbar = (hp1 + hp2) / 2;
        if (std::abs(hp1 - hp2) > pi)
        {
            Hpbar -= pi;
        }
        if (Hpbar < 0)
        {
            Hpbar += 2 * pi;
        }
        if (Cp1 * Cp2 == 0)
        {
            Hpbar = hp1 + hp2;
        }
        Hpbar *= 180 / pi;

        float T = (1
            - 0.17 * std::cos(pi / 180 * (Hpbar - 30))
            + 0.24 * std::cos(pi / 180 * (2 * Hpbar))
            + 0.32 * std::cos(pi / 180 * (3 * Hpbar + 6))
            - 0.20 * std::cos(pi / 180 * (4 * Hpbar - 63)));
        float angle = pi / 6 * std::exp(-std::pow((Hpbar - 275) / 25, 2));
        float Cpbar7 = std::pow(Cpbar, 7);
        float RC = 2 * std::sqrt(Cpbar7 / (Cpbar7 + std::pow(25, 7)));
        float Lpbar502 = std::pow(Lpbar - 50, 2);
        float SL = 1 + 0.015 * Lpbar502 / std::sqrt(20 + Lpbar502);
        float SC = 1 + 0.045 * Cpbar;
        float SH = 1 + 0.015 * Cpbar * T;
        float RT = -std::sin(2 * angle) * RC;
        float x = dLp / (kL * SL);
        float y = dCp / (kC * SC);
        float z = dHp / (kH * SH);
        return std::sqrt(x * x + y * y + z * z + RT * y * z);
    }
}

void convert_to_LabCiee( Uint8 r_1, Uint8 g_1, Uint8 b_1,  float lab_out[3] )
{
    float xyz[3] = { 0 ,0,0};
    unsigned char srgb[3] = { r_1, g_1, b_1 };
    color::srgb_to_xyz(srgb, xyz);
    color::xyz_to_lab(xyz, lab_out);
}


 
   ColorTuple::ColorTuple(  ){ 
        has_lab = false ;
    }
 
   ColorTuple::ColorTuple( int  r_in, int g_in, int b_in ){
        r = r_in;
        g = g_in;
        b = b_in;
        has_lab = false ;
    }
    // indexator for hash
    bool ColorTuple::operator<(const ColorTuple& other) const
    {
        return (r < other.r) || (r == other.r && g < other.g) || (r == other.r && g == other.g && b < other.b);
    }

    ColorTuple  ColorTuple::operator-(const ColorTuple& other) const
    {
        return ColorTuple( r - other.r, g - other.g, b - other.b );
    }

    //get Lab color
    void ColorTuple::get_lab( float lab_out[3] ){
        if( has_lab ){
            lab_out[0] = labColor[0];
            lab_out[1] = labColor[1];
            lab_out[2] = labColor[2];
            return ;
        }
        convert_to_LabCiee( r, g, b, labColor );
        lab_out[0] = labColor[0];
        lab_out[1] = labColor[1];
        lab_out[2] = labColor[2];
        has_lab = true ;
    }

    float ColorTuple::colorDiffRGB(   ColorTuple &other ){
        float r1 = r;
        float g1 = g;
        float b1 = b;
        float r2 = other.r;
        float g2 = other.g;
        float b2 = other.b;
      
        float dr = r1 - r2;
        float dg = g1 - g2;
        float db = b1 - b2;
        return std::sqrt( dr*dr + dg*dg + db*db );  
    }

    bool ColorTuple::isDull() {
        float lab[3] = { 0,0,0 };
        get_lab( lab );
        if  (( lab[1] < 20.0f ) && ( lab[2] < 20.0f )) return true;
        if  (( lab[1] > 80.0f ) && ( lab[2] > 80.0f ))  return true;
        return false ;
    }
 
    float ColorTuple::colorDiffYUV( ColorTuple &other){
 
        float Y0 = 0.299f * r + 0.587f * g + 0.114f * b;
        float U0 = -0.14713f * r - 0.28886f * g + 0.436f * b;
        float V0 = 0.615f * r - 0.51499f * g - 0.10001f * b;

        float Y1 = 0.299f * other.r + 0.587f * other.g + 0.114f * other.b;
        float U1 = -0.14713f * other.r - 0.28886f * other.g + 0.436f * other.b;
        float V1 = 0.615f * other.r - 0.51499f * other.g - 0.10001f * other.b;

        float dY = Y0 - Y1;
        float dU = U0 - U1;
        float dV = V0 - V1;

        return std::sqrt( dY*dY + dU*dU + dV*dV );

    }

    //get color difference
    float ColorTuple::colorDiffCiee(   ColorTuple &other ){

        // any one is gray like ?
        if( isDull() || other.isDull() ){
            return colorDiffRGB( other );
        }

        float lab1[3] = { 0,0,0 };
        float lab2[3] = { 0,0,0 };
        get_lab( lab1 );
        other.get_lab( lab2 );

        
        return color::diff_de00(lab1, lab2);
    }
     
 

  ColorTupleInt::ColorTupleInt( int r_in, int g_in, int b_in ){
        r = r_in;
        g = g_in;
        b = b_in;
    }
    ColorTupleInt::ColorTupleInt(){
        r = 0;
        g = 0;
        b = 0;
    }
  ColorTupleInt  ColorTupleInt::operator-(const ColorTupleInt& other) const
    {
        return ColorTupleInt( r - other.r, g - other.g, b - other.b );
  }

    ColorTupleInt  ColorTupleInt::operator+(const ColorTupleInt& other) const
        {
            return ColorTupleInt( r + other.r, g + other.g, b + other.b );
        }

    ColorTupleInt ColorTupleInt::frac16(int i) const
    {
        return ColorTupleInt( (i*r) / 16  , (i*g) / 16, (i*b) / 16 );
    }
 

  
 

float colorDiffCiee( Uint8 r_1, Uint8 g_1, Uint8 b_1, Uint8 r_2, Uint8 g_2, Uint8 b_2 )
{    
    float lab1[3] = { 0,0,0 };
    float lab2[3] = { 0,0,0 };
    convert_to_LabCiee( r_1, g_1, b_1, lab1 );
    convert_to_LabCiee( r_2, g_2, b_2, lab2 ); 
    return color::diff_de00(lab1, lab2);
}


 