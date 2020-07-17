#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>

#include "my_fb.h"
#include "drv_fb.h"
#include "debug.h"
#include "utils.h"

#define FB_DEV "/dev/fb0"
static struct my_fb *g_fb = NULL;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

static void my_fb_set_pixel(int x, int y, __u8 color)
{
	struct my_fb *mfb = get_fb();
	__u8 *fbp8;

	if (!mfb)
		return;
	fbp8 = (__u8 *)mfb->fb_mem;
	fbp8[y * mfb->var.xres_virtual + x] = color;
}

static void my_fb_clear(void)
{
	struct my_fb *mfb = get_fb();

	if (!mfb)
		return;

	pthread_mutex_lock(&g_mutex);
	memset(mfb->fb_mem, 0xff, mfb->fb_size);
	pthread_mutex_unlock(&g_mutex);
}

static void my_fb_fill_region(struct my_rect *rect, int color, uint target_buf)
{
	int i, left = rect->x, top = rect->y, width = rect->w, height = rect->h;
	unsigned char *fb_ptr;
	struct my_fb *mfb = get_fb();

	if (!mfb)
		return;

	if (target_buf == BUFFER_FB)
		fb_ptr =  (unsigned char*)(mfb->fb_mem);
	else if (target_buf == BUFFER_OVERLAY)
		fb_ptr =  (unsigned char*)(mfb->fb_mem) +
			(mfb->var.xres_virtual *
			 ALIGN_PIXEL_128(mfb->var.yres) *
			 mfb->var.bits_per_pixel / 8) / 4;
	else {
		E("Invalid target buffer specified!\n");
		return;
	}

	pthread_mutex_lock(&g_mutex);
	for (i = 0; i < height; i++) {
		memset(fb_ptr + ((i + top) * mfb->var.xres_virtual + left),
		       color, width);
	}
	pthread_mutex_unlock(&g_mutex);
}

static void my_fb_copy_image_inv(struct my_rect *rect,
				 unsigned char *img_ptr, uint target_buf)
{
	int i, left = rect->x, top = rect->y, width = rect->w, height = rect->h;
	unsigned char *fb_ptr, *buff_ptr;
	unsigned char temp;
	struct my_fb *mfb = get_fb();

	if (!mfb)
		return;

	if (target_buf == BUFFER_FB)
		fb_ptr =  (unsigned char*)(mfb->fb_mem);
	else if (target_buf == BUFFER_OVERLAY)
		fb_ptr =  (unsigned char*)(mfb->fb_mem) +
			(mfb->var.xres_virtual *
			 ALIGN_PIXEL_128(mfb->var.yres) *
			 mfb->var.bits_per_pixel / 8) / 4;
	else {
		E("Invalid target buffer specified!\n");
		return;
	}

	if ((width > mfb->var.xres) || (height > mfb->var.yres)) {
		E("Bad image dimensions!\n");
		return;
	}

	buff_ptr = img_ptr;
	for (i = 0; i < height*width; i++)	{
		temp = *buff_ptr;
		*buff_ptr++ = ~temp;
	}


	pthread_mutex_lock(&g_mutex);
	for (i = 0; i < height; i++) {
		memcpy(fb_ptr + ((i + top) * mfb->var.xres_virtual + left),
		       (unsigned char *)img_ptr + (i * width), width);
	}
	pthread_mutex_unlock(&g_mutex);
}

