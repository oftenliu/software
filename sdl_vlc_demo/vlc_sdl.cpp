#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <assert.h>
#include<string>
#include "SDL.h"
#include "SDL_mutex.h"
#include "SDL_thread.h"

#include "vlc/vlc.h"

#define VIDEOWIDTH 1920
#define VIDEOHEIGHT 1080

struct context {
	SDL_Renderer *renderer;
	SDL_Texture *texture;
	SDL_mutex *mutex;
	int n;

	SDL_Rect displayrect;
	int window_w;
	int window_h;
};

// VLC prepares to render a video frame.
static void *lock(void *data, void **p_pixels) {

	struct context *c = (struct context *) data;

	int pitch;
	SDL_LockMutex(c->mutex);
	SDL_LockTexture(c->texture, NULL, p_pixels, &pitch);

	return NULL; // Picture identifier, not needed here.
}

// VLC just rendered a video frame.
static void unlock(void *data, void *id, void * const *p_pixels) {

	struct context *c = (struct context *) data;
	SDL_UnlockTexture(c->texture);
	SDL_UnlockMutex(c->mutex);
}

// VLC wants to display a video frame.
static void display(void *data, void *id) {

	struct context *ctx = (struct context *) data;
	ctx->displayrect.x = 0;
	ctx->displayrect.y = 0;
	ctx->displayrect.w = ctx->window_w;
	ctx->displayrect.h = ctx->window_h;

	SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0,0);
	SDL_RenderClear(ctx->renderer);
	SDL_RenderCopy(ctx->renderer, ctx->texture, NULL, &ctx->displayrect);
	SDL_RenderPresent(ctx->renderer);
}

static void quit(int c) {
	SDL_Quit();
	exit(c);
}

int main(int argc, char *argv[]) {

	struct context ctx;

	libvlc_instance_t *libvlc = NULL;
	libvlc_media_t *m = NULL;
	libvlc_media_player_t *mp = NULL;


	SDL_Event event;
	int done = 0, action = 0, pause = 0, n = 0;

	//    if(argc < 2) {
	//        printf("Usage: %s <filename>\n", argv[0]);
	//        return EXIT_FAILURE;
	//    }

	// Initialise libSDL.
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("Could not initialize SDL: %s.\n", SDL_GetError());
		return EXIT_FAILURE;
	}

	// Create SDL graphics objects.
	SDL_Window * window = SDL_CreateWindow("Fartplayer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720,
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	ctx.window_w = 1280;
	ctx.window_h = 720;

	if (!window) {
		fprintf(stderr, "Couldn't create window: %s\n", SDL_GetError());
		quit(3);
	}

	int nRenderDrivers = SDL_GetNumRenderDrivers();
	int i = 0;
	for (; i < nRenderDrivers; i++) {
		SDL_RendererInfo info;
		SDL_GetRenderDriverInfo(i, &info);    //d3d
		printf("====info name %d: %s =====\n", i, info.name);
		printf("====max_texture_height %d  =====\n", i, info.max_texture_height);
		printf("====max_texture_width %d  =====\n", i, info.max_texture_width);

	}

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");

	ctx.renderer = SDL_CreateRenderer(window, -1, 0);
	if (!ctx.renderer) {
		fprintf(stderr, "Couldn't create renderer: %s\n", SDL_GetError());
		quit(4);
	}

	ctx.texture = SDL_CreateTexture(ctx.renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, VIDEOWIDTH, VIDEOHEIGHT);
	if (!ctx.texture) {
		fprintf(stderr, "Couldn't create texture: %s\n", SDL_GetError());
		quit(5);
	}

	ctx.mutex = SDL_CreateMutex();

	// If you don't have this variable set you must have plugins directory
	// with the executable or libvlc_new() will not work!

	// Initialise libVLC.

	char const *vlc_argv[] = {
		"--network-caching=1000",
		"--rtsp-frame-buffer-size=1000000",
		"--rtsp-tcp",
		"--no-xlib",
	};
	int vlc_argc = sizeof(vlc_argv) / sizeof(*vlc_argv);
	libvlc = libvlc_new(vlc_argc, vlc_argv);
	if (NULL == libvlc) {
		printf("LibVLC initialization failure.\n");
		return EXIT_FAILURE;
	}
	std::string url = "rtsp://admin:youwillsee!@192.168.1.193";
	m = libvlc_media_new_location(libvlc, url.c_str());
	
	mp = libvlc_media_player_new_from_media(m);
	//libvlc_media_release(m);

	libvlc_video_set_callbacks(mp, lock, unlock, display, &ctx);
	libvlc_video_set_format(mp, "RV32", VIDEOWIDTH, VIDEOHEIGHT, VIDEOWIDTH * 4);
	libvlc_media_player_play(mp);

	// Main loop.
	while (!done) {

		action = 0;

		// Keys: enter (fullscreen), space (pause), escape (quit).
		while (SDL_PollEvent(&event)) {

			switch (event.type) {
			case SDL_WINDOWEVENT:
				if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
					libvlc_media_player_stop(mp);
					SDL_SetRenderDrawColor(ctx.renderer, 0, 0, 0, 0);
					SDL_RenderClear(ctx.renderer);
					SDL_RenderCopy(ctx.renderer, ctx.texture, NULL, &ctx.displayrect);
					SDL_RenderPresent(ctx.renderer);
					SDL_RenderSetViewport(ctx.renderer, NULL);
					event.window.data1 = event.window.data1 > 1920 ? 1920 : event.window.data1;
					event.window.data2 = event.window.data2 > 1080 ? 1080 : event.window.data2;
					ctx.displayrect.w = ctx.window_w = event.window.data1;
					ctx.displayrect.h = ctx.window_h = event.window.data2;
					libvlc_media_player_play(mp);

				}
				break;
			case SDL_QUIT:
				done = 1;
				break;
			case SDL_KEYDOWN:
				action = event.key.keysym.sym;
				break;
			}
		}

		switch (action) {
		case SDLK_ESCAPE:
		case SDLK_q:
			done = 1;
			break;
		case ' ':
			printf("Pause toggle.\n");
			pause = !pause;
			break;
		}
		if (!pause) {
			ctx.n++;
		}

		SDL_Delay(100);
	}

	// Stop stream and clean up libVLC.
	libvlc_media_player_stop(mp);
	libvlc_media_player_release(mp);
	libvlc_release(libvlc);

	// Close window and clean up libSDL.
	SDL_DestroyMutex(ctx.mutex);
	SDL_DestroyRenderer(ctx.renderer);

	quit(0);

	return 0;
}