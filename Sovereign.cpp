#include ".\sovereign.h"
#include "Game.h"
#include <cassert>
#include <io.h>
#include "StrTok.h"
#include "NetMessages.h"
#include "winmain.h"

extern char G_cTxt[];

CSovereign::CSovereign(CGame *pGame):
	m_pGame(pGame),
	m_iState(SOV_STATE_NONE),
	m_tBegin(0)
{
	ZeroMemory(m_cName, sizeof(m_cName));
	strcpy(m_cName, "NONE");
	ZeroMemory(m_cLocation, sizeof(m_cLocation));

	m_pData = new char[1024 * 2];
}

CSovereign::~CSovereign(void)
{
	delete []m_pData;
}


BOOL CSovereign::bInit(char *pLocation)
{
	strcpy(m_cLocation, pLocation);	
	if(bParseFile() == FALSE)
		return FALSE;
	sprintf(G_cTxt, "(!) Location <%s> state<%d> sov <%s>", m_cLocation, m_iState, m_cName);
	PutLogList(G_cTxt);
	return TRUE;
}

BOOL CSovereign::bSaveToFile()
{
	char cDir[MAX_PATH], cBackup[MAX_PATH];
	sprintf(cDir, "GameData\\%s_sov.txt", m_cLocation);
	sprintf(cBackup, "GameData\\~%s_sov.bak",m_cLocation);

	DeleteFile(cBackup);
	rename(cDir, cBackup);

	FILE *pFile = fopen(cDir, "w");
	if(pFile == NULL)
	{
		sprintf(G_cTxt,"(!!!) SOV::bSaveToFile. Error opening file <%s>", m_cLocation);
		PutLogList(G_cTxt);
		return FALSE;
	}
	sprintf(m_pData, "sov = %d  %d  %s  \n", m_iState, m_tBegin, m_cName);
	fwrite(m_pData, 1, strlen(m_pData), pFile);
	//Vote
	if(m_iState == SOV_STATE_VOTE)
	{
		map<string, string>::iterator it;
		for(it = m_mVote.begin(); it != m_mVote.end(); it++)
		{
			sprintf(m_pData,"vote = %s  %s \n",it->first.c_str(),it->second.c_str());
			fwrite(m_pData, 1, strlen(m_pData), pFile);
		}
	}
	fclose(pFile);
	return TRUE;
}

BOOL CSovereign::bParseFile()
{
	char cDir[MAX_PATH];
	sprintf(cDir, "GameData\\%s_sov.txt", m_cLocation);

	FILE *pFile = fopen(cDir,"r");
	if(pFile == NULL)
	{
		sprintf(G_cTxt,"(!!!) SOV::bParseFile. File not found <%s>", m_cLocation);
		PutLogList(G_cTxt);
		return TRUE;		//Maybe this program runs for the first time.
	}

	int iLen = _filelength(pFile->_file);
	if(iLen == -1)
	{
		assert(false);
		fclose(pFile);
		return FALSE;
	}

	char *cp = new char[iLen + 2];
	ZeroMemory(cp, sizeof(cp));
	fread(cp, 1, iLen, pFile);
	fclose(pFile);

	char	seps[] = "= \t\r\n";
	CStrTok *pTok = new CStrTok(cp, seps);
	char *token = pTok->pGet();
	char cReadModeA(0),cReadModeB(0);
	char src[11],dest[11];
	ZeroMemory(src, sizeof(src));
	ZeroMemory(dest,sizeof(dest));
	while(token)
	{
		if(cReadModeA == 1)
		{
			switch(cReadModeB)
			{
			case 1:
				m_iState = atoi(token);
				assert(m_iState == SOV_STATE_NONE || m_iState == SOV_STATE_REIGNING || m_iState == SOV_STATE_VOTE);
				cReadModeB = 2;
				break;
			case 2:
				m_tBegin = atoi(token);
				cReadModeB = 3;
				break;
			case 3:
				strcpy(m_cName, token);
				cReadModeA = 0;
				cReadModeB = 0;
				break;
			}
		}
		else if(cReadModeA == 2)
		{
			if(m_iState == SOV_STATE_VOTE)
			{
				switch(cReadModeB)
				{
				case 1:
					strcpy(src, token);
					cReadModeB = 2;
					break;
				case 2:
					{
						strcpy(dest,token);
						cReadModeB = 0;
						cReadModeA = 0;

						string strSrc(src);
						string strDest(dest);
						m_mVote.insert(map<string,string>::value_type(strSrc,strDest));
						break;
					}
				}
			}
			else
				assert(false);
		}
		else if(cReadModeA == 0)
		{
			if(strcmp(token,"sov") == 0)
			{
				cReadModeA = 1;
				cReadModeB = 1;
			}
			else if(strcmp(token, "vote") == 0)
			{
				cReadModeA = 2;
				cReadModeB = 1;
			}
		}

		token = pTok->pGet();
	}
	delete []cp;
	delete pTok;
	return TRUE;
}

