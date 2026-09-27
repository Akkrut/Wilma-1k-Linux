/* wilma: No libc, no manners. ebx is the screen. somehow. */

#include "syscall.h"

#ifndef WIDTH
#define WIDTH 1024
#endif
#ifndef HEIGHT
#define HEIGHT 768
#endif

#define PIXEL_DEBT (WIDTH * HEIGHT)
#define BYTE_DAMAGE (PIXEL_DEBT * 4)

#define MID_X (WIDTH >> 1)
#define MID_Y (HEIGHT >> 1)

#define FD_FB 3
#define FD_TIMER 4

#define CURFEW 4500
#define WORM_TAX 1000
#define REWIND_MISERY 3500

#define SPARKLE_LIES 1024

static inline void spit_aplay(const void *p) {
    __asm__ volatile(
        "push %%ebx\n"
        "push $4\n"
        "pop %%eax\n"
        "push $6\n"
        "pop %%ebx\n"
        "int $0x80\n"
        "pop %%ebx"
        :
        : "c"(p), "d"(160)
        : "eax", "memory"
    );
}

/* Fake sine. good enough for government portals.
   Special thanks to government and Pettri O. for 35 mil euro funding!
   Actually they funded wrong project, Rauha 4k demo is using government level portals.
   This is about school message system but who cares, it's not that much. */
static int fake_sin(int x) {
    x &= 255;
    int h = x - 128;
    return 127 - ((h * h) >> 7);
}

void _start(void) {
    unsigned int *wall;
    __asm__("" : "=b"(wall));

    int tick = 0;

    for (;;) {
        char paperwork[8];
        sys_read(FD_TIMER, paperwork, 8);

        if (sys_read(0, paperwork, 1) == 1 && paperwork[0] == 0x1B)
            break;

        if (++tick >= CURFEW)
            break;

        if (tick < WORM_TAX || tick >= REWIND_MISERY) {
            /* stars. because originality died before this linux booted. */

            for (int grave = 0; grave < PIXEL_DEBT; grave++)
                wall[grave] = 0;

            int nostalgia = tick < WORM_TAX ? tick : CURFEW - tick;

            for (int victim = 0; victim < SPARKLE_LIES; victim++) {
                unsigned int oracle = victim * 2654435761u;
                int liex = (int)(oracle & 1023) - MID_X;
                int liey = (int)((oracle >> 10 ^ oracle >> 21) & 1023) - MID_Y;
                unsigned int rent = ((oracle >> 15) - nostalgia) & 255;
                if (rent < 4) continue;

                int hitx = MID_X + (liex << 8) / (int)rent;
                int hity = MID_Y + (liey << 8) / (int)rent;

                if ((unsigned)hitx < (unsigned)(WIDTH - 2) && (unsigned)hity < (unsigned)(HEIGHT - 2)) {
                    unsigned int brag = 255 - rent;
                    brag -= (((tick >> 3) + oracle) & 15);
                    brag = brag * nostalgia >> 9;
                    unsigned int mood = (oracle >> 24) + (tick >> 3);
                    unsigned int bruise = mood & 15;
                    unsigned int makeup = (((brag - bruise) & 0xFF) << 16) | (((brag ^ (mood & 7)) & 0xFF) << 8) | ((brag + bruise) & 0xFF);
                    unsigned int *p = wall + hity * WIDTH + hitx;
                    p[0] = (rent < 64) ? (0xFFFFFF ^ (oracle & 0xFFFF)) : makeup;
                    p[1] = makeup;
                    p[WIDTH] = makeup;
                    p[WIDTH + 1] = makeup;
                }
            }

        } else {
            /* TUNNEL. mandatory by ancient bored-nerd law.
               Worm Hole BBS, 2 Node ISDN, 90 878 XX XX  */

            /* Bytebeat #91, now with fewer bits stuck to the ceiling. */
            /* Let the aplay drain before the closing stars. */
            if (tick < REWIND_MISERY - 150) {
                unsigned char *throat = (unsigned char *)wall;
                int ache = tick * 160;
                for (int i = 0; i < 160; i++, ache++) {
                    throat[i] = ache * (0xE5B72D >> (ache >> 11 & 15) & 7) + (ache >> 8);
                }
                spit_aplay(throat);
            }

            int drunk_x = MID_X + fake_sin(tick) + (tick & 3);
            int drunk_y = MID_Y + fake_sin(tick * 3 + 64) + (tick & 7);

            for (int y = 0; y < HEIGHT; y++) {
                for (int x = 0; x < WIDTH; x++) {
                    int sideeye = x - drunk_x;
                    int downer = y - drunk_y;

                    int taxx = sideeye < 0 ? -sideeye : sideeye;
                    int taxy = downer < 0 ? -downer : downer;

                    unsigned int fake_r = taxx + taxy - ((taxx < taxy ? taxx : taxy) >> 1);
                    fake_r += fake_r < 2;

                    unsigned int greed = 8192 / fake_r;

                    unsigned int crawl = greed + tick * 4;

                    int angleish = (downer * (int)greed) >> 7;
                    if (sideeye < 0) angleish = 128 - angleish;
                    unsigned int spin = (unsigned)(angleish + tick) & 255;

                    unsigned int goo = (crawl ^ spin ^ (crawl >> 3)) & 0xFF;
                    unsigned int makeup = (goo << 16) | (((goo + tick) & 0xFF) >> 1) << 8 | ((spin + y) >> 2 & 0xFF);

                    unsigned int gz = 200 + (tick * 3 & 255);
                    if (fake_r < gz) {
                        unsigned int glow = gz - fake_r;
                        glow &= 255;
                        makeup |= (glow << 16) | (glow << 8) | (glow >> 1);
                    }
                    wall[y * WIDTH + x] = makeup;
                }
            }

        }

        sys_pwrite64(FD_FB, wall, BYTE_DAMAGE);
    }

    /* Restore canonical input and echo before leaving the VT. Modern art is not worth it. */
    __asm__ volatile(
        "push $54\n"
        "pop %%eax\n"
        "mov $0x5401, %%ecx\n"
        "mov %%esp, %%edx\n"
        "int $0x80\n"
        "orb $10, 12(%%esp)\n"
        "inc %%ecx\n"
        "mov $54, %%al\n"
        "int $0x80\n"
        "push $1\n"
        "pop %%eax\n"
        "int $0x80"
        :
        : "b"(0)
        : "eax", "ecx", "edx", "memory"
    );
    __builtin_unreachable();
}
