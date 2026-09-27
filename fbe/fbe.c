/*
 * fbe.c - Framebuffer Emulator for Linux development and release playback
 *
 * Creates an SDL2 window that displays the contents of /tmp/fb0,
 * updating at ~60 FPS. This lets you develop the demo in WSL
 * without needing a real /dev/fb0 framebuffer.
 *
 * Usage:
 *   cd fbe && make && ./fbe &
 *   cd ../src && make dev && ./demo
 *
 * The demo writes to /tmp/fb0 (via make dev), and fbe displays it.
 *
 * Based on framebuffer emulator concept from byteobserver / tinycelfgraphics.
 */

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <limits.h>

#ifndef WIDTH
#define WIDTH 1024
#endif
#ifndef HEIGHT
#define HEIGHT 768
#endif
#define BPP    4
#define FB_SIZE (WIDTH * HEIGHT * BPP)
#define FB_PATH "/tmp/fb0"

static volatile int running = 1;

static void sighandler(int sig) {
    (void)sig;
    running = 0;
}

static void usage(const char *name) {
    fprintf(stderr,
            "Usage: %s [options] [width height]\n"
            "  --fullscreen     fullscreen with aspect-preserving black bars (default)\n"
            "  --windowed       open a normal window\n"
            "  --display N      use display N\n"
            "  --linear         smooth scaling (default)\n"
            "  --nearest        sharp nearest-neighbor scaling\n"
            "  --demo PATH      launch and manage the demo process\n",
            name);
}

static void close_demo_fds(long max_fd) {
#ifdef SYS_close_range
    if (syscall(SYS_close_range, 3u, ~0u, 0) == 0) {
        return;
    }
#endif
    for (long child_fd = 3; child_fd < max_fd; child_fd++) {
        close((int)child_fd);
    }
}

int main(int argc, char *argv[]) {
    int w = WIDTH, h = HEIGHT;
    int fullscreen = 1;
    int display = 0;
    int linear = 1;
    int positional = 0;
    int exit_code = 0;
    const char *demo_path = NULL;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--fullscreen")) {
            fullscreen = 1;
        } else if (!strcmp(argv[i], "--windowed")) {
            fullscreen = 0;
        } else if (!strcmp(argv[i], "--linear")) {
            linear = 1;
        } else if (!strcmp(argv[i], "--nearest")) {
            linear = 0;
        } else if (!strcmp(argv[i], "--display") && i + 1 < argc) {
            display = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--demo") && i + 1 < argc) {
            demo_path = argv[++i];
        } else if (!strcmp(argv[i], "--help")) {
            usage(argv[0]);
            return 0;
        } else if (argv[i][0] == '-' || positional >= 2) {
            usage(argv[0]);
            return 2;
        } else if (positional++ == 0) {
            w = atoi(argv[i]);
        } else {
            h = atoi(argv[i]);
        }
    }

    if (positional == 1 || w <= 0 || h <= 0 || w > INT_MAX / BPP / h) {
        usage(argv[0]);
        return 2;
    }

    int fb_size = w * h * BPP;

    /* Create /tmp/fb0 file if it doesn't exist */
    int fd = open(FB_PATH, O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        perror("open " FB_PATH);
        return 1;
    }

    /* Ensure file is correct size */
    if (ftruncate(fd, fb_size) < 0) {
        perror("ftruncate");
        close(fd);
        return 1;
    }

    /* mmap the file so we can read from it */
    unsigned char *fb = mmap(NULL, fb_size, PROT_READ | PROT_WRITE,
                             MAP_SHARED, fd, 0);
    if (fb == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    /* Clear framebuffer to black on startup */
    memset(fb, 0, fb_size);

    /* Initialize SDL2 */
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        munmap(fb, fb_size);
        close(fd);
        return 1;
    }

    int display_count = SDL_GetNumVideoDisplays();
    if (display < 0 || display >= display_count) {
        fprintf(stderr, "Invalid display %d; available displays: 0-%d\n",
                display, display_count - 1);
        SDL_Quit();
        munmap(fb, fb_size);
        close(fd);
        return 1;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, linear ? "linear" : "nearest");

    Uint32 window_flags = SDL_WINDOW_ALLOW_HIGHDPI;
    if (fullscreen) {
        window_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    }

    SDL_Window *window = SDL_CreateWindow(
        "fbe - Framebuffer Emulator",
        SDL_WINDOWPOS_CENTERED_DISPLAY(display),
        SDL_WINDOWPOS_CENTERED_DISPLAY(display),
        w, h, window_flags
    );
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        munmap(fb, fb_size);
        close(fd);
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        munmap(fb, fb_size);
        close(fd);
        return 1;
    }

    if (SDL_RenderSetLogicalSize(renderer, w, h) < 0) {
        fprintf(stderr, "SDL_RenderSetLogicalSize failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        munmap(fb, fb_size);
        close(fd);
        return 1;
    }
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    /* Create texture: ARGB8888 matches fb0's BGRX format on little-endian */
    SDL_Texture *texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        w, h
    );
    if (!texture) {
        fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        munmap(fb, fb_size);
        close(fd);
        return 1;
    }

    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);

    if (fullscreen) {
        SDL_ShowCursor(SDL_DISABLE);
    }

    pid_t demo_pid = -1;
    if (demo_path) {
        long max_fd = sysconf(_SC_OPEN_MAX);
        if (max_fd < 4) {
            max_fd = 1024;
        }

        demo_pid = fork();
        if (demo_pid < 0) {
            perror("fork");
            exit_code = 1;
            running = 0;
        } else if (demo_pid == 0) {
            close_demo_fds(max_fd);
            execl(demo_path, demo_path, (char *)NULL);
            _exit(127);
        }
    }

    printf("fbe: Displaying %s (%dx%d), %s on display %d\n",
           FB_PATH, w, h, fullscreen ? "fullscreen" : "windowed", display);

    /* Main loop: read from mmap'd fb file and display */
    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) {
                running = 0;
            }
            if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE) {
                running = 0;
            }
        }

        if (demo_pid > 0) {
            int status;
            pid_t result = waitpid(demo_pid, &status, WNOHANG);
            if (result == demo_pid) {
                demo_pid = -1;
                running = 0;
                if (WIFEXITED(status)) {
                    exit_code = WEXITSTATUS(status);
                } else if (WIFSIGNALED(status)) {
                    exit_code = 128 + WTERMSIG(status);
                }
                if (exit_code != 0) {
                    fprintf(stderr, "Demo exited with status %d\n", exit_code);
                }
            } else if (result < 0 && errno != EINTR) {
                perror("waitpid");
                exit_code = 1;
                running = 0;
            }
        }

        /* Update texture from mmap'd framebuffer */
        SDL_UpdateTexture(texture, NULL, fb, w * BPP);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        /* ~60 FPS */
        SDL_Delay(16);
    }

    if (demo_pid > 0) {
        kill(demo_pid, SIGTERM);
        while (waitpid(demo_pid, NULL, 0) < 0 && errno == EINTR) {
        }
    }

    /* Cleanup */
    SDL_ShowCursor(SDL_ENABLE);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    munmap(fb, fb_size);
    close(fd);

    printf("fbe: Exited\n");
    return exit_code;
}
