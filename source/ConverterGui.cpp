

//create a SDL main function
//create a SDL window

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_opengl.h>
#include <iostream>
#include <vector>
#include <filesystem>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl2.h"
#include "color_adjust.h"

#include "parameters.h"


using namespace std;

 #define WIN_WIDTH 1500
 #define WIN_HEIGHT 720


  int original_width = 1024;
  int original_height = 1024;
 


 void  update_texture_msx( SDL_Surface *surface , GLuint textureID )
  {
	      //number channels in SDL surface
	  int nOfColors = surface->format->BytesPerPixel;
	 // GLenum textureFormatIn  = GL_RGBA; 
	  GLenum textureFormatIn  = surface->format->Rmask == 0x000000ff ? GL_RGBA : GL_BGRA;
   

     //create a 256x256 surface
	 SDL_Surface* surface_256 = SDL_CreateRGBSurface(0, 256, 256, 32, 0,0,0,0);
	 //blitScale
	 SDL_BlitSurface(surface, NULL, surface_256, NULL);
	// SDL_BlitScaled(surface, NULL, surface_256, NULL);
	 //copy to texture  
	 glBindTexture(GL_TEXTURE_2D, textureID);
	 glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface_256->w, surface_256->h, 0, textureFormatIn, GL_UNSIGNED_BYTE, surface_256->pixels);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


  }
  GLuint   create_openGL_texture (  SDL_Surface *surface ) {
	  GLuint textureID;

      //number channels in SDL surface
	  int nOfColors = surface->format->BytesPerPixel;
	  std::cout << "contains channels "  << nOfColors << std::endl;

	  //pixels in format RGBA or BGRA
	  GLenum textureFormatIn  = GL_RGB;

	  if (nOfColors == 4)     // contains an alpha channel
	  {
		   
		  if (surface->format->Rmask == 0x000000ff)
			  textureFormatIn =  GL_RGBA;
		  else
			  textureFormatIn = GL_BGRA;
	  }
	  else if (nOfColors == 3)     // no alpha channel
	  {
		  if (surface->format->Rmask == 0x000000ff)
			  textureFormatIn = GL_RGB;
		  else
			  textureFormatIn = GL_BGR;
	  }
	  else {
		  std::cout << "warning: the image is not truecolor..  this will probably break" << std::endl;
		  // this error should not go unhandled
	  }

	  glGenTextures(1, &textureID);
	  glBindTexture(GL_TEXTURE_2D, textureID);
	  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, textureFormatIn, GL_UNSIGNED_BYTE, surface->pixels);
     

	  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	  return textureID;
  }


  int render_texture(GLuint sdltexture) {
	  
	 glEnable(GL_TEXTURE_2D); 
	 glBindTexture(GL_TEXTURE_2D, sdltexture); 

	//openGL render QUADS
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glBegin(GL_QUADS);
	 
	glTexCoord2f(0, 0); glVertex2f(-1, 1);
	glTexCoord2f(1, 0); glVertex2f(1, 1);
	glTexCoord2f(1, 1); glVertex2f(1, -1);
	glTexCoord2f(0, 1); glVertex2f(-1, -1);
	glEnd();
 
	return 0;
  }


  bool LoadTextureFromFile( const char* filename   , SDL_Texture** texture_ptr, int& width, int& height, SDL_Renderer* renderer) {
    int channels;
  
    SDL_Surface* surface = IMG_Load(filename);
	if (surface == nullptr) {
		fprintf(stderr, "Failed to load image: %s\n", SDL_GetError());
		return false ;
	}

	width = surface->w;
	height = surface->h;

	original_width = width;
	original_height = height; 


   //nearest power 2 size	
    int w = 1; while (w < width) w *= 2;
    int h = 1; while (h < height) h *= 2;
	if (w != width || h != height) {
		SDL_Surface* surface2 = SDL_CreateRGBSurface(0, w, h, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
		SDL_BlitScaled(surface,  NULL, surface2, NULL);
		SDL_FreeSurface(surface);
		surface = surface2;
	}


    *texture_ptr = SDL_CreateTextureFromSurface(renderer, surface);
 
    if ((*texture_ptr) == nullptr) {
        fprintf(stderr, "Failed to create SDL texture: %s\n", SDL_GetError());
    }
   
    return true;
}
 

void adjust_image_size( int avaliable_width , int avaliable_height , int original_width , int original_height , float& wsize , float& hsize ) {
	//keep aspect ratio from original image
	wsize = avaliable_width ;
	hsize = avaliable_height ;

   float scale = std::max( (float)avaliable_width / (float)original_width , (float)avaliable_height / (float)original_height );

   wsize = original_width * scale;
   hsize = original_height * scale;

	while (wsize > avaliable_width || hsize > avaliable_height) {
		scale = scale * 0.99f;
		wsize = original_width * scale;
		hsize = original_height * scale;
	}
}

int ashow(const char* filename ,  SDL_Surface* surface  ) 
{
  
  unsigned w = surface->w;
  unsigned h = surface->h;
  //avoid too large window size by downscaling large image

  unsigned jump = 1;
  if(w / 1024 >= jump) jump = w / 1024 + 1;
  if(h / 1024 >= jump) jump = h / 1024 + 1;



  size_t screenw = WIN_WIDTH;
  size_t screenh =  WIN_HEIGHT;
  size_t pitch = screenw * sizeof(Uint32);
  std::string caption = filename;

  std::cout << "showing " << caption << " " << w << "x" << h << " at " << jump << "x" << jump << "=" << screenw << "x" << screenh << std::endl;
  //init SDL
  SDL_Window* sdl_window;
  SDL_Renderer* sdl_renderer;
  SDL_CreateWindowAndRenderer(screenw, screenh, SDL_WINDOW_OPENGL| SDL_WINDOW_RESIZABLE  , &sdl_window, &sdl_renderer);
    std::cout << "SDL SDL_CreateWindowAndRenderer created" << std::endl;
  SDL_SetWindowTitle(sdl_window, caption.c_str());
    std::cout << "SDL SDL_SetWindowTitle created" << std::endl;

  if(!sdl_window) {
    std::cout << "error, no SDL screen" << std::endl;
    return 0;
  }
  std::cout << "SDL window  created" << std::endl;

 
 
   
   //IMgui initiakization
				SDL_GLContext gl_context = SDL_GL_CreateContext(sdl_window);
			SDL_GL_MakeCurrent(sdl_window, gl_context);

				// Setup Dear ImGui context
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO(); 
			(void)io;
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

			// Setup Dear ImGui style
			ImGui::StyleColorsDark();
			//ImGui::StyleColorsLight();

			// Setup Platform/Renderer backends
			ImGui_ImplSDL2_InitForOpenGL(sdl_window, gl_context);
			ImGui_ImplOpenGL2_Init();
   

  //load as texture 
	SDL_Texture* my_texture;
	int my_image_width, my_image_height;
	bool ret = LoadTextureFromFile( filename  , &my_texture, my_image_width, my_image_height, sdl_renderer);
	if (ret == true )
        std::cout << "ok LoadTextureFromFile   " << std::endl;
	else 
		std::cout << "error LoadTextureFromFile   " << std::endl;
 

  GLuint  tex_id =  create_openGL_texture (surface );

  // copy of surface with color adjust
  SDL_Surface* surface_color_adjust = SDL_CreateRGBSurface(0, surface->w, surface->h, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
  SDL_BlitSurface(surface, NULL, surface_color_adjust, NULL);

  GLuint  tex_color_adjust =  create_openGL_texture (surface_color_adjust );

  std::cout << "SDL pixels plotted" << std::endl;

  GLuint  tex_id_msx ;    
  glGenTextures(1, &tex_id_msx);

  // render the pixels to the screen
 


  SDL_RenderClear(sdl_renderer);
 // SDL_RenderCopy(sdl_renderer, sdl_texture, NULL, NULL);
  //SDL_RenderPresent(sdl_renderer);

static float f_contrast = 0.0f;
static float f_compress = 1.0f;


static  adjust_parameter_t color_adjust_params[8] ; 
static  Paramameters_t  conversion_params ;

static char buf[255]; 
 
 
std::string filename_out = filename;
//replace extension by .sc2
if (filename_out.find(".") == std::string::npos) {
	filename_out = filename_out + ".sc2";
}
else {
	filename_out = filename_out.substr(0, filename_out.find_last_of(".")) + ".sc2";
}
std::filesystem::path fullpath =  std::filesystem::current_path() / filename_out;	
 
 //write to buf
 filename_out = fullpath.string();
 strcpy(buf, filename_out.c_str()); 




SDL_Surface* surface_msx = SDL_CreateRGBSurface(0, 256, 192, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);

  //pause until you press escape and meanwhile redraw screen
  SDL_Event event;
  int done = 0;
  while(done == 0) 
  {
    while(SDL_PollEvent(&event)) {
		ImGui_ImplSDL2_ProcessEvent(&event);
      if(event.type == SDL_QUIT) done = 2;
      else if(SDL_GetKeyboardState(NULL)[SDLK_ESCAPE]) done = 2;
      //else if(event.type == SDL_KEYDOWN) done = 1; //press any other key for next image 

	  //mouse Down	
	  if (!io.WantCaptureMouse)
	  {
	    if (event.type == SDL_MOUSEBUTTONDOWN) {
		  if (event.button.button == SDL_BUTTON_LEFT) {
			  std::cout << "mouse down" << std::endl;
		  }
	  }
	  }

    }

		glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor( 0.45f, 0.55f, 0.60f, 1.00f );
        glClear(GL_COLOR_BUFFER_BIT);

	    // Start the Dear ImGui frame
        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        ImGui::Begin("Input");   
		ImGui::Button("OK");
		//slider for contrast  
		ImGui::SliderFloat("Weight", &f_contrast, -1.0f, 1.0f);
		ImVec2 av = ImGui::GetContentRegionAvail();
		av.x = av.x/ 2;
 
        
		//keep aspect ratio from original image
		float wsize ;
		float hsize; 

		adjust_image_size( av.x , av.y , original_width , original_height , wsize , hsize );
		
		ImGui::Image((void*)(intptr_t)tex_id,  ImVec2(wsize, hsize), ImVec2(0, 0), ImVec2(1, 1));
		ImGui::SameLine();
		ImGui::Image((void*)(intptr_t)tex_color_adjust,  ImVec2(wsize, hsize), ImVec2(0, 0), ImVec2(1, 1));

		ImGui::End();

		 ImGui::Begin("MSX");

	 
         ImGui::InputText("Filename",buf,  255);
		 if (ImGui::Button("Save"))
		 {
			std::cout << "Save pressed" << std::endl;
			if (strlen(buf) > 0) {
				 //find folder for filename 
				  filename_out = buf;
				  //get parent directory	
				  std::filesystem::path fullpath = std::filesystem::path( filename_out);
				   std::filesystem::path diretory =  fullpath.parent_path();
				   std::cout << "parent path " << diretory.string() << std::endl;				
				   //exist this path ?
				   if (std::filesystem::exists(diretory)) {
					   std::cout << "path exists " << std::endl;
					      save_tiles(fullpath);
				   } 
				   else{
					// message to screen 
						if (ImGui::BeginPopup("Message"))
								{
									ImGui::Selectable("Error on Directory");
									ImGui::EndPopup();
								}
					
				   }
			}
		 
		 }
		 av = ImGui::GetContentRegionAvail();
		 adjust_image_size( av.x , av.y , 256 , 192 , wsize , hsize );
		 ImGui::Image((void*)(intptr_t)tex_id_msx,  ImVec2(wsize, wsize), ImVec2(0, 0), ImVec2(1, 1));	
 
		 ImGui::End();

 		//Hue adjust
		ImGui::Begin("Adjust");
		bool has_change = color_adjust(color_adjust_params);
		ImGui::End(); 

		if (has_change) {
			//copy surface to surface_color_adjust
			SDL_BlitSurface(surface, NULL, surface_color_adjust, NULL);
			//adjust color
			color_adjust_surface(surface, surface_color_adjust, color_adjust_params);
			//update texture
			tex_color_adjust = create_openGL_texture(surface_color_adjust);
		}

         ImGui::Begin("Likennes");   
		
		 ImGui::SliderFloat("Compression", &( conversion_params.compression ), 0.0f, 1.0f);
		 ImGui::SliderFloat("BlackLev", &( conversion_params.black ), 0.0f, 2.0f);
		 ImGui::SliderFloat("dithering", &( conversion_params.dithering ), 0.0f, 1.0f);
	
	    //selector for sRGB, Ciee or YUV
		 const char* items[] = { "sRGB", "Ciee", "YUV" };
		 
		 ImGui::Combo("ColorDiff", &(conversion_params.colorDiffMethod ), items, IM_ARRAYSIZE(items));
 

		 if (ImGui::Button("Convert"))
		 {
			std::cout << "OK pressed" << std::endl;
			//convert to MSX
		 
			convert_to( surface_color_adjust, surface_msx,  &conversion_params );	
			//update texture
			update_texture_msx( surface_msx, tex_id_msx );
		 } 
		ImGui::End();


	    ImGui::Render();

        //glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context where shaders may be bound
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
		
        SDL_GL_SwapWindow(sdl_window);    
		
 


    SDL_Delay(5); //pause 5 ms so it consumes less processing power
  }



  SDL_Quit();
  return done == 2 ? 1 : 0;
}

/*shows image with SDL. Returns 1 if user wants to fully quit, 0 if user wants to see next image.*/
int showfile(const char* filename) {


  std::vector<unsigned char>  image(1024);
   
  unsigned w, h;

   std::cout << "showing " << filename << std::endl;
  
  SDL_Surface* surface = IMG_Load(filename);
  
  if(!surface) {
	std::cout << "error, no SDL surface" << std::endl;
	return 0;
  }
    std::cout << "SDL surface created" << std::endl;

  w = surface->w;
  h = surface->h; 
  image.resize(w * h * 4);

  //memcpy(image.data() , surface->pixels,  w*h* 4);
   
 

  return ashow(filename,   surface  );
}

  


 int SDL_main(int argc, char* argv[]) {

    //check input => program.exe image_in
    if (argc != 2) {
		std::cout << "Usage: " << std::endl;
		return 1;
	}

	char* image_in = argv[1];
 

	 
		SDL_Init(SDL_INIT_EVERYTHING);

  


	showfile(image_in);
	  
	return 0;

	 SDL_Init(SDL_INIT_EVERYTHING);
	 SDL_Window* window = SDL_CreateWindow("Converter", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1200, 600, SDL_WINDOW_SHOWN);
	 SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED| SDL_RENDERER_TARGETTEXTURE);
	 SDL_Surface* surface = IMG_Load(image_in);
	 SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
	 SDL_FreeSurface(surface);

	 	SDL_Texture *texTarget = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, 	SDL_TEXTUREACCESS_TARGET, WIN_WIDTH, WIN_HEIGHT);
    SDL_SetRenderTarget(renderer, texTarget);

	 SDL_RenderClear(renderer);
	 SDL_RenderCopy(renderer, texture, NULL, NULL);
	 SDL_RenderPresent(renderer);
	 SDL_Delay(3000);
	 SDL_DestroyTexture(texture);
	 SDL_DestroyRenderer(renderer);
	 SDL_DestroyWindow(window);
	 SDL_Quit();
	 return 0;
 }

int main_old() {
 
	return 0;

} 