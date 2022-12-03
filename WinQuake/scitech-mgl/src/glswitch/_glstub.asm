;****************************************************************************
;*
;*                      SciTech OpenGL Switching Library
;*
;*  ========================================================================
;*
;*   Copyright (C) 1991-2004 SciTech Software, Inc. All rights reserved.
;*
;*   This file may be distributed and/or modified under the terms of the
;*   GNU General Public License version 2.0 as published by the Free
;*   Software Foundation and appearing in the file LICENSE.GPL included
;*   in the packaging of this file.
;*
;*   Licensees holding a valid Commercial License for this product from
;*   SciTech Software, Inc. may use this file in accordance with the
;*   Commercial License Agreement provided with the Software.
;*
;*   This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
;*   THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
;*   PURPOSE.
;*
;*   See http://www.scitechsoft.com/license/ for information about
;*   the licensing options available and how to purchase a Commercial
;*   License Agreement.
;*
;*   Contact license@scitechsoft.com if any conditions of this licensing
;*   are not clear to you, or you have questions about licensing options.
;*
;*  ========================================================================
;*
;* Language:    NASM
;* Environment: Intel x86
;*
;* Description: OpenGL stub functions to call dynamically linked OpenGL
;*              code in any version of OpenGL on the system.
;*
;****************************************************************************

include "scitech.mac"           ; Memory model macros

