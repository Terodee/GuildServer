#include ".\guild.h"
#include "Item.h"
#include "Guildsman.h"
#include <assert.h>
#include <io.h>
#include <direct.h>
#include "StrTok.h"

CGuild::CGuild(void):
	m_cLevel(1),
	m_dwBlackPoint(0),
	m_dwGold(0),
	m_iGuildGUID(0),
	m_dwGuildPoint(0),
	m_bHasWarehouse(FALSE)
{
	for(int i = 0;i < DEF_MAXGUILDITEMS;i++)
		m_pGuildItems[i] = NULL;

	ZeroMemory(m_cGuildName, sizeof(m_cGuildName));
	ZeroMemory(m_cMasterName, sizeof(m_cMasterName));
	ZeroMemory(m_cLocation, sizeof(m_cLocation));
	ZeroMemory(m_cBulletin, sizeof(m_cBulletin));
	ZeroMemory(m_cCreateTime,sizeof(m_cCreateTime));
	strcpy(m_cBulletin, "<NONE>");
}

CGuild::~CGuild(void)
{
	for(int i = 0;i < DEF_MAXGUILDITEMS;i++)	{
		delete m_pGuildItems[i];
		m_pGuildItems[i] = NULL;
	}

	map<string, CGuildsman*>::iterator it;
	for(it = m_mGuildsman.begin(); it != m_mGuildsman.end();it++)	{
		delete it->second;
		it->second = NULL;
	}
	m_mGuildsman.clear();
}


BOOL CGuild::bAddGuildsman(CGuildsman* pGuildsman)
{
	//assert(pGuildsman);
	string str(pGuildsman->m_cName);
	pair<map<string, CGuildsman* >::iterator,bool> ret = m_mGuildsman.insert(map<string,CGuildsman*>::value_type(str, pGuildsman));
	//assert(ret.second);
	return ret.second;
}

void CGuild::DelGuildsman(string& sName)
{
	m_mGuildsman.erase(sName);
}

int CGuild::iComposeGuildInfo(char* pData, BOOL bToClient)
{
	int *ip;
	DWORD *dwp;
	WORD *wp;
	short *sp;
	//guild info
	char* cp = pData;
	ZeroMemory(cp,20);			
	strcpy(cp,m_cGuildName);
	cp += 20;

	strcpy(cp, m_cMasterName);
	cp += 10;

	ip = (int*) cp;
	*ip = m_iGuildGUID;
	cp += 4;

	memcpy(cp,m_cLocation,10);
	cp += 10;

	*cp = m_cLevel;
	cp ++;
	dwp =(DWORD*) cp;
	*dwp = m_dwBlackPoint;
	cp += 4;

	dwp =(DWORD*) cp;
	*dwp = m_dwGuildPoint;
	cp += 4;

	dwp =(DWORD*) cp;
	*dwp = m_dwGold;
	cp += 4;
	
	memcpy(cp, m_cCreateTime, 10);
	cp += 10;

	WORD wLen =(WORD) (strlen(m_cBulletin) + 1);
	wp =(WORD*) cp;
	*wp = wLen;
	cp += 2;
	memcpy(cp, m_cBulletin, wLen);
	cp += wLen;

	if(m_bHasWarehouse)
		*cp = 1;
	else
		*cp = 0;
	cp ++;

	//guildsman
	wLen =(WORD)(m_mGuildsman.size());
	wp = (WORD*)cp;
	*wp = wLen;
	cp += 2;

	map<string,CGuildsman*>::iterator it;
	for(it = m_mGuildsman.begin();it != m_mGuildsman.end(); it++)
	{
		CGuildsman* pGuildsman = it->second;
		//strcpy(cp, pGuildsman->m_cName);
		memcpy(cp, pGuildsman->m_cName, 10);
		cp += 10;
		//strcpy(cp, pGuildsman->m_cTitle);
		memcpy(cp, pGuildsman->m_cTitle, 20);
		cp += 20;
		*cp = pGuildsman->m_cRank;
		cp ++;
		dwp = (DWORD*) cp;
		*dwp = pGuildsman->m_dwBlackPointContri;
		cp += 4;
		dwp = (DWORD*) cp;
		*dwp = pGuildsman->m_dwGoldContri;
		cp += 4;
	}

	//guild items
	WORD wCount(0);
	for(int i = 0;i < DEF_MAXGUILDITEMS; i ++)
	{
		if(m_pGuildItems[i] != NULL)
			wCount ++;
	}
	wp = (WORD*) cp;
	*wp = wCount;
	cp += 2;
	for(int i = 0;i < DEF_MAXGUILDITEMS; i++)
	{
		if(m_pGuildItems[i] == NULL)
			continue;
		sp = (short*) cp;
		*sp = (short) i;			//index
		cp += 2;
		CItem* pItem = m_pGuildItems[i];
#ifndef	_DEF_HBCLIENT_
		if(bToClient)
			cp += pItem->iComposeClientItemInfo(cp);
		else
			cp += pItem->iComposeItemInfo(cp);
#else
		cp += pItem->iComposeClientItemInfo(cp);
#endif

	}

	return (int)(cp - pData);
}