static void my_fb_copy_image(struct my_rect *rect,
				 unsigned char *img_ptr, uint target_buf)
{
	int i, left = rect->x, top = rect->y, width = rect->w, height = rect->h;
	unsigned char *fb_ptr;
	unsigned char temp;
	struct my_fb *mfb = get_fb();

	if (!mfb)
		return;

	if (target_buf == BUFFER_FB)
		fb_ptr =  (unsigned char*)(mfb->fb_mem);
	else if (target_buf == BUFFER_OVERLAY)
		fb_ptr =  (unsigned char*)(mfb->fb_mem) +
			(mfb->var.xres_virtual *
			 ALIGN_PIXEL_128(mfb->var.yres) *
			 mfb->var.bits_per_pixel / 8) / 4;
	else {
		E("Invalid target buffer specified!\n");
		return;
	}

	if ((width > mfb->var.xres) || (height > mfb->var.yres)) {
		E("Bad image dimensions!\n");
	}

	if (width > mfb->var.xres)
		width = mfb->var.xres;

	if (height > mfb->var.yres)
		height = mfb->var.yres;

	pthread_mutex_lock(&g_mutex);
	for (i = 0; i < height; i++) {
		memcpy(fb_ptr + ((i + top) * mfb->var.xres_virtual + left),
		       (unsigned char *)img_ptr + (i * width), width);
	}
	pthread_mutex_unlock(&g_mutex);
}

static int my_fb_update_fast(struct my_rect *rect, int wave_mode)
{
	struct mxcfb_update_data upd_data;
	int error, ret, max_retry = 10;

	upd_data.dither_mode = 0;
	upd_data.update_mode = UPDATE_MODE_PARTIAL;
	upd_data.waveform_mode = wave_mode;
	upd_data.update_region.left = rect->x;
	upd_data.update_region.width = rect->w;
	upd_data.update_region.top = rect->y;
	upd_data.update_region.height = rect->h;
	upd_data.temp = TEMP_USE_AMBIENT;
	upd_data.flags = 0;

	upd_data.update_marker = 0;

	do {
		/* We have limited memory available for updates, so wait and
		 * then try again after some updates have completed */
		ret = ioctl(g_fb->fd, MXCFB_SEND_UPDATE, &upd_data);
		error = errno;
		if (--max_retry <= 0) {
			E("Max retries exceeded\n");
			break;
		}

		if ((ret < 0) && (error != EFAULT)) {
			usleep(5*100);
		} else if (error == EFAULT)
			do_reboot("tce underrun");
	} while (ret < 0);

	return upd_data.waveform_mode;
}

static int my_fb_update(struct my_rect *rect, int wave_mode,
			int wait_done, uint flags)
{
	struct mxcfb_update_data upd_data;
	struct mxcfb_update_marker_data upd_marker_data;
	int ret, error;
	int wait = wait_done | (flags & EPDC_FLAG_TEST_COLLISION);
	int max_retry = 10;

	upd_data.dither_mode = 0;
	upd_data.update_mode = UPDATE_MODE_PARTIAL;
	upd_data.waveform_mode = wave_mode;
	upd_data.update_region.left = rect->x;
	upd_data.update_region.width = rect->w;
	upd_data.update_region.top = rect->y;
	upd_data.update_region.height = rect->h;
	upd_data.temp = TEMP_USE_AMBIENT;
	upd_data.flags = flags;

/* add overlay mode display */
#if 1
	__u32 ol_phys_addr;

	if (upd_data.flags == EPDC_FLAG_USE_ALT_BUFFER)                {
		ol_phys_addr = g_fb->fix.smem_start +
			g_fb->var.xres_virtual *
			ALIGN_PIXEL_128(g_fb->var.yres) *
			g_fb->var.bits_per_pixel / 8;

		/* Overlay buffer data */
		upd_data.alt_buffer_data.phys_addr = ol_phys_addr;
		upd_data.alt_buffer_data.width = g_fb->var.xres_virtual;
		upd_data.alt_buffer_data.height = g_fb->var.yres;
		upd_data.alt_buffer_data.alt_update_region.left = rect->y;
		upd_data.alt_buffer_data.alt_update_region.width = rect->w;
		upd_data.alt_buffer_data.alt_update_region.top = rect->x;
		upd_data.alt_buffer_data.alt_update_region.height = rect->h;
	}
#endif
	if (wait) {
		/* Get unique marker value */
		g_fb->lock();
		upd_data.update_marker = g_fb->marker_val++;
		g_fb->unlock();
	} else
		upd_data.update_marker = 0;

	do {
		/* We have limited memory available for updates, so wait and
		 * then try again after some updates have completed */
		ret = ioctl(g_fb->fd, MXCFB_SEND_UPDATE, &upd_data);
		error = errno;
		if (--max_retry <= 0) {
			//ioctl(g_fb->fd, FBIOBLANK, FB_BLANK_UNBLANK);
			E("Max retries exceeded,ret=%d\n", ret);
			wait = 0;
			flags = 0;
			break;
		}
		if ((ret < 0) && (error != EFAULT))
			sleep(1);
		else if (error == EFAULT)
			do_reboot("tce underrun");
	} while (ret < 0);

	if (wait) {
		upd_marker_data.update_marker = upd_data.update_marker;

		/* Wait for update to complete */
		ret = ioctl(g_fb->fd, MXCFB_WAIT_FOR_UPDATE_COMPLETE,
			    &upd_marker_data);
		if (ret < 0) {
			E("Wait for update complete failed.  Error = 0x%x",
			  ret);
			flags = 0;
		}
	}

	if (flags & EPDC_FLAG_TEST_COLLISION) {
		E("Collision test result = %d\n",
			upd_marker_data.collision_test);
		return upd_marker_data.collision_test;
	} else
		return upd_data.waveform_mode;
}

