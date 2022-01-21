#pragma once

int GL_LoadPicTexture(qpic_t* pic);
void GL_Upload8_EXT(byte* data, int width, int height, bool mipmap, bool alpha);
void GL_Set2D(void);
