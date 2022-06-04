#pragma once

void MaskExceptions(void);
void Sys_Init(void);
void Sys_InitFloatTime(void);
void Sys_PushFPCW_SetHigh(void);
void Sys_PopFPCW(void);
void Sys_AtExit(void);