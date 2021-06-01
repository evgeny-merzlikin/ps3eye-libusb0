/*This source code copyrighted by Lazy Foo' Productions (2004-2020)
and may not be redistributed without written permission.*/

//Using SDL and standard IO
#include <SDL2/SDL.h>
#include <stdio.h>
#include <sstream>
#include <ps3eye_capi.h>



//Screen dimension constants
int screen_width, screen_height;
float kx, ky;

//Screen dimension constants
const int CAMERA_WIDTH = 320;
const int CAMERA_HEIGHT = 240;
const int CAMERA_FPS = 125;
const int CAMERA_FRAME_SIZE = CAMERA_WIDTH * CAMERA_HEIGHT * 3; // BGR24 or RGB24 formats




void threshold_bgr(unsigned char *src, unsigned char *dest,int width,int height, int threshold_val)
{
    int r, g, b, gray;
    for (int i=0;i<width*height;++i)
    {
        b = *src++; // load red
        g = *src++; // load green
        r = *src++; // load blue
        // build weighted average:
		//gray = (r * 76 + g * 150 + b * 30) >>8;
		gray = 0.212671f * r + 0.715160f * g + 0.072169f * b;
		if (gray < threshold_val)  {
			gray = gray +160;
			if (gray > 255) gray = 255;
			*dest++ = gray; // b
			*dest++ = gray; // g 
			*dest++ = gray; // r

		} else {
			*dest++ = 0; // b
			*dest++ = 0; // g 
			*dest++ = 255; // r

		
		}
		
    }
    
}

int main( int argc, char* args[] )
{
	
	ps3eye_init();

	auto i = ps3eye_count_connected();
	printf("Found %i cameras\n", i);
	if (i == 0) {
		printf("No PS3 camera found!");
		exit(0);
	}

	auto eye = ps3eye_open(0, CAMERA_WIDTH, CAMERA_HEIGHT, CAMERA_FPS, ps3eye_format::PS3EYE_FORMAT_BGR);
	ps3eye_set_parameter(eye, ps3eye_parameter::PS3EYE_HFLIP, 0);
	ps3eye_set_parameter(eye, ps3eye_parameter::PS3EYE_VFLIP, 0);
	ps3eye_set_parameter(eye, ps3eye_parameter::PS3EYE_AUTO_GAIN, 0);
	ps3eye_set_parameter(eye, ps3eye_parameter::PS3EYE_GAIN, 0);
	ps3eye_set_parameter(eye, ps3eye_parameter::PS3EYE_EXPOSURE, 255);

	//ps3eye_set_parameter(eye, ps3eye_parameter::PS3EYE_AUTO_WHITEBALANCE, 1);

	/*
	capi_sccb_reg_write(eye, 0x64, 0xFF);
	capi_sccb_reg_write(eye, 0x8E, 0x20);
	capi_sccb_reg_write(eye, 0x91, 0x10);
	*/
	//uint8_t* frame = (uint8_t*)malloc(640*480*3);
	
	//The window we'll be rendering to
	//SDL_Window* window = NULL;
	
	//The surface contained by the window
	//SDL_Surface* screenSurface = NULL;

	//Initialize SDL
	if( SDL_Init( SDL_INIT_EVERYTHING ) < 0 )
	{
		printf( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
		exit(0);
	}



	//Create window
	//auto window = SDL_CreateWindow( "PS3EYE", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN );
	auto window = SDL_CreateWindow("", 0, 0, 0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_SHOWN);
	SDL_DisplayMode DM;
	SDL_GetCurrentDisplayMode(0, &DM);
	screen_width = DM.w;
	screen_height = DM.h;
	printf("screen_width = %d, screen_height = %d\n",screen_width, screen_height);
	kx = (float)screen_width / CAMERA_WIDTH;
	ky = (float)screen_height / CAMERA_HEIGHT;
	printf("kx = %f, ky = %f\n",kx, ky);

	SDL_Rect frame_rect;

	frame_rect.w = CAMERA_WIDTH  * ky;
	frame_rect.h = CAMERA_HEIGHT * ky;
	
	frame_rect.x = (screen_width - frame_rect.w) / 2;
	frame_rect.y = 0;

	


	if( window == NULL )
	{
		printf( "Window could not be created! SDL_Error: %s\n", SDL_GetError() );
	}

	auto renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	if (renderer == NULL) 
	{
		SDL_DestroyWindow(window);
		printf("Failed to create renderer: %s\n", SDL_GetError());
	}

	//SDL_RenderSetLogicalSize(renderer, CAMERA_WIDTH, CAMERA_HEIGHT);

	auto video_tex = SDL_CreateTexture(
		renderer, SDL_PIXELFORMAT_BGR24, SDL_TEXTUREACCESS_STREAMING,
		CAMERA_WIDTH, CAMERA_HEIGHT);			



	if (video_tex == NULL )
	{
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		printf("Failed to create video texture: %s\n", SDL_GetError());
	}

	auto frame = (uint8_t*)calloc(CAMERA_FRAME_SIZE, sizeof(uint8_t));
	
	auto thresholded_frame = (uint8_t*)calloc(CAMERA_FRAME_SIZE, sizeof(uint8_t));
	if (frame == NULL)
	{
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		printf("Unable to allocate video_framebuf\n", SDL_GetError());
	}


	//Main loop flag
	bool quit = false;

	//Event handler
	SDL_Event e;

	//While application is running
	while( quit == false )
	{
		//Handle events on queue
		while(SDL_PollEvent( &e ))
		{

			//User requests quit
			if( e.type == SDL_QUIT )
			{
				quit = true;
				break;
			}
		}
		

		ps3eye_grab_frame(eye, frame);
		//bgr2gray( frame, gray_frame, CAMERA_WIDTH, CAMERA_HEIGHT);
		threshold_bgr(frame, thresholded_frame, CAMERA_WIDTH, CAMERA_HEIGHT, 112);

		void *pixels;
		int pitch;
		if ( SDL_LockTexture(video_tex, NULL, &pixels, &pitch) ) {
			printf("SDL_LockTexture failed: %s\n", SDL_GetError());
		}

		memcpy(pixels, thresholded_frame, CAMERA_FRAME_SIZE);
		SDL_UnlockTexture(video_tex);

		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		SDL_RenderClear(renderer);

		SDL_RenderCopy(renderer, video_tex, NULL, &frame_rect);
		
		
		SDL_RenderPresent(renderer);

	}
			

		
	
	//printf("closing camera...\n");
	ps3eye_close(eye);
	ps3eye_uninit();
	free(frame);

	SDL_DestroyTexture(video_tex);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}



