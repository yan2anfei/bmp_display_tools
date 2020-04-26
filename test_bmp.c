#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>

#include "my_fb.h"
#include "bmp.h"

static inline void do_show_raw_bmp(struct bmp_handle *handle)
{
	struct bmp_hdr *hdr = handle->hdr;
	struct bmp_info_hdr *info = (typeof(info))
		((char *)(hdr) + sizeof(*hdr));
	struct my_fb *mfb = get_fb();
	unsigned char *buf, *data, r, g, b;
	struct my_rect rect;
	int i, j;

	buf = malloc(info->biWidth * info->biHeight);
	if (!buf)
		return;

	for (i = 0; i < info->biHeight; i++) {
		for (j = 0; j < info->biWidth; j++) {
			data = (char *)hdr + hdr->bfOffBits +
				i * info->biWidth * 3 +
				j * 3;
			r = data[2];
			g = data[1];
			b = data[0];
			buf[i * info->biWidth + j] =
				(299 * r) / 1000 +
				(587 * g) / 1000 +
				(114 * b) / 1000;
		}
	}

	rect.x = rect.y = 0;
	rect.w = info->biWidth;
	rect.h = info->biHeight;
	mfb->clear();
	mfb->copy_image(&rect, buf, BUFFER_FB);
	free(buf);
	mfb->update_full(&rect, WAVEFORM_MODE_GC16, 1, 0);
}

static inline void do_show_xfer_bmp(struct bmp_handle *handle)
{
	struct bmp_hdr *hdr = handle->hdr;
	struct bmp_info_hdr *info = (typeof(info))
		((char *)(hdr) + sizeof(*hdr));
	struct my_fb *mfb = get_fb();
	unsigned char *buf;
	struct my_rect rect;

	buf = malloc(info->biWidth * info->biHeight * 4);
	if (!buf)
		return;
	xfer_bmp(handle, buf, info->biWidth * info->biHeight * 4,
		 info->biWidth * 2, info->biHeight * 2);
	rect.x = rect.y = 0;
	rect.w = info->biWidth * 2;
	rect.h = info->biHeight * 2;
	mfb->clear();
	mfb->copy_image(&rect, buf, BUFFER_FB);
	free(buf);
	mfb->update_full(&rect, WAVEFORM_MODE_GC16, 1, 0);
}

static void show_bmp(char *file, bool xfer)
{
	struct bmp_handle *handle = load_bmp(file);

	if (!handle) {
		E("load bitmap failed.\n");
		return;
	}

	if (xfer)
		do_show_xfer_bmp(handle);
	else
		do_show_raw_bmp(handle);

	free_bmp(handle);
	handle = NULL;
}

int main(int argc, char **argv)
{
	int opt;
	char *file = NULL;
	bool xfer = false;

	while ((opt = getopt(argc, argv, "f:hx")) != -1) {
		switch (opt) {
		case 'x':
			xfer = true;
			break;
		case 'f':
			file = optarg;
			break;
		case 'h':
		default:
			fprintf(stderr, "Usage: %s [-x] -f xx.bmp\n", argv[0]);
			fprintf(stderr, "-x xfer pixels.\n");
			exit(0);
		}
	}

	if (!file) {
		fprintf(stderr, "Usage: %s [-x] -f xx.bmp\n", argv[0]);
		fprintf(stderr, "x means xfer pixels.\n");
		exit(0);
	}

	show_bmp(file, xfer);

	destroy_fb();
	return 0;
}