static int my_fb_update_full(struct my_rect *rect, int wave_mode,
			int wait_done, uint flags)
{
	struct mxcfb_update_data upd_data;
	struct mxcfb_update_marker_data upd_marker_data;
	int ret, error;
	int wait = wait_done | (flags & EPDC_FLAG_TEST_COLLISION);
	int max_retry = 10;

	upd_data.dither_mode = 0;
	upd_data.update_mode = UPDATE_MODE_FULL;
	upd_data.waveform_mode = wave_mode;
	upd_data.update_region.left = rect->x;
	upd_data.update_region.width = rect->w;
	upd_data.update_region.top = rect->y;
	upd_data.update_region.height = rect->h;
	upd_data.temp = TEMP_USE_AMBIENT;
	upd_data.flags = flags;

#if   1   //add overlay mode display
	__u32 ol_phys_addr;

	if( upd_data.flags == EPDC_FLAG_USE_ALT_BUFFER )		{

		ol_phys_addr = g_fb->fix.smem_start +
			g_fb->var.xres_virtual *
			ALIGN_PIXEL_128(g_fb->var.yres) *
			g_fb->var.bits_per_pixel / 8;

		/* Overlay buffer data */
		upd_data.alt_buffer_data.phys_addr = ol_phys_addr;
		upd_data.alt_buffer_data.width = g_fb->var.xres_virtual;
		upd_data.alt_buffer_data.height = g_fb->var.yres;
		upd_data.alt_buffer_data.alt_update_region.left = rect->x;
		upd_data.alt_buffer_data.alt_update_region.width = rect->w;
		upd_data.alt_buffer_data.alt_update_region.top = rect->y;
		upd_data.alt_buffer_data.alt_update_region.height = rect->h;
	}
#endif

	if (wait) {
		/* Get unique marker value */
		g_fb->lock();
		upd_data.update_marker = g_fb->marker_val++;
		g_fb->unlock();
	} else
		upd_data.update_marker = 0;

	do {
		/* We have limited memory available for updates, so wait and
		 * then try again after some updates have completed */
		ret = ioctl(g_fb->fd, MXCFB_SEND_UPDATE, &upd_data);
		error = errno;
		if (--max_retry <= 0) {
			//ioctl(g_fb->fd, FBIOBLANK, FB_BLANK_UNBLANK);
			E("Max retries exceeded\n");
			wait = 0;
			flags = 0;
			break;
		}
		if ((ret < 0) && (error != EFAULT))
			sleep(1);
		else if (error == EFAULT)
			do_reboot("tce underrun");
	} while (ret < 0);

	if (wait) {
		upd_marker_data.update_marker = upd_data.update_marker;

		/* Wait for update to complete */
		ret = ioctl(g_fb->fd, MXCFB_WAIT_FOR_UPDATE_COMPLETE,
			       &upd_marker_data);
		if (ret < 0) {
			D("Wait for update complete failed.  Error = 0x%x",
			  ret);
			flags = 0;
		}
	}

	if (flags & EPDC_FLAG_TEST_COLLISION) {
		D("Collision test result = %d\n",
			upd_marker_data.collision_test);
		return upd_marker_data.collision_test;
	} else
		return upd_data.waveform_mode;

}

