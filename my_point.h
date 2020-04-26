#ifndef __MY_POINT_H__
#define __MY_POINT_H__

struct my_point {
	int x;
	int y;
};

#define INIT_POINT(p, p1, p2) \
	struct my_point p = { \
		.x = (p1), \
		.y = (p2), \
	};

#endif