BOOL CGuild::bDecodeGuildInfo(char* pData, BOOL bToClient)
{
	int *ip;
	DWORD *dwp;
	WORD *wp;
	short *sp;
	//guild info
	char* cp = pData;
	memcpy(m_cGuildName,cp ,20);
	cp += 20;

	memcpy(m_cMasterName,cp,10);
	cp += 10;

	ip = (int*) cp;
	m_iGuildGUID = *ip;
	cp += 4;

	memcpy(m_cLocation,cp, 10);
	cp += 10;

	m_cLevel = *cp;
	cp ++;

	dwp =(DWORD*) cp;
	m_dwBlackPoint = *dwp;
	cp += 4;

	dwp =(DWORD*) cp;
	m_dwGuildPoint = *dwp;
	cp += 4;

	dwp =(DWORD*) cp;
	m_dwGold = *dwp;
	cp += 4;
	
	memcpy(m_cCreateTime, cp, 10);
	cp += 10;

	wp = (WORD*) cp;
	WORD wLen = *wp;
	cp += 2;
	memcpy(m_cBulletin, cp, wLen);
	cp += wLen;

	m_bHasWarehouse =(BOOL) *cp;
	cp ++;
	//guildsman
	wp = (WORD*)cp;
	WORD wCount = *wp;
	cp += 2;

	for(int i = 0;i < wCount; i++)
	{
		CGuildsman* pGuildsman = new CGuildsman();
		//strcpy(pGuildsman->m_cName, cp);
		memcpy(pGuildsman->m_cName, cp, 10);
		cp += 10;
		//strcpy(pGuildsman->m_cTitle, cp);
		memcpy(pGuildsman->m_cTitle, cp, 20);
		cp += 20;
		pGuildsman->m_cRank = *cp;
		cp ++;
		dwp = (DWORD*) cp;
		pGuildsman->m_dwBlackPointContri = *dwp;
		cp += 4;
		dwp = (DWORD*) cp;
		pGuildsman->m_dwGoldContri = *dwp;
		cp += 4;

		if(bAddGuildsman(pGuildsman) == FALSE)
		{
			assert(false);
			//return FALSE;
		}
	}

	//guild items
	wp = (WORD*) cp;
	wCount = *wp;
	cp += 2;
	for(int i = 0;i < wCount; i++)
	{
		//if(m_pGuildItems[i] == NULL)
		//	continue;
		sp = (short*) cp;
		short sIndex = *sp;
		cp += 2;
		if(sIndex < 0 || sIndex >= DEF_MAXGUILDITEMS)
		{
			assert(false);
			return FALSE;
		}
		delete m_pGuildItems[sIndex];
		m_pGuildItems[sIndex] = new CItem();
		CItem* pItem = m_pGuildItems[sIndex];

#ifndef _DEF_HBCLIENT_
		if(bToClient)
			cp += pItem->iDecodeClientItemInfo(cp);
		else
			cp += pItem->iDecodeItemInfo(cp);
#else
		cp += pItem->iDecodeClientItemInfo(cp);
#endif
		
	}

	return TRUE;
}

