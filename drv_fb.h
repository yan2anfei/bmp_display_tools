#ifndef _DRV_FB_H_
#define _DRV_FB_H_

#define BUFFER_FB		0
#define BUFFER_OVERLAY		1

#define WAVEFORM_MODE_INIT	0x0	/* Screen goes to white (clears) */
#define WAVEFORM_MODE_DU	0x1	/* Grey->white/grey->black */
#define WAVEFORM_MODE_GC16	0x2	/* High fidelity (flashing) */
#define WAVEFORM_MODE_GL16	0x3	/* High fidelity (no flashing) */
#if 0
	#define WAVEFORM_MODE_A2  	0x6	/* Fast black/white animation */
#else
	#define WAVEFORM_MODE_A2  0x4	/* Fast black/white animation */
#endif
#define WAVEFORM_MODE_DU4	0x7

#define EPDC_STR_ID		"mxc_epdc_fb"

#define  ALIGN_PIXEL_128(x)  ((x+ 127) & ~127)

#define CROSSHATCH_ALTERNATING  0
#define CROSSHATCH_COLUMNS_ROWS 1

#define ALLOW_COLLISIONS	0
#define NO_COLLISIONS		1

struct hw_dithering {
	int dither_mode;
	int quant_bit;
} hw_dithering;


#endif /* _DRV_FB_H_ */

