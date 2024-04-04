#include ".\guildsman.h"

CGuildsman::CGuildsman(void):
	m_cRank(-1),
	m_dwBlackPointContri(0),
	m_dwGoldContri(0),
	m_sKillNum(0)
{
	ZeroMemory(m_cName, sizeof(m_cName));
	ZeroMemory(m_cTitle, sizeof(m_cTitle));
}

CGuildsman::~CGuildsman(void)
{
}
