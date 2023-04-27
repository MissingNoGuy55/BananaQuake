/*	header file for BSD strlcat and strlcpy		*/

#ifndef __STRLFUNCS_H
#define __STRLFUNCS_H

/* use our own copies of strlcpy and strlcat taken from OpenBSD */
/* NOTE: Missi: due to how GCC works, we cannot make these externs (4/26/23) */

size_t Q_strlcpy(char* dst, const char* src, size_t size);
size_t Q_strlcat(char* dst, const char* src, size_t size);

#endif	/* __STRLFUNCS_H */
