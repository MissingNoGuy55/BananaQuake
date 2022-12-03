/****************************************************************************
*
*                   SciTech Multi-platform Graphics Library
*
*  ========================================================================
*
*   Copyright (C) 1991-2004 SciTech Software, Inc. All rights reserved.
*
*   This file may be distributed and/or modified under the terms of the
*   GNU General Public License version 2.0 as published by the Free
*   Software Foundation and appearing in the file LICENSE.GPL included
*   in the packaging of this file.
*
*   Licensees holding a valid Commercial License for this product from
*   SciTech Software, Inc. may use this file in accordance with the
*   Commercial License Agreement provided with the Software.
*
*   This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
*   THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
*   PURPOSE.
*
*   See http://www.scitechsoft.com/license/ for information about
*   the licensing options available and how to purchase a Commercial
*   License Agreement.
*
*   Contact license@scitechsoft.com if any conditions of this licensing
*   are not clear to you, or you have questions about licensing options.
*
*  ========================================================================
*
* Language:     ANSI C
* Environment:  Any
*
* Description:  Header file for the SciTech VGA and VBE dumb framebuffer
*               driver. We ignore any accelerated graphics modes in this
*               driver, and can support VGA, VBE 1.2 banked and VBE 2.0/3.0
*               linear modes.
*
****************************************************************************/

#ifndef __DRIVERS_SNAP_SNAPVBE_H
#define __DRIVERS_SNAP_SNAPVBE_H

#include "drivers/common/gsnap.h"

/*------------------------- Function Prototypes ---------------------------*/

void *  MGLAPI SNAPVBE_createInstance(void);
ibool   MGLAPI SNAPVBE_detect(void *data,int id,int *numModes,modetab availableModes);
ibool   MGLAPI SNAPVBE_initDriver(void *data,MGLDC *dc,modeent *mode,ulong hwnd,int virtualX,int virtualY,int numBuffers,ibool stereo,int refreshRate,ibool useLinearBlits);
void    MGLAPI SNAPVBE_destroyInstance(void *data);

#endif  /* __DRIVERS_SNAP_SNAPVBE_H */

