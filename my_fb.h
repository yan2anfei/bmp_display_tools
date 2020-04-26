#ifndef __MY_FB_H__
#define __MY_FB_H__

#include <linux/mxcfb.h>
#include <sys/types.h>
#include <pthread.h>

#include "my_rect.h"
#include "drv_fb.h"

struct my_fb {
	int fd;
	int fb_size;
	void *fb_mem;
	struct fb_var_screeninfo var;
	struct fb_fix_screeninfo fix;
	__u32 marker_val;
	pthread_mutex_t mutex;
	void (*lock)(void);
	void (*unlock)(void);
	int (*update)(struct my_rect *, int, int, uint);
	int (*update_fast)(struct my_rect *, int);
	int (*update_full)(struct my_rect *, int, int, uint);
	void (*copy_image_inv)(struct my_rect *, unsigned char *, uint);
	void (*copy_image)(struct my_rect *, unsigned char *, uint);
	void (*fill_region)(struct my_rect *, int, uint);
	void (*clear)(void);
	void (*set_pixel)(int x, int y, __u8 color);
};

struct my_fb *get_fb(void);
void destroy_fb(void);

#endif
