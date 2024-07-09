#pragma once

typedef struct _TargaHeader {
    unsigned char 	id_length, colormap_type, image_type;
    unsigned short	colormap_index, colormap_length;
    unsigned char	colormap_size;
    unsigned short	x_origin, y_origin, width, height;
    unsigned char	pixel_size, attributes;
} TargaHeader;

constexpr size_t TARGA_HEADER_SIZE = sizeof(TargaHeader);

extern TargaHeader  targa_header;

extern byte* LoadTGA(FILE* fin);
extern byte* LoadPCX(FILE* f);
extern byte* targa_rgba;
extern byte* pcx_rgb;
