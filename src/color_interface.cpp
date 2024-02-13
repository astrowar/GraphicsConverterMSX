
#include "imgui.h"

#include "color_adjust.h"

#include <cstdio>
#include <algorithm>


bool color_adjust( adjust_parameter_t cadj[NUMCOLORSADJ] ){

 //copy cadj to input_cadj
 // check at end if some parameter has changed, if so, call color_adjust_surface
 adjust_parameter_t input_cadj[NUMCOLORSADJ] ;
  for (int ncolor = 0; ncolor<NUMCOLORSADJ;  ncolor++)
  {
    input_cadj[ncolor].hue = cadj[ncolor].hue;
    input_cadj[ncolor].saturation = cadj[ncolor].saturation;
    input_cadj[ncolor].lightness = cadj[ncolor].lightness;
   
  }

 ImDrawList* draw_list = ImGui::GetWindowDrawList();
 


 const ImVec2 p = ImGui::GetCursorScreenPos();
//// float x = p.x + 4.0f, y = p.y + 4.0f, spacing = 8.0f;
 static float sz = 16.0f;
 const ImU32 col32 = ImColor(ImVec4(1.0f, 1.0f, 0.1f, 1.0f));

 
  
 // draw_list->AddCircleFilled (ImVec2(x+sz*0.5f, y+sz*0.5f), sz*0.5, col32, 20 ); x += sz+spacing;    // Circle   
 // ImGui::Dummy(ImVec2(sz, sz));
  //ImGui::SetCursorScreenPos(ImVec2(x, y));

  ImU32 HueTableColor [NUMCOLORSADJ] ; 
  //red, orange, yoelow, green, light blue, blue, purple, pink
  HueTableColor[0] = ImColor(ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
  HueTableColor[1] = ImColor(ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
  HueTableColor[2] = ImColor(ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
  HueTableColor[3] = ImColor(ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
  HueTableColor[4] = ImColor(ImVec4(0.0f, 1.0f, 1.0f, 1.0f));
  HueTableColor[5] = ImColor(ImVec4(0.0f, 0.0f, 1.0f, 1.0f));
  HueTableColor[6] = ImColor(ImVec4(0.5f, 0.0f, 1.0f, 1.0f));
  HueTableColor[7] = ImColor(ImVec4(1.0f, 0.0f, 1.0f, 1.0f)); 


  for (int ncolor = 0; ncolor<NUMCOLORSADJ;  ncolor++)
  { 
    ImGui::PushID( ncolor );
    const ImVec2 p = ImGui::GetCursorScreenPos();
    float x = p.x + 4.0f, y = p.y + 4.0f, spacing = 8.0f;

    const ImU32 col32 =  HueTableColor[ncolor];
     draw_list->AddCircleFilled (ImVec2(x+sz*0.5f, y+sz*0.5f), sz*0.5, col32, 20 ); x += sz+spacing;    // Circle   
     ImGui::Dummy(ImVec2(sz, sz));ImGui::SameLine();


    ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.20f);  
    ImGui::SliderFloat("Hue", &(cadj[ncolor].hue), -0.3f, 0.3f);ImGui::SameLine();
    

    ImGui::SameLine();
    //ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.15f);
    ImGui::SliderFloat("Sat", &(cadj[ncolor].saturation), -1.0f, 1.0f);ImGui::SameLine();
    
    ImGui::SameLine();
    //ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.15f);
    ImGui::SliderFloat("Ligh", &(cadj[ncolor].lightness), -1.0f, 1.0f);
 
    ImGui::PopID();
  }

  //chech if has change 
  bool has_change = false;
  for (int ncolor = 0; ncolor<NUMCOLORSADJ;  ncolor++)
  {
    if (input_cadj[ncolor].hue != cadj[ncolor].hue)
    {
      has_change = true;
      break;
    }
    if (input_cadj[ncolor].saturation != cadj[ncolor].saturation)
    {
      has_change = true;
      break;
    }
    if (input_cadj[ncolor].lightness != cadj[ncolor].lightness)
    {
      has_change = true;
      break;
    }
   
  }

  return  has_change;
}
 

 void color_adjust_surface(SDL_Surface *surface_in,  SDL_Surface *surface_to,   adjust_parameter_t p[NUMCOLORSADJ] ){

//same size ?
if (surface_in->w != surface_to->w || surface_in->h != surface_to->h)
{
  printf("Error: surface_in and surface_to must have same size\n");
  return;
}

//hue center values for each color
//red, orange, yoelow, green, light blue, blue, purple, pink

float hue_color[NUMCOLORSADJ] = { 0.0f, 0.083f, 0.167f, 0.333f, 0.5f, 0.667f, 0.75f, 0.917f };


  

  //for each pixel in surface_in 
  for (int y = 0; y < surface_in->h; y++)
  {
    for (int x = 0; x < surface_in->w; x++)
    {
      //get pixel
      Uint32 pixel = get_pixel(surface_in, x, y);

      //get RGB
      Uint8 r, g, b;
      SDL_GetRGB(pixel, surface_in->format, &r, &g, &b);

      //convert to HSL
      float h, s, l;
      RGBtoHSL(r, g, b, h, s, l);

      const float fraction_hue = 0.09f;

      //adjust HSL
      for (int ncolor = 0; ncolor<NUMCOLORSADJ;  ncolor++)
      {  
        float fraction = 0.0f;
        if (h >= hue_color[ncolor] + fraction_hue) continue;
        if (h <= hue_color[ncolor] - fraction_hue) continue;

        float  faction  = 1.0f - std::abs(h - hue_color[ncolor]) / fraction_hue;
        faction = faction * s;

        // float flatness_factor = 1.0f - (faction*  p[ncolor].flatness); 
        // flatness_factor = flatness_factor ;
        // // flatness_factor ==0.0f means no changes 
        // // flatness_factor ==1.0f means color == hue_color[ncolor]

        // h =  (h- hue_color[ncolor]) * flatness_factor + hue_color[ncolor]; 
        // s = ( s - 0.5f) * flatness_factor +0.5f;
        // l = ( l - 0.5f) * flatness_factor +0.5f;
       
        h += faction * p[ncolor].hue / 1.0;
        s += faction * p[ncolor].saturation/1.0;
        l += faction * p[ncolor].lightness/1.0;

    
      }

      //keep in range
      h = std::max(0.0f, std::min(1.0f, h));
      s = std::max(0.0f, std::min(1.0f, s));
      l = std::max(0.0f, std::min(1.0f, l));

      //convert to RGB
      Uint8 r2, g2, b2;
      HSLtoRGB(h, s, l, r2, g2, b2);

      //set pixel
      set_pixel(surface_to, x, y, r2, g2, b2);
    }
  }

 }