BOOL CGuild::bParseFile(const char* pszFile)
{
	assert(pszFile);
	FILE* pFile = fopen(pszFile,"rt");
	assert(pFile);
	if(pFile == NULL)
		return FALSE;

	int iSize = _filelength(pFile->_file);
	char* cp =new char[iSize + 2];
	ZeroMemory(cp, sizeof(cp));
	fread(cp,1,iSize, pFile);
	fclose(pFile);

	char	seps[] = "= \t\n";
	CStrTok* pTok = new CStrTok(cp,seps);
	char* token = pTok->pGet();
	char cReadModeA(0),cReadModeB(0);

	CGuildsman* pGuildsman(NULL);
	CItem* pItem(NULL);

	while(token)
	{
		if(cReadModeA == 1)
		{
			switch(cReadModeB)		//guild info
			{
			case 1:
				strcpy(m_cGuildName, token);
				cReadModeB = 0;
				break;
			case 2:
				strcpy(m_cMasterName,token);
				cReadModeB = 0;
				break;
			case 3:
				m_iGuildGUID = atoi(token);
				cReadModeB = 0;
				break;
			case 4:
				strcpy(m_cLocation, token);
				cReadModeB = 0;
				break;
			case 5:
				m_cLevel =(char) atoi(token);
				cReadModeB = 0;
				break;
			case 6:
				m_dwBlackPoint = atoi(token);
				cReadModeB = 0;
				break;
			case 7:
				m_dwGuildPoint = atoi(token);
				cReadModeB = 0;
				break;
			case 8:
				m_dwGold = atoi(token);
				cReadModeB = 0;
				break;
			case 9:
				strcpy(m_cCreateTime,token);
				cReadModeB = 0;
				break;
			case 10:
				{
					strcpy(m_cBulletin, token);
					int iLen = (int)strlen(m_cBulletin);
					char src[] = "%^&*";
					char dest[] = "= \t";
					int num = sizeof(src) / sizeof(char);
					for(int i = 0;i < iLen; i++)
					{
						for(int j = 0; j < num; j++)
						{
							if(m_cBulletin[i] == src[j])
							{
								m_cBulletin[i] = dest[j];
								break;
							}
						}
					}
					cReadModeB = 0;
					break;
				}
			case 11:
				m_bHasWarehouse = atoi(token);
				cReadModeB = 0;
				break;
			case 0:
				{
					const char* head[]= {
						"guild-name",
						"guildmaster-name",
						"guild-GUID",
						"guild-location",
						"guild-level",
						"guild-blackpoint",
						"guild-point",
						"guild-gold",
						"guild-createtime",
						"guild-bulletin",
						"guild-storehouse"
					};
					const char mode[] ={
						1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
					};
					int num = sizeof(head)/sizeof(char*);
					if(strcmp(token,"[GUILDSMAN]") == 0)	
					{
						cReadModeA =2;
						cReadModeB = 0;
					}
					else
					{
						for(int i = 0;i < num;i++)
						{
							if(memcmp(token,head[i],strlen(head[i])) == 0)
							{
								cReadModeB = mode[i];
								break;
							}
						}
					}

				}
				break;
			default:
				assert(false);
			}
		}
		else if(cReadModeA == 2)		//guildsman
		{
			switch(cReadModeB)
			{
			case 1:
				strcpy(pGuildsman->m_cName, token);
				cReadModeB = 2;
				break;
			case 2:
				strcpy(pGuildsman->m_cTitle, token);
				cReadModeB = 3;
				break;
			case 3:
				pGuildsman->m_cRank =(char) atoi(token);
				assert(pGuildsman->m_cRank >= 0 && pGuildsman->m_cRank <= DEF_GUILDSTARTRANK);
				cReadModeB = 4;
				break; 
			case 4:
				pGuildsman->m_dwBlackPointContri = atoi(token);
				cReadModeB = 5;
				break;
			case 5:
				pGuildsman->m_dwGoldContri = atoi(token);
				bAddGuildsman(pGuildsman);
				cReadModeB = 0;
				break;
			case 0:
				{
					if(strcmp(token, "[GUILD-ITEMS]") == 0)
					{
						cReadModeA = 3;
						cReadModeB = 0;
					}
					else if(strcmp(token, "member") == 0)
					{
						cReadModeB = 1;
						pGuildsman = new CGuildsman();
					}

					break;
				}

			default:
				assert(false);
			}
		}
		else if(cReadModeA == 3)		//guild items
		{
			int iTemp(0);
			int index(0);
			switch( cReadModeB )
			{
			case 0:
				if(memcmp(token, "item", 4) == 0) 	{
					cReadModeB = 1;
					pItem = new CItem();
				}
				break;
			case 1:
				//m_cName
				strcpy(pItem->m_cName,token);
				cReadModeB = 2;
				break;

			case 2:
				// m_dwCount
				iTemp = atoi( token );
				if( iTemp < 0 ) iTemp = 1;

				pItem->m_dwCount = (DWORD)iTemp;
				cReadModeB = 3;

				break;

			case 3:
				// m_sTouchEffectType
				pItem->m_sTouchEffectType = atoi( token );
				cReadModeB = 4;
				break;

			case 4:
				// m_sTouchEffectValue1
				pItem->m_sTouchEffectValue1 = atoi( token );
				cReadModeB = 5;
				break;

			case 5:
				// m_sTouchEffectValue2
				pItem->m_sTouchEffectValue2 = atoi( token );
				cReadModeB = 6;
				break;

			case 6:
				// m_sTouchEffectValue3
				pItem->m_sTouchEffectValue3 = atoi( token );
				cReadModeB = 7;
				break;

			case 7:
				// m_cItemColor
				pItem->m_cItemColor = atoi( token );
				cReadModeB = 8;
				break;

			case 8:
				// m_sItemSpecEffectValue1
				pItem->m_sItemSpecEffectValue1 = atoi( token );
				cReadModeB = 9;
				break;

			case 9:
				// m_sItemSpecEffectValue2
				pItem->m_sItemSpecEffectValue2 = atoi( token );
				cReadModeB = 10;
				break;

			case 10:
				// m_sItemSpecEffectValue3
				pItem->m_sItemSpecEffectValue3 = atoi( token );
				cReadModeB = 11;
				break;

			case 11:
				// m_wCurLifeSpan
				pItem->m_wCurLifeSpan = atoi( token );

				cReadModeB = 12;
				break;

			case 12:
				{
					// m_dwAttribute
					pItem->m_dwAttribute = atoi( token );

					cReadModeB = 0;

					m_pGuildItems[index ++] = pItem;
					break;
				}
			}

		}
		else if(cReadModeA == 0)		//0
		{
			const char* head[]= {
				"[GUILD-INFO]",
				"[GUILDSMAN]",
				"[GUILD-ITEMS]"
			};
			const char mode[] ={
				1,	2,	3
			};
			int num = sizeof(head)/sizeof(char*);
			for(int i = 0;i < num;i++)
			{
				if(memcmp(token,head[i],strlen(head[i])) == 0)
				{
					cReadModeA = mode[i];
					cReadModeB = 0;
					break;
				}
			}
		}
		else 
			assert(false);

		token = pTok->pGet();
	}

	delete pTok;
	delete [] cp;

	return TRUE;
}

