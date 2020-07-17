#ifndef __BMP_H__
#define __BMP_H__
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "debug.h"

struct bmp_hdr {
	unsigned short bfType;
	int bfSize;
	unsigned short bfReserved1;
	unsigned short bfReserved2;
	int bfOffBits;
} __attribute__((packed));

struct bmp_info_hdr {
	int  biSize;
	int   biWidth;
	int   biHeight;
	short   biPlanes;
	short   biBitCount;
	int  biCompression;
	int  biSizeImage;
	int   biXPelsPerMeter;
	int   biYPelsPerMeter;
	int  biClrUsed;
	int biClrImportant;
} __attribute__((packed));

struct bmp_handle {
	int fd, size;
	struct bmp_hdr *hdr;
};

static inline void dump_bmp(struct bmp_handle *handle)
{
	int i, j, fd;
	struct bmp_hdr *hdr = handle->hdr;
	struct bmp_info_hdr *info = (typeof(info))((char *)(handle->hdr) +
						   sizeof(*hdr));
	unsigned char *buf = (char *)(handle->hdr) + hdr->bfOffBits, *data;

	printf("bfType : 0x%04x\n", hdr->bfType);
	printf("bfSize : %d\n", hdr->bfSize);
	printf("bfOffBits : %d\n", hdr->bfOffBits);

	printf("width: %d\n", info->biWidth);
	printf("height: %d\n", info->biHeight);
#if 0
	fd = open("./raw.bin", O_CREAT|O_TRUNC|O_RDWR, 0664);
	if (fd >= 0) {
		write(fd, buf, info->biWidth * info->biHeight * 3);
		close(fd);
		fd = -1;
	}
	printf("r:%02x,g:%02x,b:%02x\n", buf[2], buf[1], buf[0]);
	printf("r:%02x,g:%02x,b:%02x\n", buf[5], buf[4], buf[3]);
#endif
#if 0
	for (i = 0; i < info->biWidth; i++) {
		for (j = 0; j < info->biHeight; j++) {
			data = buf + i * info->biHeight * 3 + j * 3;
			printf("0x%02x,0x%02x,0x%02x", data[2], data[1],
			       data[0]);
			if (j != info->biHeight - 1)
				printf(" ");
		}
		printf("\n");
	}
#endif
}

static inline int xfer_bmp(struct bmp_handle *handle, void *out, int size,
			    int width, int height)
{
	int i, j, k, fd;
	struct bmp_hdr *hdr = handle->hdr;
	struct bmp_info_hdr *info = (typeof(info))((char *)(handle->hdr) +
						   sizeof(*hdr));
	unsigned char *buf = (char *)(handle->hdr) + hdr->bfOffBits,
		      *dst = out, *src;
	unsigned char R, G, B;
	int line_stripe = info->biWidth * 3;

	//bmp 行宽 4 字节对齐
	while (line_stripe%4)
		line_stripe++ ;

	if (size < (info->biWidth * info->biHeight) * 9) {
		E("out bmp buffer too small.\n");
		return -1;
	}

	for (i = 0; i < height ; i++) { 
		switch ( (i % 9) / 3 ) {
		case 0:
			k = 0;
			break;
		case 1:
			k = 1;
			break;
		case 2:
			k = 2;
			break;
		default:
			printf("wrong i.\n");
			break;
		}

		for (j = 0; j < width ; j+=3 ) {
		src = buf + i / 3 * line_stripe + j;
			R = src[2];
			G = src[1];
			B = src[0];

			switch (k) {
			case 0:
				dst[i * width + j] = R;
				dst[i * width + j + 1] = B;
				dst[i * width + j + 2] = G;
				break;
			case 1:
				dst[i * width + j] = G;
				dst[i * width + j + 1] = R;
				dst[i * width + j + 2] = B;
				break;
			case 2:
				dst[i * width + j] = B;
				dst[i * width + j + 1] = G;
				dst[i * width + j + 2] = R;
				break;
			default:
				printf("wrong k.\n");
				break;
			}
		}
	}

#if 0
	fd = open("./xfer.bin", O_CREAT|O_TRUNC|O_RDWR, 0664);
	if (fd >= 0) {
		write(fd, out, size);
		close(fd);
		fd = -1;
	}
#endif
	return 0;
}

struct bmp_handle *load_bmp(char *path);
void free_bmp(struct bmp_handle *handle);

#endif
