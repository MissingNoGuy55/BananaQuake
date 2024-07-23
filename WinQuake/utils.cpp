#include "quakedef.h"
#include "utils.h"

CQVector<CQTimer> g_pTimers;

void CQTimer::UpdateTimer()
{	
	if (m_dElapsed > m_dTime && !m_bElapsed)
	{
		m_fCallback(callbackParm);
		m_bElapsed = true;
		g_pTimers.RemoveElement(g_pTimers.FindElement(this));
	}
	else
		m_dElapsed += host->host_frametime;
}

void CQTimer::SetCallback(timercommand_t fCallback, void* parm)
{
	m_fCallback = fCallback;
	callbackParm = parm;
}
