#ifndef __MY_RECT_H__
#define __MY_RECT_H__

struct my_rect {
	int x;
	int y;
	int h;
	int w;
};

#define INIT_RECT(r, r1, r2, r3, r4) \
	struct my_rect r = { \
		.x = (r1), \
		.y = (r2), \
		.w = (r3), \
		.h = (r4), \
	};

#endif