BEGIN_IMPORTS_DEF   _GLS_glFuncs
DECLARE_STDCALL glAccum,8
DECLARE_STDCALL glAlphaFunc,8
DECLARE_STDCALL glAreTexturesResident,12
DECLARE_STDCALL glArrayElement,4
DECLARE_STDCALL glBegin,4
DECLARE_STDCALL glBindTexture,8
DECLARE_STDCALL glBitmap,28
DECLARE_STDCALL glBlendFunc,8
DECLARE_STDCALL glCallList,4
DECLARE_STDCALL glCallLists,12
DECLARE_STDCALL glClear,4
DECLARE_STDCALL glClearAccum,16
DECLARE_STDCALL glClearColor,16
DECLARE_STDCALL glClearDepth,8
DECLARE_STDCALL glClearIndex,4
DECLARE_STDCALL glClearStencil,4
DECLARE_STDCALL glClipPlane,8
DECLARE_STDCALL glColor3b,12
DECLARE_STDCALL glColor3bv,4
DECLARE_STDCALL glColor3d,24
DECLARE_STDCALL glColor3dv,4
DECLARE_STDCALL glColor3f,12
DECLARE_STDCALL glColor3fv,4
DECLARE_STDCALL glColor3i,12
DECLARE_STDCALL glColor3iv,4
DECLARE_STDCALL glColor3s,12
DECLARE_STDCALL glColor3sv,4
DECLARE_STDCALL glColor3ub,12
DECLARE_STDCALL glColor3ubv,4
DECLARE_STDCALL glColor3ui,12
DECLARE_STDCALL glColor3uiv,4
DECLARE_STDCALL glColor3us,12
DECLARE_STDCALL glColor3usv,4
DECLARE_STDCALL glColor4b,16
DECLARE_STDCALL glColor4bv,4
DECLARE_STDCALL glColor4d,32
DECLARE_STDCALL glColor4dv,4
DECLARE_STDCALL glColor4f,16
DECLARE_STDCALL glColor4fv,4
DECLARE_STDCALL glColor4i,16
DECLARE_STDCALL glColor4iv,4
DECLARE_STDCALL glColor4s,16
DECLARE_STDCALL glColor4sv,4
DECLARE_STDCALL glColor4ub,16
DECLARE_STDCALL glColor4ubv,4
DECLARE_STDCALL glColor4ui,16
DECLARE_STDCALL glColor4uiv,4
DECLARE_STDCALL glColor4us,16
DECLARE_STDCALL glColor4usv,4
DECLARE_STDCALL glColorMask,16
DECLARE_STDCALL glColorMaterial,8
DECLARE_STDCALL glColorPointer,16
DECLARE_STDCALL glCopyPixels,20
DECLARE_STDCALL glCopyTexImage1D,28
DECLARE_STDCALL glCopyTexImage2D,32
DECLARE_STDCALL glCopyTexSubImage1D,24
DECLARE_STDCALL glCopyTexSubImage2D,32
DECLARE_STDCALL glCullFace,4
DECLARE_STDCALL glDeleteLists,8
DECLARE_STDCALL glDeleteTextures,8
DECLARE_STDCALL glDepthFunc,4
DECLARE_STDCALL glDepthMask,4
DECLARE_STDCALL glDepthRange,16
DECLARE_STDCALL glDisable,4
DECLARE_STDCALL glDisableClientState,4
DECLARE_STDCALL glDrawArrays,12
DECLARE_STDCALL glDrawBuffer,4
DECLARE_STDCALL glDrawElements,16
DECLARE_STDCALL glDrawPixels,20
DECLARE_STDCALL glEdgeFlag,4
DECLARE_STDCALL glEdgeFlagPointer,8
DECLARE_STDCALL glEdgeFlagv,4
DECLARE_STDCALL glEnable,4
DECLARE_STDCALL glEnableClientState,4
DECLARE_STDCALL glEnd,0
DECLARE_STDCALL glEndList,0
DECLARE_STDCALL glEvalCoord1d,8
DECLARE_STDCALL glEvalCoord1dv,4
DECLARE_STDCALL glEvalCoord1f,4
DECLARE_STDCALL glEvalCoord1fv,4
DECLARE_STDCALL glEvalCoord2d,16
DECLARE_STDCALL glEvalCoord2dv,4
DECLARE_STDCALL glEvalCoord2f,8
DECLARE_STDCALL glEvalCoord2fv,4
DECLARE_STDCALL glEvalMesh1,12
DECLARE_STDCALL glEvalMesh2,20
DECLARE_STDCALL glEvalPoint1,4
DECLARE_STDCALL glEvalPoint2,8
DECLARE_STDCALL glFeedbackBuffer,12
DECLARE_STDCALL glFinish,0
DECLARE_STDCALL glFlush,0
DECLARE_STDCALL glFogf,8
DECLARE_STDCALL glFogfv,8
DECLARE_STDCALL glFogi,8
DECLARE_STDCALL glFogiv,8
DECLARE_STDCALL glFrontFace,4
DECLARE_STDCALL glFrustum,48
DECLARE_STDCALL glGenLists,4
DECLARE_STDCALL glGenTextures,8
DECLARE_STDCALL glGetBooleanv,8
DECLARE_STDCALL glGetClipPlane,8
DECLARE_STDCALL glGetDoublev,8
DECLARE_STDCALL glGetError,0
DECLARE_STDCALL glGetFloatv,8
DECLARE_STDCALL glGetIntegerv,8
DECLARE_STDCALL glGetLightfv,12
DECLARE_STDCALL glGetLightiv,12
DECLARE_STDCALL glGetMapdv,12
DECLARE_STDCALL glGetMapfv,12
DECLARE_STDCALL glGetMapiv,12
DECLARE_STDCALL glGetMaterialfv,12
DECLARE_STDCALL glGetMaterialiv,12
DECLARE_STDCALL glGetPixelMapfv,8
DECLARE_STDCALL glGetPixelMapuiv,8
DECLARE_STDCALL glGetPixelMapusv,8
DECLARE_STDCALL glGetPointerv,8
DECLARE_STDCALL glGetPolygonStipple,4
DECLARE_STDCALL glGetString,4
DECLARE_STDCALL glGetTexEnvfv,12
DECLARE_STDCALL glGetTexEnviv,12
DECLARE_STDCALL glGetTexGendv,12
DECLARE_STDCALL glGetTexGenfv,12
DECLARE_STDCALL glGetTexGeniv,12
DECLARE_STDCALL glGetTexImage,20
DECLARE_STDCALL glGetTexLevelParameterfv,16
DECLARE_STDCALL glGetTexLevelParameteriv,16
DECLARE_STDCALL glGetTexParameterfv,12
DECLARE_STDCALL glGetTexParameteriv,12
DECLARE_STDCALL glHint,8
DECLARE_STDCALL glIndexMask,4
DECLARE_STDCALL glIndexPointer,12
DECLARE_STDCALL glIndexd,8
DECLARE_STDCALL glIndexdv,4
DECLARE_STDCALL glIndexf,4
DECLARE_STDCALL glIndexfv,4
DECLARE_STDCALL glIndexi,4
DECLARE_STDCALL glIndexiv,4
DECLARE_STDCALL glIndexs,4
DECLARE_STDCALL glIndexsv,4
DECLARE_STDCALL glIndexub,4
DECLARE_STDCALL glIndexubv,4
DECLARE_STDCALL glInitNames,0
DECLARE_STDCALL glInterleavedArrays,12
DECLARE_STDCALL glIsEnabled,4
DECLARE_STDCALL glIsList,4
DECLARE_STDCALL glIsTexture,4
DECLARE_STDCALL glLightf,12
DECLARE_STDCALL glLightfv,12
DECLARE_STDCALL glLighti,12
DECLARE_STDCALL glLightiv,12
DECLARE_STDCALL glLightModelf,8
DECLARE_STDCALL glLightModelfv,8
DECLARE_STDCALL glLightModeli,8
DECLARE_STDCALL glLightModeliv,8
DECLARE_STDCALL glLineStipple,8
DECLARE_STDCALL glLineWidth,4
DECLARE_STDCALL glListBase,4
DECLARE_STDCALL glLoadIdentity,0
DECLARE_STDCALL glLoadMatrixd,4
DECLARE_STDCALL glLoadMatrixf,4
DECLARE_STDCALL glLoadName,4
DECLARE_STDCALL glLogicOp,4
DECLARE_STDCALL glMap1d,32
DECLARE_STDCALL glMap1f,24
DECLARE_STDCALL glMap2d,56
DECLARE_STDCALL glMap2f,40
DECLARE_STDCALL glMapGrid1d,20
DECLARE_STDCALL glMapGrid1f,12
DECLARE_STDCALL glMapGrid2d,40
DECLARE_STDCALL glMapGrid2f,24
DECLARE_STDCALL glMaterialf,12
DECLARE_STDCALL glMaterialfv,12
DECLARE_STDCALL glMateriali,12
DECLARE_STDCALL glMaterialiv,12
DECLARE_STDCALL glMatrixMode,4
DECLARE_STDCALL glMultMatrixd,4
DECLARE_STDCALL glMultMatrixf,4
DECLARE_STDCALL glNewList,8
DECLARE_STDCALL glNormal3b,12
DECLARE_STDCALL glNormal3bv,4
DECLARE_STDCALL glNormal3d,24
DECLARE_STDCALL glNormal3dv,4
DECLARE_STDCALL glNormal3f,12
DECLARE_STDCALL glNormal3fv,4
DECLARE_STDCALL glNormal3i,12
DECLARE_STDCALL glNormal3iv,4
DECLARE_STDCALL glNormal3s,12
DECLARE_STDCALL glNormal3sv,4
DECLARE_STDCALL glNormalPointer,12
DECLARE_STDCALL glOrtho,48
DECLARE_STDCALL glPassThrough,4
DECLARE_STDCALL glPixelMapfv,12
DECLARE_STDCALL glPixelMapuiv,12
DECLARE_STDCALL glPixelMapusv,12
DECLARE_STDCALL glPixelStoref,8
DECLARE_STDCALL glPixelStorei,8
DECLARE_STDCALL glPixelTransferf,8
DECLARE_STDCALL glPixelTransferi,8
DECLARE_STDCALL glPixelZoom,8
DECLARE_STDCALL glPointSize,4
DECLARE_STDCALL glPolygonMode,8
DECLARE_STDCALL glPolygonOffset,8
DECLARE_STDCALL glPolygonStipple,4
DECLARE_STDCALL glPopAttrib,0
DECLARE_STDCALL glPopClientAttrib,0
DECLARE_STDCALL glPopMatrix,0
DECLARE_STDCALL glPopName,0
DECLARE_STDCALL glPrioritizeTextures,12
DECLARE_STDCALL glPushAttrib,4
DECLARE_STDCALL glPushClientAttrib,4
DECLARE_STDCALL glPushMatrix,0
DECLARE_STDCALL glPushName,4
DECLARE_STDCALL glRasterPos2d,16
DECLARE_STDCALL glRasterPos2dv,4
DECLARE_STDCALL glRasterPos2f,8
DECLARE_STDCALL glRasterPos2fv,4
DECLARE_STDCALL glRasterPos2i,8
DECLARE_STDCALL glRasterPos2iv,4
DECLARE_STDCALL glRasterPos2s,8
DECLARE_STDCALL glRasterPos2sv,4
DECLARE_STDCALL glRasterPos3d,24
DECLARE_STDCALL glRasterPos3dv,4
DECLARE_STDCALL glRasterPos3f,12
DECLARE_STDCALL glRasterPos3fv,4
DECLARE_STDCALL glRasterPos3i,12
DECLARE_STDCALL glRasterPos3iv,4
DECLARE_STDCALL glRasterPos3s,12
DECLARE_STDCALL glRasterPos3sv,4
DECLARE_STDCALL glRasterPos4d,32
DECLARE_STDCALL glRasterPos4dv,4
DECLARE_STDCALL glRasterPos4f,16
DECLARE_STDCALL glRasterPos4fv,4
DECLARE_STDCALL glRasterPos4i,16
DECLARE_STDCALL glRasterPos4iv,4
DECLARE_STDCALL glRasterPos4s,16
DECLARE_STDCALL glRasterPos4sv,4
DECLARE_STDCALL glReadBuffer,4
DECLARE_STDCALL glReadPixels,28
DECLARE_STDCALL glRectd,32
DECLARE_STDCALL glRectdv,8
DECLARE_STDCALL glRectf,16
DECLARE_STDCALL glRectfv,8
DECLARE_STDCALL glRecti,16
DECLARE_STDCALL glRectiv,8
DECLARE_STDCALL glRects,16
DECLARE_STDCALL glRectsv,8
DECLARE_STDCALL glRenderMode,4
DECLARE_STDCALL glRotated,32
DECLARE_STDCALL glRotatef,16
DECLARE_STDCALL glScaled,24
DECLARE_STDCALL glScalef,12
DECLARE_STDCALL glScissor,16
DECLARE_STDCALL glSelectBuffer,8
DECLARE_STDCALL glShadeModel,4
DECLARE_STDCALL glStencilFunc,12
DECLARE_STDCALL glStencilMask,4
DECLARE_STDCALL glStencilOp,12
DECLARE_STDCALL glTexCoord1d,8
DECLARE_STDCALL glTexCoord1dv,4
DECLARE_STDCALL glTexCoord1f,4
DECLARE_STDCALL glTexCoord1fv,4
DECLARE_STDCALL glTexCoord1i,4
DECLARE_STDCALL glTexCoord1iv,4
DECLARE_STDCALL glTexCoord1s,4
DECLARE_STDCALL glTexCoord1sv,4
DECLARE_STDCALL glTexCoord2d,16
DECLARE_STDCALL glTexCoord2dv,4
DECLARE_STDCALL glTexCoord2f,8
DECLARE_STDCALL glTexCoord2fv,4
DECLARE_STDCALL glTexCoord2i,8
DECLARE_STDCALL glTexCoord2iv,4
DECLARE_STDCALL glTexCoord2s,8
DECLARE_STDCALL glTexCoord2sv,4
DECLARE_STDCALL glTexCoord3d,24
DECLARE_STDCALL glTexCoord3dv,4
DECLARE_STDCALL glTexCoord3f,12
DECLARE_STDCALL glTexCoord3fv,4
DECLARE_STDCALL glTexCoord3i,12
DECLARE_STDCALL glTexCoord3iv,4
DECLARE_STDCALL glTexCoord3s,12
DECLARE_STDCALL glTexCoord3sv,4
DECLARE_STDCALL glTexCoord4d,32
DECLARE_STDCALL glTexCoord4dv,4
DECLARE_STDCALL glTexCoord4f,16
DECLARE_STDCALL glTexCoord4fv,4
DECLARE_STDCALL glTexCoord4i,16
DECLARE_STDCALL glTexCoord4iv,4
DECLARE_STDCALL glTexCoord4s,16
DECLARE_STDCALL glTexCoord4sv,4
DECLARE_STDCALL glTexCoordPointer,16
DECLARE_STDCALL glTexEnvf,12
DECLARE_STDCALL glTexEnvfv,12
DECLARE_STDCALL glTexEnvi,12
DECLARE_STDCALL glTexEnviv,12
DECLARE_STDCALL glTexGend,16
DECLARE_STDCALL glTexGendv,12
DECLARE_STDCALL glTexGenf,12
DECLARE_STDCALL glTexGenfv,12
DECLARE_STDCALL glTexGeni,12
DECLARE_STDCALL glTexGeniv,12
DECLARE_STDCALL glTexImage1D,32
DECLARE_STDCALL glTexImage2D,36
DECLARE_STDCALL glTexParameterf,12
DECLARE_STDCALL glTexParameterfv,12
DECLARE_STDCALL glTexParameteri,12
DECLARE_STDCALL glTexParameteriv,12
DECLARE_STDCALL glTexSubImage1D,28
DECLARE_STDCALL glTexSubImage2D,36
DECLARE_STDCALL glTranslated,24
DECLARE_STDCALL glTranslatef,12
DECLARE_STDCALL glVertex2d,16
DECLARE_STDCALL glVertex2dv,4
DECLARE_STDCALL glVertex2f,8
DECLARE_STDCALL glVertex2fv,4
DECLARE_STDCALL glVertex2i,8
DECLARE_STDCALL glVertex2iv,4
DECLARE_STDCALL glVertex2s,8
DECLARE_STDCALL glVertex2sv,4
DECLARE_STDCALL glVertex3d,24
DECLARE_STDCALL glVertex3dv,4
DECLARE_STDCALL glVertex3f,12
DECLARE_STDCALL glVertex3fv,4
DECLARE_STDCALL glVertex3i,12
DECLARE_STDCALL glVertex3iv,4
DECLARE_STDCALL glVertex3s,12
DECLARE_STDCALL glVertex3sv,4
DECLARE_STDCALL glVertex4d,32
DECLARE_STDCALL glVertex4dv,4
DECLARE_STDCALL glVertex4f,16
DECLARE_STDCALL glVertex4fv,4
DECLARE_STDCALL glVertex4i,16
DECLARE_STDCALL glVertex4iv,4
DECLARE_STDCALL glVertex4s,16
DECLARE_STDCALL glVertex4sv,4
DECLARE_STDCALL glVertexPointer,16
DECLARE_STDCALL glViewport,16
DECLARE_STDCALL glDrawRangeElements,24
DECLARE_STDCALL glTexImage3D,40
DECLARE_STDCALL glTexSubImage3D,44
DECLARE_STDCALL glCopyTexSubImage3D,36
END_IMPORTS_DEF

        END                     ; End of module
