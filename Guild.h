//2006-3-19 created by jcfly

#pragma once
#include <Windows.h>
#include <string>
#include <map>
using namespace std;

#define	DEF_MAXGUILDMEMBERS	100
#define	DEF_MAXGUILDITEMS	100
#define DEF_MAX_BULLETINLEN	100

#define DEF_GUILDSTARTRANK	4

class CGuildsman;
class CItem;

class CGuild
{
public:
	CGuild(void);
	~CGuild(void);
	//Return FALSE if the guildsman already exists
	BOOL	bAddGuildsman(CGuildsman* pGuildsman);
	void	DelGuildsman(string& sName);
	int		GetMemberNum(){
		return (int)m_mGuildsman.size();
	}
	const map<string,CGuildsman*>& GetGuildsman(){
		return m_mGuildsman;
	}
	int		GetItemNum();
	BOOL 	IsGuildsman(string& sName)	{
		return (m_mGuildsman.find(sName) != m_mGuildsman.end());
	}

	CGuildsman* FindGuildsman(char* pCharName);
	int		iComposeGuildInfo(char* pData, BOOL bToClient);
	BOOL	bDecodeGuildInfo(char* pData, BOOL bToClient);
	//
	BOOL	bParseFile(const char* pszFile);
	BOOL	bSaveToFile();
	void		SetBulletin(const char* cp, int iLen);
	//return the index of the item in the item list.-1 if failed.
	int		AddItem(CItem* pItem);
	//number of guildsmans this guild can recruit
	int		GetMaxGuildsman();


	void		ClearGuildsmanKillNum();
	CGuildsman *pGetSiegeHero();		//The player who killed the most monsters in the monster-siege.(如果有相同的，随机选一个)
public:
	//工会信息
	char	m_cGuildName[21];
	char	m_cMasterName[11];
	char	m_cLocation[11];
	char	m_cCreateTime[11];
	char	m_cBulletin[DEF_MAX_BULLETINLEN + 1];
	char	m_cLevel;
	int		m_iGuildGUID;
	BOOL	m_bHasWarehouse;	
	DWORD	m_dwGuildPoint;
	DWORD	m_dwBlackPoint;
	DWORD	m_dwGold;
	
	//仓库物品
	CItem*			m_pGuildItems[DEF_MAXGUILDITEMS];

private:
	//成员
	map<string, CGuildsman*>	m_mGuildsman;


};
