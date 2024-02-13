#include <SDL.h>
#include <algorithm>

Uint32 get_pixel(SDL_Surface *surface, int x, int y){
    Uint32 * const target_pixel = (Uint32 *) ((Uint8 *) surface->pixels
                                                 + y * surface->pitch
                                                 + x * surface->format->BytesPerPixel);
    return *target_pixel;
}

//Converts an RGB color to HSL color
	void  RGBtoHSL(Uint8 r_in, Uint8 g_in, Uint8 b_in, float &h_out, float &s_out, float &l_out ) 
	{
		double r, g, b, a, h = 0, s = 0, l; //this function works with floats between 0 and 1
		// r = ColorRGBA.r / 256.0;
		// g = ColorRGBA.g / 256.0;
		// b = ColorRGBA.b / 256.0;
		// a = ColorRGBA.a / 256.0;
        r = r_in / 256.0;
        g = g_in / 256.0;
        b = b_in / 256.0;
        a = 0;


		const double maxColor = std::max(r, std::max(g, b));
		const double minColor = std::min(r, std::min(g, b));

		if (minColor == maxColor) //R = G = B, so it's a shade of grey
		{
			h = 0; //it doesn't matter what value it has
			s = 0;
			l = r; //doesn't matter if you pick r, g, or b
		}
		else
		{
			l = (minColor + maxColor) / 2;

			if (l < 0.5) s = (maxColor - minColor) / (maxColor + minColor);
			if (l >= 0.5) s = (maxColor - minColor) / (2.0 - maxColor - minColor);

			if (r == maxColor) h = (g - b) / (maxColor - minColor);
			if (g == maxColor) h = 2.0 + (b - r) / (maxColor - minColor);
			if (b == maxColor) h = 4.0 + (r - g) / (maxColor - minColor);

			h /= 6; //to bring it to a number between 0 and 1
			if (h < 0) h += 1;
		}

		// ColorHSL colorHSL;
		// colorHSL.h = int(h * 255.0);
		// colorHSL.s = int(s * 255.0);
		// colorHSL.l = int(l * 255.0);
		// colorHSL.a = int(a * 255.0);
        // return colorHSL;
        h_out  = h ;
        s_out  = s ;
        l_out  = l ;

		 
	}

	//Converts an HSL color to RGB color
	void  HSLtoRGB( float h_in, float s_in, float l_in, Uint8 &r_out, Uint8 &g_out, Uint8 &b_out )
	{
		double r, g, b, a, h, s, l; //this function works with floats between 0 and 1
		double temp1, temp2, tempr, tempg, tempb;
		// h = colorHSL.h / 256.0;
		// s = colorHSL.s / 256.0;
		// l = colorHSL.l / 256.0;
		// a = colorHSL.a / 256.0;
        h = h_in ;
        s = s_in ;
        l = l_in ;
        a = 0;

		//If saturation is 0, the color is a shade of grey
		if (s == 0) r = g = b = l;
		//If saturation > 0, more complex calculations are needed
		else
		{
			//set the temporary values
			if (l < 0.5) temp2 = l * (1 + s);
			else temp2 = (l + s) - (l * s);
			temp1 = 2 * l - temp2;
			tempr = h + 1.0 / 3.0;
			if (tempr > 1.0) tempr--;
			tempg = h;
			tempb = h - 1.0 / 3.0;
			if (tempb < 0.0) tempb++;

			//red
			if (tempr < 1.0 / 6.0) r = temp1 + (temp2 - temp1) * 6.0 * tempr;
			else if (tempr < 0.5) r = temp2;
			else if (tempr < 2.0 / 3.0) r = temp1 + (temp2 - temp1) * ((2.0 / 3.0) - tempr) * 6.0;
			else r = temp1;

			//green
			if (tempg < 1.0 / 6.0) g = temp1 + (temp2 - temp1) * 6.0 * tempg;
			else if (tempg < 0.5) g = temp2;
			else if (tempg < 2.0 / 3.0) g = temp1 + (temp2 - temp1) * ((2.0 / 3.0) - tempg) * 6.0;
			else g = temp1;

			//blue
			if (tempb < 1.0 / 6.0) b = temp1 + (temp2 - temp1) * 6.0 * tempb;
			else if (tempb < 0.5) b = temp2;
			else if (tempb < 2.0 / 3.0) b = temp1 + (temp2 - temp1) * ((2.0 / 3.0) - tempb) * 6.0;
			else b = temp1;
		}

		// ColorRGBA ColorRGBA;
		// ColorRGBA.r = int(r * 255.0);
		// ColorRGBA.g = int(g * 255.0);
		// ColorRGBA.b = int(b * 255.0);
		// ColorRGBA.a = int(a * 255.0);
        // return ColorRGBA;
        r_out = int(r * 255.0);
        g_out = int(g * 255.0);
        b_out = int(b * 255.0);
 
	}