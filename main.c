#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <float.h>

#include <time.h>

#include <X11/Xlib.h>

#define abs(v) (v<0?v*-1:v)

#define HEIGHT 1000
#define WIDTH 1000

#define ARR_SIZE (WIDTH)

size_t count_sorted = 0;

int8_t in_bounds(int32_t x, int32_t y, int64_t width, int64_t height);
void gc_put_pixel(void *memory, int32_t x, int32_t y, uint32_t color);
void gc_draw_thick_line(void *memory, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t thickness, uint32_t color);
void gc_fill_rectangle(void *memory, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color);
void update(Display *display, GC *graphics, Window *window, XImage *image);

uint32_t decodeRGB(uint8_t r, uint8_t g, uint8_t b)
{
    return (r << 16) + (g << 8) + b;
}

void fill_rand(int16_t *arr)
{
    for(int i = 0; i < ARR_SIZE; ++i)
        *(arr + i) = rand()%HEIGHT;
}

void min_sort(int16_t *arr)
{
    size_t min_index = count_sorted;
    for(int i = count_sorted + 1; i < ARR_SIZE; ++i) {
        if(*(arr + i) < *(arr + min_index)) min_index = i;
    }
    int16_t temp = *(arr + count_sorted);
    *(arr + count_sorted) = *(arr + min_index);
    *(arr + min_index) = temp;
    if(count_sorted <= ARR_SIZE) count_sorted++;
}

void bubble_sort(int16_t *arr)
{
    for(int i = 1; i < ARR_SIZE; ++i) {
        if(*(arr + i-1) > *(arr + i)) {
            int16_t temp = *(arr + i-1);
            *(arr + i-1) = *(arr + i);
            *(arr + i) = temp;
        }
    }
}

void show_arr(void *memory, int16_t *arr)
{
    int x = 0;
    for(int i = 0; i < ARR_SIZE; ++i) {
        gc_draw_thick_line(memory, x, HEIGHT, x, HEIGHT-*arr++, 0, 0x00FFFFFF);
        x++;
    }
}

int8_t exitloop = 0;
int8_t auto_update = 0;

int main(void)
{
    srand(time(NULL));

    Display *display = XOpenDisplay(NULL);

    int screen = DefaultScreen(display);

    Window window = XCreateSimpleWindow(
        display, RootWindow(display, screen),
        0, 0,
        WIDTH, HEIGHT,
        0, 0,
        0
    );

    char *memory = (char *) malloc(sizeof(uint32_t)*HEIGHT*WIDTH);

    XWindowAttributes winattr = {0};
    XGetWindowAttributes(display, window, &winattr);

    XImage *image = XCreateImage(
        display, winattr.visual, winattr.depth,
        ZPixmap, 0, memory,
        WIDTH, HEIGHT,
        32, WIDTH*4
    );

    GC graphics = XCreateGC(display, window, 0, NULL);

    Atom delete_window = XInternAtom(display, "WM_DELETE_WINDOW", 0);
    XSetWMProtocols(display, window, &delete_window, 1);

    Mask key = KeyPressMask | KeyReleaseMask;
    XSelectInput(display, window, ExposureMask | key);

    XMapWindow(display, window);
    XSync(display, False);

    XEvent event;

    int16_t arr[ARR_SIZE] = {0};

    fill_rand(arr);
    show_arr(memory, arr);

    update(display, &graphics, &window, image);

    while(!exitloop) {
        while(XPending(display) > 0) {
            XNextEvent(display, &event);
            switch(event.type) {
                case Expose: {
                    update(display, &graphics, &window, image);
                    break;
                }
                case ClientMessage: {
                    if((Atom) event.xclient.data.l[0] == delete_window) {
                        exitloop = 1;   
                    }
                    break;
                }
                case KeyPress: {
                    if(event.xkey.keycode == 0x24) {
                        gc_fill_rectangle(memory, 0, 0, WIDTH, HEIGHT, 0x00000000);
                        min_sort(arr);
                        show_arr(memory, arr);
                        update(display, &graphics, &window, image);
                    }
                    if(event.xkey.keycode == 0x41) {
                        auto_update = !auto_update;
                    }
                    break;
                }
            }
        }

        if(auto_update) {
            gc_fill_rectangle(memory, 0, 0, WIDTH, HEIGHT, 0x00000000);
            bubble_sort(arr);
            show_arr(memory, arr);
            update(display, &graphics, &window, image);
        }
    }


    XCloseDisplay(display);

    free(memory);

    return 0;
}

void update(Display *display, GC *graphics, Window *window, XImage *image)
{
    XPutImage(
        display,
        *window,
        *graphics,
        image,
        0, 0,
        0, 0,
        WIDTH, HEIGHT
    );

    XSync(display, False);
}

int8_t in_bounds(int32_t x, int32_t y, int64_t w, int64_t h)
{
    return (x >= 0 && x < w && y >= 0 && y < h);
}

void gc_put_pixel(void *memory, int32_t x, int32_t y, uint32_t color)
{
    if(in_bounds(x, y, WIDTH, HEIGHT))
        *((uint32_t *) memory + y * WIDTH + x) = color;
}

void gc_draw_thick_line(void *memory, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t thickness, uint32_t color)
{
    int dx = x1 - x0;
    int dy = y1 - y0;

    int steps = abs(dy) > abs(dx) ? abs(dy) : abs(dx);

    float X = x0;
    float Y = y0;
    float Xinc = dx / (float) steps;
    float Yinc = dy / (float) steps;

    for(int x = 0; x < steps; ++x) {
        int r = thickness;
        for(int j = -r; j < r; ++j) {
            for(int i = -r; i < r; ++i) {
                int sqr_dist = j*j + i*i;
                if(sqr_dist <= r*r) {
                    gc_put_pixel(memory, (int) X + i, (int) Y + j, color);
                }
            }
        }
        if(thickness == 0) gc_put_pixel(memory, (int) X, (int) Y, color);
        X += Xinc;
        Y += Yinc;
    }
}

void gc_fill_rectangle(void *memory, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color)
{
    for(int j = y0; j < y1; ++j) {
        for(int i = x0; i < x1; ++i) {
            gc_put_pixel(memory, i, j, color);
        }
    }
}