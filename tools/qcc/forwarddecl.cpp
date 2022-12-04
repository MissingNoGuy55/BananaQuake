#include <windows.h>
#include <MGRAPH.H>
#include "forwarddecl.h"

//===========================================================================
//					FORWARD DECLARATIONS FROM BANANAQUAKE
//===========================================================================

const char* PR_GetString();

bool block_drawing = false;
bool DDActive = false;
bool scr_disabled_for_loading = false;
bool scr_skipupdate = false;

bool pr_trace = false;