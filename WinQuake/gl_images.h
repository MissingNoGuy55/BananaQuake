#pragma once

struct tgaheader_t
{
    unsigned char idlength;
    unsigned char colortype;
    unsigned char imagetype;

    unsigned short colormap_first_entry;
    unsigned short colormap_length;
    unsigned char colormap_entrysize;

    unsigned short x_origin;
    unsigned short y_origin;
    unsigned short width;
    unsigned short height;
    unsigned char pixel_depth;
    unsigned char pixel_descriptor;
};

constexpr size_t TARGA_HEADER_SIZE = sizeof(tgaheader_t);

extern byte* LoadTGA(FILE* fin);
extern byte* LoadPCX(FILE* f);
extern byte* targa_rgba;
extern byte* pcx_rgb;