BOOL CGuild::bSaveToFile()
{
	
	char	cTxt[200],cTmp[50],pData[2048];

	_mkdir("Guild");
	char szFileName[MAX_PATH],szBackup[MAX_PATH];
	sprintf(szFileName,"Guild\\%s.gild",m_cGuildName);
	sprintf(szBackup, "Guild\\~%s.bak",m_cGuildName);

	DeleteFile(szBackup);
	rename(szFileName,szBackup);

	FILE* pFile = fopen(szFileName, "w");
	if(pFile == NULL)
		return FALSE;


	SYSTEMTIME	SysTime;
	GetLocalTime( &SysTime);
	sprintf(pData, ";Guild file - Updated %d/%d/%d %d:%d\n\n",SysTime.wYear,SysTime.wMonth,SysTime.wDay,
		SysTime.wHour,SysTime.wMinute);

	//GUID-INFO
	strcat(pData, "[GUILD-INFO]\n");

	strcat(pData, "\nguild-name = ");
	strcat(pData, m_cGuildName);

	strcat(pData, "\nguildmaster-name = ");
	strcat(pData, m_cMasterName);

	strcat(pData, "\nguild-GUID = ");
	sprintf(cTxt,"%d",m_iGuildGUID);
	strcat(pData,cTxt);

	strcat(pData, "\nguild-location = ");
	strcat(pData, m_cLocation);

	strcat(pData, "\nguild-level = ");
	sprintf(cTxt, "%d", m_cLevel);
	strcat(pData, cTxt);

	strcat(pData, "\nguild-blackpoint = ");
	sprintf(cTxt, "%d", m_dwBlackPoint);
	strcat(pData, cTxt);

	strcat(pData, "\nguild-point = ");
	sprintf(cTxt, "%d", m_dwGuildPoint);
	strcat(pData, cTxt);

	strcat(pData, "\nguild-gold	= ");
	sprintf(cTxt, "%d", m_dwGold);
	strcat(pData, cTxt);

	strcat(pData, "\nguild-createtime = ");
	strcat(pData, m_cCreateTime);

	int iLen = (int)strlen(m_cBulletin);
	strcpy(cTxt, m_cBulletin);
	char src[] = "= \t\n";
	char dest[] = "%^&**";
	int num = sizeof(src) /sizeof(char);
	for(int i = 0;i < iLen; i++)
	{
		for(int j = 0; j < num; j++)
		{
			if(cTxt[i] == src[j])
			{
				cTxt[i] = dest[j];
				break;
			}
		}
	}
	strcat(pData, "\nguild-bulletin = ");
	strcat(pData, cTxt);

	strcat(pData, "\nguild-storehouse = ");
	if(m_bHasWarehouse)
		strcat(pData, "1");
	else
		strcat(pData, "0");

	strcat(pData, "\n\n");

	fwrite(pData, 1, strlen(pData), pFile);

	//GUILDSMAN
	strcpy(pData, "[GUILDSMAN]\n");
	fwrite(pData, 1, strlen(pData), pFile);

	map<string, CGuildsman*>::iterator it;
	for(it = m_mGuildsman.begin(); it != m_mGuildsman.end();it++)
	{
		CGuildsman* pGuildsman = it->second;
		assert(pGuildsman);

		sprintf(pData,"member = %s  %s  %d  %d  %d\n",pGuildsman->m_cName, pGuildsman->m_cTitle,
			pGuildsman->m_cRank, pGuildsman->m_dwBlackPointContri, pGuildsman->m_dwGoldContri);
		fwrite(pData, 1, strlen(pData), pFile);
	}

	//GUILDS-ITEMS
	strcpy(pData,"\n[GUILD-ITEMS]\n");
	fwrite(pData, 1, strlen(pData), pFile);

	for(int i = 0;i < DEF_MAXGUILDITEMS; i++)
	{
		CItem* pItem = m_pGuildItems[i];
		if(pItem == NULL)
			continue;

		strcpy( pData, "item = " );
		memset( cTmp, ' ', 21 );
		strcpy( cTmp, pItem->m_cName );
		cTmp[strlen( pItem->m_cName )] = (char)' ';
		cTmp[20] = NULL;
		strcat( pData, cTmp );
		strcat( pData, " " );
		itoa( pItem->m_dwCount, cTxt, 10 );
		strcat( pData, cTxt );
		strcat( pData, " " );
		itoa( pItem->m_sTouchEffectType, cTxt, 10 );
		strcat( pData, cTxt );
		strcat( pData, " " );
		itoa( pItem->m_sTouchEffectValue1, cTxt, 10 );
		strcat( pData, cTxt );
		strcat( pData, " " );
		itoa( pItem->m_sTouchEffectValue2, cTxt, 10 );
		strcat( pData, cTxt );
		strcat( pData, " " );
		itoa( pItem->m_sTouchEffectValue3, cTxt, 10 );
		strcat( pData, cTxt );
		strcat( pData, " " );
		itoa( pItem->m_cItemColor, cTxt, 10 );
		strcat( pData, cTxt );
		strcat( pData, " " );
		itoa( pItem->m_sItemSpecEffectValue1, cTxt, 10 );
		strcat( pData, cTxt );
		strcat( pData, " " );
		itoa( pItem->m_sItemSpecEffectValue2, cTxt, 10 );
		strcat( pData, cTxt );
		strcat( pData, " " );
		itoa( pItem->m_sItemSpecEffectValue3, cTxt, 10 );
		strcat( pData, cTxt );
		strcat( pData, " " );
		itoa( pItem->m_wCurLifeSpan, cTxt, 10 );
		strcat( pData, cTxt );
		strcat( pData, " " );
		itoa( pItem->m_dwAttribute, cTxt, 10 );
		strcat( pData, cTxt );
		strcat( pData, "\n" );

		fwrite(pData, 1, strlen(pData), pFile);
	}
	fclose(pFile);

	return TRUE;
}