void CSovereign::OnTimer()
{
	time_t ltime;
	time(&ltime);


	static time_t tSave = time(NULL);
	if(ltime - tSave >= 60 * 20)		//20 minutes
	{
		if(bSaveToFile() == FALSE)
		{
			sprintf(G_cTxt,"(!!!) <%s> SaveToFile failed", m_cLocation);
			PutLogList(G_cTxt);
		}
		else
		{
			sprintf(G_cTxt,"(!) <%s> Sovereign saved",m_cLocation);
			PutLogList(G_cTxt);
		}
		tSave = ltime;
		return;
	}
	struct tm *today = localtime( &ltime );

	if(m_iState == SOV_STATE_NONE)
	{
		if((today->tm_hour == 20) && (ltime - m_tBegin >= 24 * 3600))		//8:00 pm, 24 hours
			StartVote();
	}
	else if(m_iState == SOV_STATE_REIGNING)
	{
		if(ltime - m_tBegin >= 24 * 3600 * 30)		//30 days
			StartVote();
	}
	else if(m_iState == SOV_STATE_VOTE)
	{
		if(ltime - m_tBegin >= 24 * 3600 * 3)		//3 days
			EndVote();
	}
}

void CSovereign::StartVote()
{
	//assert(m_iState != SOV_STATE_VOTE);
	if(m_iState == SOV_STATE_VOTE)
		return;

	if(m_iState == SOV_STATE_REIGNING)
		ZeroMemory(m_cName, sizeof(m_cName));

	m_iState = SOV_STATE_VOTE;
	m_tBegin = time(NULL);
	m_mVote.clear();

	sprintf(G_cTxt,"(!) <%s> Vote started", m_cLocation);
	PutLogList(G_cTxt);
	SendSovMsgToAll(DEF_SOV_STARTVOTE);
}

void CSovereign::EndVote()
{
	//assert(m_iState == SOV_STATE_VOTE);
	if(m_iState != SOV_STATE_VOTE)
		return;
	map<string, string>::iterator it;
	map<string, int>	mVote;
	map<string, int>::iterator itVote;
	for(it = m_mVote.begin(); it != m_mVote.end(); it++)
	{
		string &strName = it->second;
		itVote = mVote.find(strName);
		if(itVote == mVote.end())
			mVote.insert(map<string, int>::value_type(strName,1));
		else
			itVote->second ++;
	}
	//得票数最多的三个人
	const char *pCharName[] = {NULL, NULL, NULL};
	int count[] = {0, 0, 0};
	const int num = sizeof(pCharName) / sizeof(char*);
	for(itVote = mVote.begin(); itVote != mVote.end();itVote++)
	{
		if(itVote->second <= count[num -1])
			continue;
		for(int i = num - 2; i >= 0; i--)
		{
			if(itVote->second <= count[i])
			{
				pCharName[i + 1] = itVote->first.c_str();
				count[i + 1] = itVote->second;
				break;
			}
			else
			{
				pCharName[i + 1] = pCharName[i];
				count[i + 1] = count[i];

				pCharName[i] = itVote->first.c_str();
				count[i] = itVote->second;
			}
		}
	}
	//The winner
	int votes = 0;
	for(int i = 0; i < num; i++)
		if(pCharName[i] != NULL)
			votes ++;
	if(votes > 0)
	{
		assert(pCharName[0] != NULL);
		ZeroMemory(m_cName, sizeof(m_cName));
		strcpy(m_cName, pCharName[0]);
		m_iState = SOV_STATE_REIGNING;
	}
	else 
		m_iState= SOV_STATE_NONE;

	time_t ltime;
	time(&ltime);
	m_tBegin = ltime;
	//compose message
	char *cp = m_pData;
	DWORD *dwp = (DWORD*) cp;
	*dwp = MSGID_SOVEREIGN;
	cp += 4;
	WORD *wp = (WORD*) cp;
	*wp = DEF_SOV_ENDVOTE;
	cp += 2;

	memcpy(cp, m_cLocation, 10);
	cp += 10;
	//time
	time_t *pt = (time_t*) cp;
	*pt = ltime;
	cp += 4;

	*cp = (char) votes;
	cp ++;

	for(int i = 0; i < votes; i++)
	{
		memcpy(cp, pCharName[i], 10);
		cp += 10;

		int *ip = (int*) cp;
		*ip = count[i];
		cp += 4;
	}

	m_pGame->SendMsgToAll(m_pData, (DWORD)(cp - m_pData));
	
	//
	sprintf(G_cTxt, "(!) %s vote resulte:", m_cLocation);
	PutLogList(G_cTxt);
	for(int i = 0; i < num; ++i)
	{
		sprintf(G_cTxt, "(!) Name (%s) votes(%d)", pCharName[i], count[i]);
		PutLogList(G_cTxt);
	}

	//Save to file
	bSaveToFile();
}

