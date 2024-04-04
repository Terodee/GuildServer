//2006-3-19 created by jcfly

#pragma once
#include <Windows.h>

class CGuild;
class CGuildsman
{
public:
	CGuildsman(void);
	~CGuildsman(void);

	friend CGuild;

	void		AddKillNum()	{
		m_sKillNum ++;
	}
	short	sGetKillNum()		{
		return m_sKillNum;
	}
public:
	char	m_cName[11];
	char	m_cTitle[21];
	char	m_cRank;
	DWORD	m_dwGoldContri;
	DWORD	m_dwBlackPointContri;

protected:
	short 	m_sKillNum;		//Number of monsters this guildsman killed in the monster-siege

};