CGuildsman* CGuild::FindGuildsman(char* pCharName)
{
	string strCharName(pCharName);
	map<string, CGuildsman*>::iterator it = m_mGuildsman.find(strCharName);
	if(it == m_mGuildsman.end())
		return NULL;
	return it->second;
}

void CGuild::SetBulletin(const char* cp, int iLen)
{
	if(iLen > DEF_MAX_BULLETINLEN)
		return;
	ZeroMemory(m_cBulletin, sizeof(m_cBulletin));
	memcpy(m_cBulletin, cp, iLen);

}

int CGuild::GetItemNum()
{
	int count(0);
	for(int i = 0; i < DEF_MAXGUILDITEMS; i++)
	{
		if(m_pGuildItems[i])
			count ++;
	}
	return count;
}

int CGuild::AddItem(CItem* pItem)
{
	for(int i = 0;i < DEF_MAXGUILDITEMS;i ++)
	{
		if(m_pGuildItems[i] == NULL)
		{
			m_pGuildItems[i] = pItem;
			return i;
		}
	}
	return -1;
}

int CGuild::GetMaxGuildsman()
{
	assert(m_cLevel >= 1 && m_cLevel <= 5);
	//2006-6-5 modified by jcfly
	//int max[] ={10, 30, 50, 70, 100};
	int max[] ={20, 40, 60, 80, 200};	// 2008-3-17 modified by jcfly. 100 -> 200
	return max[m_cLevel -1];

}

CGuildsman *CGuild::pGetSiegeHero()
{
	CGuildsman *pGuildsman(NULL);
	short sMaxKillNum(0);

	map<string, CGuildsman*>::iterator it;
	for(it = m_mGuildsman.begin(); it != m_mGuildsman.end(); ++it)	{
		if(it->second->m_sKillNum> sMaxKillNum)	{
			pGuildsman = it->second;
			sMaxKillNum = pGuildsman->m_sKillNum;
		}
	}
	return pGuildsman;
}

void CGuild::ClearGuildsmanKillNum()
{
	map<string, CGuildsman*>::iterator it;
	for(it = m_mGuildsman.begin(); it != m_mGuildsman.end(); ++it)	{
		it->second->m_sKillNum = 0;;
	}
}