//====================================================================
//			net message
//====================================================================
void CSovereign::SendSovMsgToAll(WORD wMsgType)
{
	char *cp = m_pData;
	DWORD *dwp = (DWORD*) cp;
	*dwp = MSGID_SOVEREIGN;
	cp += 4;

	WORD *wp = (WORD*) cp;
	*wp = wMsgType;
	cp += 2;

	switch(wMsgType)
	{
	case DEF_SOV_STARTVOTE:
		memcpy(cp, m_cLocation, 10);
		cp += 10;
		break;
	}

	m_pGame->SendMsgToAll(m_pData, (DWORD)(cp - m_pData));
}

/*----------------------------------------------------------------------
 *	return values:
 *	0: OK	
 *	1: character dest does not exist
 *	2: dest has not the right
 *	3: src has voted
 *----------------------------------------------------------------------*/
int CSovereign::iVote(char *src, char* dest)
{

	char cDir[MAX_PATH];
	sprintf(cDir, "Character\\AscII%d\\%s.txt", (unsigned char)dest[0], dest);
	FILE *pFile = fopen(cDir, "r");
	if(pFile == NULL)
		return 1;

	ZeroMemory(m_pData, sizeof(m_pData));
	fread(m_pData, 1, 600, pFile);
	fclose(pFile);
	char	seps[] = "= \t\n";
	CStrTok *pTok = new CStrTok(m_pData, seps);
	char *token = pTok->pGet();
	BOOL bCheck(FALSE);
	while(token)
	{
		char *str = "character-location";
		if(strcmp(str, token) == 0)
		{
			token = pTok->pGet();
			if(token == NULL)
			{
				delete pTok;
				return 2;
			}
			if(strcmp(token, m_cLocation) == 0)
				bCheck = TRUE;
			else
			{
				delete pTok;
				return 2;
			}
			break;
		}
		token = pTok->pGet();
	}
	delete pTok;
	if(bCheck == FALSE)
		return 2;


	string strSrc(src);
	if(m_mVote.find(strSrc) != m_mVote.end())
		return 3;

	string strDest(dest);
	m_mVote.insert(map<string, string>::value_type(strSrc, strDest));
	return 0;
}

int CSovereign::iComposeSovInfo(char* pData)
{
	char *cp = pData;
	memcpy(cp, m_cLocation, 10);
	cp += 10;

	memcpy(cp,m_cName, 10);
	cp += 10;

	int *ip = (int*)cp;
	*ip = m_iState;
	cp += 4;

	time_t *tp = (time_t*)cp;
	*tp = m_tBegin;
	cp += 4;

	return (int)(cp - pData);
}

//====================================================================
//			utility
//====================================================================