static void my_fb_lock(void)
{
	pthread_mutex_lock(&g_fb->mutex);
}

static void my_fb_unlock(void)
{
	pthread_mutex_unlock(&g_fb->mutex);
}

struct my_fb *get_fb(void)
{
	int ret;
	void *fb_mem;

	if (g_fb)
		return g_fb;
	pthread_mutex_lock(&mutex);
	if (!g_fb) {
		g_fb = (typeof(g_fb))malloc(sizeof(*g_fb));
		if (!g_fb) {
			E("malloc fb failed,errno=%d(%s)\n", errno,
			  strerror(errno));
			goto err;
		}
		memset(g_fb, 0, sizeof(*g_fb));

		g_fb->marker_val = 1;
		pthread_mutex_init(&g_fb->mutex, NULL);
		g_fb->set_pixel = my_fb_set_pixel;
		g_fb->clear = my_fb_clear;
		g_fb->update = my_fb_update;
		g_fb->update_fast = my_fb_update_fast;
		g_fb->update_full = my_fb_update_full;
		g_fb->copy_image_inv = my_fb_copy_image_inv;
		g_fb->copy_image = my_fb_copy_image;
		g_fb->fill_region = my_fb_fill_region;
		g_fb->lock = my_fb_lock;
		g_fb->unlock = my_fb_unlock;

		g_fb->fd = open(FB_DEV, O_RDWR, 0);
		if (g_fb->fd < 0) {
			E("open %s failed,errno=%d(%s)\n", FB_DEV, errno,
			  strerror(errno));
			goto err_open;
		}

		ret = ioctl(g_fb->fd, FBIOGET_FSCREENINFO, &g_fb->fix);
		if (ret < 0) {
			E("get screen fixed info failed,errno=%d(%s)\n",
			  errno, strerror(errno));
			goto err_ioctl;
		}
		if (strncmp(g_fb->fix.id, EPDC_STR_ID, strlen(EPDC_STR_ID))) {
			E("not epdc fb(%s)!\n", g_fb->fix.id);
			goto err_ioctl;
		}

		ret = ioctl(g_fb->fd, FBIOGET_VSCREENINFO, &g_fb->var);
		if (ret < 0) {
			E("get screen var info failed,errno=%d(%s)\n",
			  errno, strerror(errno));
			goto err_ioctl;
		}
		g_fb->fb_size = g_fb->var.xres_virtual *
			g_fb->var.yres_virtual * g_fb->var.bits_per_pixel / 8;
#if 0
		fb_mem = mmap(0, g_fb->fb_size,
			   PROT_READ | PROT_WRITE,
			   MAP_SHARED, g_fb->fd, 0);
		if (fb_mem) {
			D("Clean early fbmem(%dbytes)\n", g_fb->fb_size);
			memset(fb_mem, 0xffffffff, g_fb->fb_size);
			munmap(fb_mem, g_fb->fb_size);
			fb_mem = NULL;
		}
#endif

		D("Set the background to 8-bpp\n");
		g_fb->var.bits_per_pixel = 8;
		g_fb->var.grayscale = GRAYSCALE_8BIT;
		g_fb->var.yoffset = 0;
		g_fb->var.rotate = 3; //FB_ROTATE_UD;
		g_fb->var.activate = FB_ACTIVATE_FORCE;
		ret = ioctl(g_fb->fd, FBIOPUT_VSCREENINFO, &g_fb->var);
		if (ret < 0) {
			E("set screen info failed,errno=%d(%s)\n",
			  errno, strerror(errno));
			goto err_ioctl;
		}
		g_fb->fb_size = g_fb->var.xres_virtual *
			g_fb->var.yres_virtual * g_fb->var.bits_per_pixel / 8;
		D("screen_info.xres = %d\nscreen_info.yres = %d\n",
		  g_fb->var.xres, g_fb->var.yres);
		D("screen_info.xres_virtual=%d\n"
		  "screen_info.yres_virtual=%d\n"
		  "screen_info.bits_per_pixel=%d\n",
		  g_fb->var.xres_virtual,
		  g_fb->var.yres_virtual,
		  g_fb->var.bits_per_pixel);

		D("Mem-Mapping FB\n");
		g_fb->fb_mem = mmap(0, g_fb->fb_size,
			   PROT_READ | PROT_WRITE,
			   MAP_SHARED, g_fb->fd, 0);
		if (!g_fb->fb_mem) {
			E("map fb mem failed,errno=%d(%s)\n",
			  errno, strerror(errno));
			goto err_ioctl;
		}

		D("Set to region update mode\n");
		int auto_update_mode = AUTO_UPDATE_MODE_REGION_MODE;
		ret = ioctl(g_fb->fd, MXCFB_SET_AUTO_UPDATE_MODE,
			    &auto_update_mode);
		if (ret < 0) {
			E("failed to set update mode.\n");
			goto err_setmode;
		}

		int update_mode = UPDATE_SCHEME_QUEUE_AND_MERGE;
		D("Set update scheme - %d\n", update_mode);
		ret = ioctl(g_fb->fd, MXCFB_SET_UPDATE_SCHEME, &update_mode);
		if (ret < 0) {
			E("failed to set update scheme.\n");
			goto err_setmode;
		}

		__u32 pwrdown_delay = 500;
		D("Set pwrdown_delay - %d\n", pwrdown_delay);
		ret = ioctl(g_fb->fd, MXCFB_SET_PWRDOWN_DELAY, &pwrdown_delay);
		if (ret < 0) {
			E("failed to set power down delay.\n");
			goto err_setmode;
		}

		D("Blank screen - auto-selected to DU\n");
		memset(g_fb->fb_mem, 0xffffffff, g_fb->fb_size);
		//ioctl(g_fb->fd, FBIOBLANK, FB_BLANK_POWERDOWN);
		ioctl(g_fb->fd, FBIOBLANK, FB_BLANK_UNBLANK);
#if 0
		INIT_RECT(rect, 0, 0, g_fb->var.xres, g_fb->var.yres);
		D("Update to display.\n");
		g_fb->update_full(&rect, WAVEFORM_MODE_GC16, 1, 0);
#endif
	}
	pthread_mutex_unlock(&mutex);

	return g_fb;

err_setmode:
	munmap(g_fb->fb_mem, g_fb->fb_size);
	g_fb->fb_mem = NULL;
err_ioctl:
	close(g_fb->fd);
	g_fb->fd = -1;
err_open:
	free(g_fb);
	g_fb = NULL;
err:
	pthread_mutex_unlock(&mutex);
	return NULL;
}

void destroy_fb(void)
{
	pthread_mutex_lock(&mutex);
	if (!g_fb)
		goto out;

	pthread_mutex_destroy(&g_fb->mutex);
	munmap(g_fb->fb_mem, g_fb->fb_size);
	g_fb->fb_mem = NULL;
	close(g_fb->fd);
	g_fb->fd = -1;
	free(g_fb);
	g_fb = NULL;
out:
	pthread_mutex_unlock(&mutex);
}
