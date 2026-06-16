# BMP Display Tools

A small C utility for loading BMP images and displaying them on an e-paper / color EPD framebuffer device.

This project was originally written for Linux framebuffer-based e-paper displays, especially devices exposing an EPDC framebuffer such as `/dev/fb0` with the `mxc_epdc_fb` driver interface.

⭐ If this project is useful to you, please star the repository:
https://github.com/yan2anfei/bmp_display_tools

## Features

- Load BMP files from disk.
- Map BMP files into memory for efficient parsing.
- Convert 24-bit BMP RGB data into grayscale framebuffer data.
- Display images through the Linux framebuffer device.
- Support full-screen EPDC refresh using waveform modes such as `GC16`.
- Provide an optional pixel transfer mode (`-x`) for color EPD pixel layout conversion.
- Include a helper script for displaying all BMP files in the `images_eink` directory.

## Repository Layout

```text
.
├── bmp.c          # BMP file loading and memory mapping
├── bmp.h          # BMP header definitions and pixel transfer logic
├── my_fb.c        # Framebuffer / EPDC display operations
├── my_fb.h        # Framebuffer wrapper interface
├── drv_fb.h       # EPDC framebuffer constants and waveform definitions
├── test_bmp.c     # Command-line test program
├── utils.c        # Utility helpers
├── utils.h
├── debug.h        # Debug logging macros
├── run.sh         # Batch display helper script
├── images/        # Sample or source images
└── images_eink/   # Images prepared for e-paper display
```

## Requirements

- Linux system with framebuffer support.
- An e-paper / EPDC framebuffer device, usually exposed as:

```text
/dev/fb0
```

- A framebuffer driver compatible with `mxc_epdc_fb`.
- GCC or another C compiler.
- Standard Linux system headers, including framebuffer and ioctl support.

The tool is designed for embedded Linux environments and may require platform-specific framebuffer headers or driver definitions.

## Build

Build the test program with:

```bash
make
```

This generates:

```text
test_bmp
```

To remove generated files:

```bash
make clean
```

## Usage

Display a BMP file directly:

```bash
./test_bmp -f path/to/image.bmp
```

Display a BMP file using pixel transfer mode:

```bash
./test_bmp -x -f path/to/image.bmp
```

Show help:

```bash
./test_bmp -h
```

## Pixel Transfer Mode

The `-x` option enables pixel transfer mode.

In normal mode, the tool converts the source BMP image to grayscale and writes it directly to the framebuffer.

In transfer mode, the tool expands the image by 3x in both width and height and rearranges RGB data into a special pixel pattern. This mode is intended for color e-paper display layouts that require custom RGB sub-pixel ordering.

## Batch Display Script

The `run.sh` script loops through all files in the `images_eink` directory and displays them one by one:

```bash
./run.sh
```

The script expects a display binary named `test_bmp_eink`:

```sh
./test_bmp_eink -x -f <image>
```

If your build output is named `test_bmp`, either rename the binary or update `run.sh` accordingly.

## Notes

- The code currently opens `/dev/fb0` directly.
- The framebuffer is configured to 8 bits per pixel and grayscale mode.
- The display rotation is set in `my_fb.c`.
- Full-screen updates use EPDC update ioctl calls.
- BMP input is expected to be uncompressed 24-bit BMP data.
- The code assumes BMP row alignment and manually handles 4-byte row padding.

## Limitations

- The project is platform-specific and depends on EPDC framebuffer ioctl support.
- It is not a general-purpose desktop BMP viewer.
- Only BMP input is supported.
- The current implementation is intended for embedded e-paper display testing and development.
- Some paths in the Makefile and scripts may need to be adjusted for your local environment.

## Example Workflow

```bash
make
./test_bmp -f images_eink/example.bmp
./test_bmp -x -f images_eink/example.bmp
```

## License

No license file is currently included in this repository. Add a license before distributing or reusing the code publicly.
