#include <windows.h>
#include <winbase.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <memory.h>
#include <direct.h>
#include <io.h>
#include <list>
#include <assert.h>

#include "winmain.h"
#include "StrTok.h"
#include "Xsocket.h"
#include "UserMessages.h"
#include "NetMessages.h"
#include "MessageIndex.h"
#include "Version.h"
#include "GlobalDef.h"
#include "StrTok.h"
#include "Game.h"
#include "Client.h"
#include "Guildsman.h"
#include "Guild.h"
#include "Item.h"
#include "Sovereign.h"

extern	char G_cTxt[1024];

#define	DEF_SERVERSOCKETBLOCKLIMIT	50

CGame::CGame():
	m_pListenSock(NULL)
{
	for(int i = 0;i < DEF_MAXCLIENTS; i++)
		m_pClientList[i] = NULL;

	for(int i = 0; i< 2; i++)
		m_pSovereign[i] = NULL;
	m_pData = new char[50 * 1024];		//50 K

}

CGame::~CGame()
{
	delete []m_pData;
	for(int i = 0 ;i < 2; i++)
	{
		delete m_pSovereign[i];
		m_pSovereign[i] = NULL;
	}
}

//======================================================================
//		Initialize
//======================================================================
BOOL CGame::bInit(HWND hWnd)
{
	m_hWnd = hWnd;

	if(bReadConfigFile("GuildServer.cfg") == FALSE)
	{
		PutLogList("(!!!) CRITICAL ERROR! error reading GuildServer.cfg");
		return FALSE;
	}
	int iCount = iReadGuildsInfo();
	sprintf(G_cTxt,"(!) %d guild-file read.",iCount);
	PutLogList(G_cTxt);

	char *pLocation[] = {
		"aresden",
		"elvine"
	};

	for(int i = 0; i < 2; i++)
	{
		m_pSovereign[i] = new CSovereign(this);
		if(m_pSovereign[i]->bInit(pLocation[i]) == FALSE)
		{
			sprintf(G_cTxt, "(!!!) Init Sovereign <%s> failed.", pLocation[i]);
			PutLogList(G_cTxt);
			return FALSE;
		}
	}
	PutLogList("(!) Init Sovereign successfully.");

	if (_InitWinsock() == FALSE) {
		MessageBox(hWnd, "Socket 2.2 not found! Cannot execute program.","ERROR", MB_ICONEXCLAMATION | MB_OK);
		return FALSE;
	}

	m_pListenSock = new XSocket(hWnd,DEF_SERVERSOCKETBLOCKLIMIT);
	if(m_pListenSock->bListen(m_cServerAddr, m_iPort, WM_USER_INTERNAL_ACCEPT) == FALSE)
	{
		PutLogList("(!!!) CRITICAL ERROR! Listen failed");
		return FALSE;
	}
	PutLogList("(!) Guild Server Listening...");
	return TRUE;
}


BOOL CGame::bReadConfigFile(const char* pFileName)
{
	assert(pFileName);
	FILE* pFile = fopen(pFileName,"r");
	if(pFile == NULL)
		return FALSE;

	int iLen = _filelength(pFile->_file);
	if(iLen == -1)
		return FALSE;
	PutLogList("(!)Reading Guild Server configuration file...");

	char* cp = new char[iLen + 2];
	ZeroMemory(cp, sizeof(cp));
	fread(cp, 1, iLen, pFile);
	fclose(pFile);

	char	seps[] = "= \t\n";
	CStrTok* pTok = new CStrTok(cp,seps);
	char* token = pTok->pGet();
	char cReadMode(0);
	while(token)
	{
		switch(cReadMode)
		{
		case 0:
			{
				const char* head[]= {
					"guild-server-address",
					"guild-server-port"
				};
				const char mode[] ={
					1, 
					2
				};
				int num = sizeof(head)/sizeof(char*);
				for(int i = 0;i < num;i++)
				{
					if(memcmp(token,head[i],strlen(head[i])) == 0)
					{
						cReadMode = mode[i];
						break;
					}
				}
			}
			break;
		case 1:
			strcpy(m_cServerAddr,token);
			sprintf(G_cTxt,"(*) Guild server address: %s",token);
			PutLogList(G_cTxt);
			cReadMode = 0;
			break;
		case 2:
			m_iPort = atoi(token);
			sprintf(G_cTxt,"(*) Guild server port: %s",token);
			PutLogList(G_cTxt);
			cReadMode = 0;
			break;
		}
		token = pTok->pGet();
	}
	delete []cp;
	delete pTok;

	return TRUE;
}

//close 
BOOL CGame::bOnClose()
{
	int iRet = MessageBox(m_hWnd, "确实要关闭 GuildServer 吗？", "GuildServer", MB_YESNO);
	if(iRet == IDNO)
		return FALSE;

	//Save data
	map<string, CGuild*>::iterator it;
	for(it = m_mGuilds.begin(); it != m_mGuilds.end(); it++)
	{
		CGuild* pGuild = it->second;
		if(pGuild->bSaveToFile() == FALSE)
		{
			sprintf(G_cTxt,"(!!!) Error saving guild-info <%s>", pGuild->m_cGuildName);
			PutLogList(G_cTxt);
		}
	}
	
	for(int i = 0;i < 2;i ++)
	{
		if(m_pSovereign[i] && (m_pSovereign[i]->bSaveToFile() == FALSE))
		{
			sprintf(G_cTxt,"(!!!) Error saving sovereign-info <%s>",m_pSovereign[i]->GetLocation());
			PutLogList(G_cTxt);
		}
	}

	PutLogList("(!) Data saved,the guildserver is shuting down");

	return TRUE;
}


void CGame::OnTimer()
{
	static int index(0);
	index = 1 - index;
	if(m_pSovereign[index])
		m_pSovereign[index]->OnTimer();
}

//======================================================================
//		Reading and writing guild-file
//======================================================================
int CGame::iReadGuildsInfo()
{
	list<string>	lstFile;

	_finddata_t filestruct;
	int p  = 0;
	int fn = 0;
	int num = 0;
	char szSearch[255];

	sprintf(szSearch, "Guild\\*.gild");

	intptr_t hnd = _findfirst(szSearch , &filestruct);

	if(hnd == -1) 
	{
		return 0;
	}
	do
	{ 	
		char szFullName[255];

		sprintf(szFullName , "Guild\\%s" , filestruct.name);

		string str(szFullName);

		if(!(filestruct.attrib & _A_SUBDIR)) // 如果文件属性不是’目录’
		{
			lstFile.push_back(str);
		}
	}while(!_findnext(hnd , &filestruct));

	list<string>::iterator it;
	int iCount(0);
	for(it = lstFile.begin();it != lstFile.end(); it++)
	{
		CGuild* pGuild = new CGuild();
		if(pGuild->bParseFile((*it).c_str()))
		{
			string strGuildName(pGuild->m_cGuildName);
			m_mGuilds.insert(map<string, CGuild*>::value_type(strGuildName, pGuild));
			iCount++;
		}
		else	
		{
			sprintf(G_cTxt, "(!!!) Erorr reading guild-file %s",(*it).c_str());
			PutLogList(G_cTxt);
		}
	}
	return iCount;
}

//======================================================================
//		Socket events
//======================================================================
BOOL CGame::bAccept()
{
	for(int i = 0;i < DEF_MAXCLIENTS; i++)
	{
		if(m_pClientList[i] == NULL)	
		{
			m_pClientList[i] = new CClient(m_hWnd);
			BOOL bRet = m_pListenSock->bAccept(m_pClientList[i]->GetSock(),WM_ONCLIENTSOCKETEVENT + i);
			assert(bRet);
			char cAddr[20];
			m_pClientList[i]->GetSock()->iGetPeerAddress(cAddr);
			sprintf(G_cTxt,"<%d> Accept new client: %s",i,cAddr);
			PutLogList(G_cTxt);
			
			return TRUE;
		}
	}
	XSocket* pTmpSock = new class XSocket(m_hWnd, DEF_SERVERSOCKETBLOCKLIMIT);
	m_pListenSock->bAccept(pTmpSock, NULL); 
	delete pTmpSock;
	return FALSE;
}

void CGame::OnClientSocketEvent( UINT message, WPARAM wParam, LPARAM lParam )
{
	int	iClientH, iRet;
	UINT	iTmp;

	iTmp = WM_ONCLIENTSOCKETEVENT;
	iClientH = message - iTmp;

	if( m_pClientList[iClientH] == NULL ) return;

	iRet = m_pClientList[iClientH]->GetSock()->iOnSocketEvent( wParam, lParam );

	switch( iRet )
	{
	case DEF_XSOCKEVENT_READCOMPLETE:
		OnClientRead( iClientH );
		//m_pClientList[iClientH]->m_dwTime = timeGetTime();
		break;

	case DEF_XSOCKEVENT_BLOCK:
		PutLogList( "Socket BLOCKED!" );
		break;

	case DEF_XSOCKEVENT_CONFIRMCODENOTMATCH:
		wsprintf( G_cTxt,"<%d> Confirmcode notmatch!", iClientH );
		PutLogList( G_cTxt );
		DeleteClient( iClientH);
		break;

	case DEF_XSOCKEVENT_MSGSIZETOOLARGE:
	case DEF_XSOCKEVENT_SOCKETERROR:
	case DEF_XSOCKEVENT_SOCKETCLOSED:
		wsprintf( G_cTxt,"<%d> HGServer connection Lost!,iRet<%d>", iClientH, iRet);
		PutLogList( G_cTxt );

		DeleteClient( iClientH);
		break;
	}
}

void CGame::OnClientRead(int iClientH)
{
	char * pData;
	BOOL bFlag = FALSE;
	DWORD dwTime = timeGetTime(), *dwpMsgID;
	//WORD *wp;
	//char cBuff[265], cTxt[20];
	DWORD dwMsgSize;

	pData = m_pClientList[iClientH]->GetSock()->pGetRcvDataPointer(&dwMsgSize, 0);
	dwpMsgID = (DWORD *)(pData);
	switch(*dwpMsgID) {
	case MSGID_REQUEST_CREATENEWGUILD:
		CreateNewGuildHandler(iClientH, pData);
		break;	
	case MSGID_REQUEST_DISBANDGUILD:
		DisbandGuildHandler(iClientH, pData);
		break;
	case MSGID_REQUEST_UPDATEGUILDINFO_NEWGUILDSMAN:
		NewGuildsmanHandler(iClientH, pData);
		break;
	case MSGID_REQUEST_UPDATEGUILDINFO_DELGUILDSMAN:
	case MSGID_REQUEST_DELGUILDSMAN:
		DelGuildsmanHandler(iClientH,pData);
		break;
	case MSGID_REQUEST_REGISTERGAMESERVER:
		RegisterGameServerHandler(iClientH, pData);
		break;
	case MSGID_REQUEST_UPGRADEGUILDSMAN:
	case MSGID_REQUEST_DEGRADEGUILDSMAN:
		NotifyGuildsmanRankHandler(iClientH, pData);
		break;
	case MSGID_REQUEST_UPGRADEGUILD:
		UpgradeGuildHandler(iClientH,pData);
		break;
	case MSGID_REQUEST_MODIFYBULLETIN:
		ModifyBulletinHandler(iClientH, pData);
		break;
	case MSGID_REQUEST_CONTRIBUTE:
		GuildContributeHandler(iClientH, pData);
		break;
	case MSGID_VOTE:
		VoteHandler(iClientH, pData);
		break;
	case MSGID_REQUEST_BUYWAREHOUSE:
		BuyWarehouseHandler(iClientH, pData);
		break;
	case MSGID_REQUEST_GETGUILDITEM:
		GetGuildItemHandler(iClientH, pData);
		break;
	case MSGID_REQUEST_MODIFYITEMLEVEL:
		ModifyItemLevelHandler(iClientH, pData);
		break;
	case MSGID_TRANSMIT:
		TransmitHandler(iClientH, pData, dwMsgSize); 
		break;
	case MSGID_REQUEST_MODIFYWEALTH:
		ModifyWealthHandler(iClientH, pData);
		break;
	case MSGID_REQUEST_MODIFYTITLE:
		ModifyGuildsmanTitleHandler(iClientH, pData);
		break;
	case MSGID_GM_COMMAND:
		GMCmdHandler(iClientH, pData);
		break;
	default:
		sprintf(G_cTxt,"(!!!) Unknown message: 0x%.8X",*dwpMsgID);
		PutLogList( G_cTxt);
	}
}

//======================================================================
//		Net message handler
//======================================================================
void CGame::CreateNewGuildHandler(int iClientH ,char* pData)
{
	char* cp;
	char cCharName[11],cGuildName[21], cLocation[11];
	int iGuildGUID,*ip;

	ZeroMemory(cCharName,sizeof(cCharName));
	ZeroMemory(cGuildName,sizeof(cGuildName));
	ZeroMemory(cLocation, sizeof(cLocation));

	cp =  pData + DEF_INDEX2_MSGTYPE + 2;

	memcpy(cCharName,cp, 10);					//Char name
	cp += 10;

//	cp +=10;									//account name
//	cp +=10;									//account password

	memcpy(cGuildName,cp, 20);					//guild name
	cp += 20;

	memcpy(cLocation, cp, 10);					//location
	cp += 10;

	ip = (int*)cp;								//Guild GUID
	iGuildGUID = *ip;
	cp +=4;

	string strGuildName(cGuildName);
	if(m_mGuilds.find(strGuildName) != m_mGuilds.end() || strcmp(cGuildName,"NONE") == 0)
	{
		bSendGuildMsgToHG(iClientH, MSGID_RESPONSE_CREATENEWGUILD,DEF_MSGTYPE_REJECT, NULL, NULL, cCharName);
		return;
	}

	CGuild* pGuild = new CGuild();
	strcpy(pGuild->m_cGuildName, cGuildName);
	strcpy(pGuild->m_cMasterName, cCharName);
	strcpy(pGuild->m_cLocation, cLocation);
	pGuild->m_iGuildGUID = iGuildGUID;
	__time64_t ltime;
	_time64(&ltime);
	struct tm *today = _localtime64( &ltime );
	sprintf(pGuild->m_cCreateTime,"%d/%d/%d",today->tm_year + 1900,today->tm_mon + 1, today->tm_hour);

	//Insert the master as a guildsman
	CGuildsman* pGuildsman = new CGuildsman();
	strcpy(pGuildsman->m_cName, cCharName);
	pGuildsman->m_cRank = 0;
	strcpy(pGuildsman->m_cTitle, "会长");
	pGuild->bAddGuildsman(pGuildsman);

	//save to file
	if(pGuild->bSaveToFile() == FALSE)
	{
		sprintf(G_cTxt,"(!!!) Error saveing guild info: %s", pGuild->m_cGuildName);
		PutLogList( G_cTxt);
		delete pGuild;
		bSendGuildMsgToHG(iClientH, MSGID_RESPONSE_CREATENEWGUILD,DEF_MSGTYPE_REJECT, NULL, NULL, cCharName);
		return;
	}

	m_mGuilds.insert(map<string, CGuild*>::value_type(strGuildName, pGuild));

	bSendGuildMsgToHG(iClientH,MSGID_RESPONSE_CREATENEWGUILD, DEF_MSGTYPE_CONFIRM, NULL, NULL, cCharName);
	//Notify all the game servers
	SendGuildNotifyMsg(DEF_NG_ADDNEWGUILD, pGuild);
	//
	sprintf(G_cTxt,"(!) New Guild<%s> Created by <%s>",cGuildName, cCharName);
	PutLogList( G_cTxt);
}

void CGame::DisbandGuildHandler(int iClientH, char* pData)
{
	char* cp;
	char cCharName[11],cGuildName[21];

	ZeroMemory(cCharName,sizeof(cCharName));
	ZeroMemory(cGuildName,sizeof(cGuildName));

	cp =  pData + DEF_INDEX2_MSGTYPE + 2;

	memcpy(cCharName,cp, 10);					//Char name
	cp += 10;

//	cp +=10;									//account name
//	cp +=10;									//account password

	memcpy(cGuildName,cp, 20);					//guild name
	cp += 20;

	string strGuildName(cGuildName);
	map<string, CGuild*>::iterator it;
	it = m_mGuilds.find(strGuildName);
	if(it != m_mGuilds.end())
	{
		CGuild* pGuild = it->second;
		if(strcmp(cCharName, pGuild->m_cMasterName) != 0)	//not the master
		{
			bSendGuildMsgToHG(iClientH, MSGID_RESPONSE_DISBANDGUILD, DEF_MSGTYPE_REJECT, NULL, NULL, cCharName);
			return;
		}

		bSendGuildMsgToHG(iClientH, MSGID_RESPONSE_DISBANDGUILD, DEF_MSGTYPE_CONFIRM, NULL, NULL,cCharName);
		//Notify all the game servers
		SendGuildNotifyMsg(DEF_NG_DELGUILD, pGuild);
		//
		sprintf(G_cTxt, "(!) Guild <%s> deleted",cGuildName);
		PutLogList(G_cTxt);
		//Delete the guild-file
		sprintf(G_cTxt, "Guild\\%s.gild",cGuildName);
		if(DeleteFile(G_cTxt) == FALSE)
		{
			sprintf(G_cTxt,"(!!!) Error deleting file %s.gild",cGuildName);
			PutLogList( G_cTxt);
		}

		delete pGuild;
		m_mGuilds.erase(it);
	}
	else
	{
		bSendGuildMsgToHG(iClientH, MSGID_RESPONSE_DISBANDGUILD, DEF_MSGTYPE_CONFIRM);
		sprintf(G_cTxt, "(!!!) Try to delete guild that not exists.<%s>",cGuildName);
		PutLogList( G_cTxt);
	}

}

void CGame::NewGuildsmanHandler(int iClientH, char* pData)
{
	char* cp;
	char cCharName[11],cGuildName[21];

	ZeroMemory(cCharName,sizeof(cCharName));
	ZeroMemory(cGuildName,sizeof(cGuildName));

	cp =  pData + DEF_INDEX2_MSGTYPE + 2;

	memcpy(cCharName,cp, 10);					//Char name
	cp += 10;

	memcpy(cGuildName,cp, 20);					//guild name
	cp += 20;

	string strGuildName(cGuildName);
	map<string,CGuild*>::iterator it = m_mGuilds.find(strGuildName);
	if(it != m_mGuilds.end())
	{
		CGuildsman* pGuildsman = new CGuildsman();
		strcpy(pGuildsman->m_cName, cCharName);
		pGuildsman->m_cRank = DEF_GUILDSTARTRANK;
		strcpy(pGuildsman->m_cTitle, "会员");

		it->second->bAddGuildsman(pGuildsman);
		//Notify all the game servers 
		SendGuildNotifyMsg(DEF_NG_ADDNEWGUILDSMAN, it->second, pGuildsman);
		//Save to file
		it->second->bSaveToFile();
	}
}

void CGame::DelGuildsmanHandler(int iClientH, char* pData)
{
	char* cp;
	char cCharName[11],cGuildName[21];

	ZeroMemory(cCharName,sizeof(cCharName));
	ZeroMemory(cGuildName,sizeof(cGuildName));

	cp =  pData + DEF_INDEX2_MSGTYPE + 2;

	memcpy(cCharName,cp, 10);					//Char name
	cp += 10;

	memcpy(cGuildName,cp, 20);					//guild name
	cp += 20;

	string strGuildName(cGuildName);
	map<string,CGuild*>::iterator it = m_mGuilds.find(strGuildName);
	if(it != m_mGuilds.end())
	{
		string strCharName(cCharName);
		it->second->DelGuildsman(strCharName);
		//notfiy
		SendGuildNotifyMsg(DEF_NG_DELGUILDSMAN, it->second, NULL,cCharName);
		//Save to file
		it->second->bSaveToFile();
	}
}

void CGame::RegisterGameServerHandler(int iClientH, char* pData)
{
	char* cp;
	char cServerName[11],cAddress[17];
	cp = pData + DEF_INDEX2_MSGTYPE +2;
	ZeroMemory(cServerName, sizeof(cServerName));
	ZeroMemory(cAddress,sizeof(cAddress));

	memcpy(cServerName, cp, 10);
	cp += 10;
	memcpy(cAddress, cp, 16);
	cp += 16;

	for(int i = 0;i < DEF_MAXCLIENTS;i++)
	{
		if(m_pClientList[i] == NULL)
			continue;
		if(strcmp(cServerName, m_pClientList[i]->m_cName) == 0)
		{
			sprintf(G_cTxt, "(!!!) Game server registeration failed, server (%s) already exsit", cServerName);
			PutLogList( G_cTxt);
			bSendGuildMsgToHG(iClientH,MSGID_RESPONSE_REGISTERGAMESERVER, DEF_MSGTYPE_REJECT);
			return;
		}
	}
	m_pClientList[iClientH]->m_bIsProcessingAvailable = TRUE;
	strcpy(m_pClientList[iClientH]->m_cName, cServerName);
	strcpy(m_pClientList[iClientH]->m_cIPAddress, cAddress);
	sprintf(G_cTxt, "(!) Game server registeration -success. server(%s) address(%s)",cServerName, cAddress);
	PutLogList( G_cTxt);
	bSendGuildMsgToHG(iClientH,MSGID_RESPONSE_REGISTERGAMESERVER,DEF_MSGTYPE_CONFIRM);
	//
	SendSovMsgToHG(iClientH, DEF_SOV_INFO);
	//
	SendGuildsInfoToHG(iClientH);
}

void CGame::ModifyBulletinHandler(int iClientH, char* pData)
{
	char* cp = pData + DEF_INDEX2_MSGTYPE + 2;

	char cGuildName[21];
	ZeroMemory(cGuildName, sizeof(cGuildName));
	memcpy(cGuildName,cp, 20);					
	cp += 20;

	CGuild *pGuild = FindGuild(cGuildName);
	if(pGuild == NULL)
	{
		sprintf(G_cTxt, "(!!!) Modify bulletin, guild not found %s", cGuildName);
		PutLogList( G_cTxt);
		return;
	}

	WORD *wp = (WORD*) cp;
	WORD wLen = *cp;
	cp += 2;

	if(wLen > DEF_MAX_BULLETINLEN)
	{
		sprintf(G_cTxt, "(!!!) Invalid bulletin len %d", wLen);
		PutLogList( G_cTxt);
		return;
	}

	pGuild->SetBulletin(cp, wLen);
	SendGuildNotifyMsg(DEF_NG_NOTIFYBULLETIN,pGuild);
	//Save to file
	pGuild->bSaveToFile();
}

void CGame::TransmitHandler(int iClientH, char *pData, DWORD dwMsgSize)
{
	SendMsgToAll(pData, dwMsgSize);
}

void CGame::NotifyGuildsmanRankHandler(int iClientH, char* pData)
{
	char *cp = pData + DEF_INDEX4_MSGID;
	DWORD *dwp;
	dwp = (DWORD*) cp;
	DWORD dwMsg = *dwp;
	cp += 4;
	cp += 2;
	
	char cGuildName[21], cCharName[11];
	ZeroMemory(cGuildName, sizeof(cGuildName));
	ZeroMemory(cCharName, sizeof(cCharName));

	memcpy(cCharName,cp, 10);					
	cp += 10;
	memcpy(cGuildName, cp, 20);
	cp += 20;

	CGuild *pGuild = FindGuild(cGuildName);
	CGuildsman *pGuildsman(NULL);
	if(pGuild)
		pGuildsman =pGuild->FindGuildsman(cCharName);

	if(pGuild == NULL)
	{
		sprintf(G_cTxt, "(!!!) NotifyGuildsmanRank.Guild not found <%s>", cGuildName);
		PutLogList( G_cTxt);
		return;
	}
	if(pGuildsman == NULL)
	{
		sprintf(G_cTxt,"(!!!) NotifyGuildsmanRank.Guildsman<%s> not found in guild <%s>", cCharName,cGuildName);
		PutLogList( G_cTxt);
		return;
	}
	if(dwMsg == MSGID_REQUEST_UPGRADEGUILDSMAN)
		pGuildsman->m_cRank --;
	else 
		pGuildsman->m_cRank ++;
	
	if(pGuildsman->m_cRank < 1 || pGuildsman->m_cRank > DEF_GUILDSTARTRANK)
	{
		sprintf(G_cTxt,"(!!!) NotifyGuildsmanRank. Invalid rank <%s>", pGuildsman->m_cRank);
		PutLogList( G_cTxt);

		if(pGuildsman->m_cRank < 1)
			pGuildsman->m_cRank = 1;
		else if(pGuildsman->m_cRank > DEF_GUILDSTARTRANK)
			pGuildsman->m_cRank = DEF_GUILDSTARTRANK;
	}
	//notify
	SendGuildNotifyMsg(DEF_NG_NOTIFYRANK, pGuild, pGuildsman);
	//Save to file
	pGuild->bSaveToFile();
}

void CGame::UpgradeGuildHandler(int iClientH, char* pData)
{
	char* cp = pData + DEF_INDEX2_MSGTYPE + 2;

	char cGuildName[21];
	ZeroMemory(cGuildName, sizeof(cGuildName));

	memcpy(cGuildName, cp, 20);
	cp += 20;

	CGuild* pGuild = FindGuild(cGuildName);
	if(pGuild == NULL)
	{
		sprintf(G_cTxt,"(!!!) Upgrade guild. Guild <%s> not found", cGuildName);
		PutLogList( G_cTxt);
		return;
	}

	int iGold[] = {500000, 1000000,2000000,4000000};

	if(pGuild->m_cLevel < 1 || pGuild->m_cLevel >=5)	
	{
		assert(false);
		return;
	}
	int index = pGuild->m_cLevel -1;
	pGuild->m_dwBlackPoint = 0;
	pGuild->m_dwGuildPoint = 0;
	pGuild->m_dwGold -= iGold[index];
	pGuild->m_cLevel ++;

	//notify
	SendGuildNotifyMsg(DEF_NG_UPGRADEGUILD, pGuild);
	//Save to file
	pGuild->bSaveToFile();
}

void CGame::GuildContributeHandler(int iClientH, char* pData)
{
	char* cp = pData + DEF_INDEX2_MSGTYPE + 2;

	char cGuildName[21], cCharName[11];
	ZeroMemory(cGuildName,sizeof(cGuildName));
	ZeroMemory(cCharName,sizeof(cCharName));

	memcpy(cGuildName, cp, 20);
	cp += 20;
	memcpy(cCharName, cp, 10);
	cp += 10;

	int iBlackPoint, iGold, *ip;
	ip = (int*)cp;
	iBlackPoint = *ip;
	cp += 4;

	ip = (int*)cp;
	iGold = *ip;
	cp += 4;

	char cItem = *cp;
	cp ++;
	CItem *pItem(NULL);
	if(cItem == 1)
	{
		pItem = new CItem();

		cp += pItem->iDecodeItemInfo(cp);
	}

	CGuild *pGuild = FindGuild(cGuildName);
	CGuildsman *pGuildsman(NULL);
	if(pGuild)
		pGuildsman = pGuild->FindGuildsman(cCharName);

	if(pGuildsman == NULL)
	{
		char cTxt[200];
		strcpy(G_cTxt,"(!!!) Contribute.");
		if(pGuild == NULL)
			sprintf(cTxt, "Guild <%s> not found", cGuildName);
		else 
			sprintf(cTxt, "Guildsman <%s> not found in Guild <%s>",cGuildName, cCharName);
		strcat(G_cTxt, cTxt);
		PutLogList( G_cTxt);
        sprintf(G_cTxt, "(!) BlackPoint <%d> Gold <%d> Item<%s> Attri <%d>", iBlackPoint, iGold, pItem->m_cName,pItem->m_dwAttribute);
		PutLogList( G_cTxt);
		return;
	}
	if(iBlackPoint > 0)
	{
		pGuild->m_dwBlackPoint += iBlackPoint;
		pGuild->m_dwGuildPoint = pGuild->m_dwBlackPoint / 10;
		pGuildsman->m_dwBlackPointContri += iBlackPoint;
	}
	if(iGold > 0)
	{
		pGuild->m_dwGold += iGold;
		pGuildsman->m_dwGoldContri += iGold;
	}
	if(pItem != NULL)
	{
		if(pGuild->m_bHasWarehouse == FALSE)
		{
			sprintf(G_cTxt,"(!!!) Contribute. Guild <%s> do not has a storehouse", pGuild->m_cGuildName);
			PutLogList( G_cTxt);
			return;
		}
		int num = pGuild->GetItemNum();
		if(num >= DEF_MAXGUILDITEMS)
		{
			sprintf(G_cTxt,"(!!!) Contribute. Guild <%s> storehouse is full", pGuild->m_cGuildName);
			PutLogList( G_cTxt);
			return;
		}
		int index = pGuild->AddItem(pItem);
		assert(index != -1);

		sprintf(G_cTxt, "@ %s contribute item %s", cCharName, pItem->m_cName);
		PutLogList( G_cTxt);
	}

	SendGuildNotifyMsg(DEF_NG_CONTRIBUTE, pGuild, pGuildsman, NULL, iBlackPoint, iGold, pItem);
	//Save to file
	pGuild->bSaveToFile();
}

void CGame::VoteHandler(int iClientH, char *pData)
{
	char *cp = pData + DEF_INDEX2_MSGTYPE + 2;
	char cLocation[11], cSrc[11], cDest[11];
	ZeroMemory(cLocation, sizeof(cLocation));
	ZeroMemory(cSrc, sizeof(cSrc));
	ZeroMemory(cDest, sizeof(cDest));

	memcpy(cLocation, cp, 10);
	cp += 10;
	memcpy(cSrc, cp, 10);
	cp += 10;
	memcpy(cDest, cp, 10);
	cp += 10;

	for(int i = 0;i < 2; i ++)
	{
		if(strcmp(m_pSovereign[i]->GetLocation(), cLocation) == 0)
		{
			int iRet = m_pSovereign[i]->iVote(cSrc,cDest);
			SendSovMsgToHG(iClientH, DEF_SOV_VOTERESPONSE, cSrc, iRet);
			return;
		}
	}
	sprintf(G_cTxt,"(!!!) Vote. Invalid location <%s>", cLocation);
	PutLogList( G_cTxt);
}

void CGame::BuyWarehouseHandler(int iClientH, char *pData)
{
	char cGuildName[21];
	ZeroMemory(cGuildName,sizeof(cGuildName));

	char *cp =  pData + DEF_INDEX2_MSGTYPE + 2;
	
	memcpy(cGuildName,cp, 20);					//guild name
	cp += 20;

	CGuild *pGuild = FindGuild(cGuildName);
	if(pGuild == NULL)
	{
		sprintf(G_cTxt,"(!!!) ButStorehourseHandler, Guild <%s> not found", cGuildName);
		PutLogList( G_cTxt);
		return;
	}
	if(pGuild->m_bHasWarehouse)
		return;
	if(pGuild->m_dwGold < 200000)
		return;

	pGuild->m_dwGold -= 200000;
	pGuild->m_bHasWarehouse = TRUE;
	//Notify
	SendGuildNotifyMsg(DEF_NG_BUYWAREHOUSE, pGuild);
	//Save to file
	pGuild->bSaveToFile();
}

void CGame::GetGuildItemHandler(int iClientH, char *pData)
{
	char* cp = pData + DEF_INDEX2_MSGTYPE + 2;

	char cGuildName[21], cCharName[11];
	ZeroMemory(cGuildName,sizeof(cGuildName));
	ZeroMemory(cCharName,sizeof(cCharName));

	memcpy(cCharName, cp, 10);
	cp += 10;
	memcpy(cGuildName, cp, 20);
	cp += 20;

	short *sp = (short*) cp;
	short sItem = *sp;
	cp += 2;

	CGuild *pGuild = FindGuild(cGuildName);

	if(pGuild == NULL)
	{
		sprintf(G_cTxt,"(!!!) GetGuildItemHandler, Guild <%s> not found", cGuildName);
		PutLogList( G_cTxt);
		return;
	}
	CGuildsman *pGuildsman = pGuild->FindGuildsman(cCharName);
	if(pGuildsman == NULL)
	{
		sprintf(G_cTxt,"(!!!) GetGuildItemHandler. Guildsman <%s> (guild< %s>) not found", cCharName, cGuildName);
		PutLogList( G_cTxt);
		return;
	}
	if(pGuild->m_bHasWarehouse == FALSE)
		return;
	if(sItem < 0 || sItem >= DEF_MAXGUILDITEMS)
		return;
	if(pGuild->m_pGuildItems[sItem] == NULL)
		return;
	SendGuildNotifyMsg(DEF_NG_GETGUILDITEM,pGuild,pGuildsman,NULL, sItem, NULL, pGuild->m_pGuildItems[sItem]);
	
	//delete the item
	sprintf(G_cTxt, "(!) Player <%s> (guild <%s>) GetItem <%s> Attri<%d>",cCharName, cGuildName, 
		pGuild->m_pGuildItems[sItem]->m_cName, pGuild->m_pGuildItems[sItem]->m_dwAttribute);
	PutLogList( G_cTxt);
	delete pGuild->m_pGuildItems[sItem];
	pGuild->m_pGuildItems[sItem] = NULL;
	//Save to file
	pGuild->bSaveToFile();
}

void CGame::ModifyItemLevelHandler(int iClientH, char *pData)
{
	char* cp = pData + DEF_INDEX2_MSGTYPE + 2;

	char cGuildName[21];
	ZeroMemory(cGuildName,sizeof(cGuildName));

	memcpy(cGuildName, cp, 20);
	cp += 20;

	short *sp = (short*) cp;
	short sItem = *sp;
	cp += 2;

	sp = (short*) cp;
	short sType = *sp;
	cp += 2;

	CGuild *pGuild = FindGuild(cGuildName);
	if(pGuild == NULL)
	{
		sprintf(G_cTxt,"(!!!) ModifyItemLevelHandler, Guild <%s> not found", cGuildName);
		PutLogList( G_cTxt);
		return;
	}

	if(sItem < 0 || sItem >= DEF_MAXGUILDITEMS)
		return;
	if(pGuild->m_bHasWarehouse == FALSE || pGuild->m_pGuildItems[sItem] == NULL)
		return;

	char cLevel = pGuild->m_pGuildItems[sItem]->cGetLevel();
	if(sType == 1)
		cLevel --;
	else if(sType == 2)
		cLevel ++;
	else
	{
		sprintf(G_cTxt,"(!!!) ModifyItemLevelHandler. Invalid item level <%d>", cLevel);
		PutLogList( G_cTxt);
		return;
	}
	if(cLevel < 1)
		cLevel = 1;
	else if(cLevel > DEF_GUILDSTARTRANK + 1)
		cLevel = DEF_GUILDSTARTRANK + 1;
	pGuild->m_pGuildItems[sItem]->SetLevel(cLevel);
	//Notify
	SendGuildNotifyMsg(DEF_NG_MODIFYITEMLEVEL, pGuild, NULL,NULL, sItem, cLevel);
	//Save to file
	pGuild->bSaveToFile();

}

void CGame::ModifyWealthHandler(int iClientH, char *pData)
{
	char* cp = pData + DEF_INDEX2_MSGTYPE + 2;

	char cGuildName[21];
	ZeroMemory(cGuildName,sizeof(cGuildName));

	memcpy(cGuildName, cp, 20);
	cp += 20;

	BOOL bPlus = (BOOL)(*cp);
	cp ++;

	DWORD *dwp = (DWORD*)cp;
	DWORD dwGuildBlackPoint = *dwp;
	cp += 4;
	dwp = (DWORD*) cp;
	DWORD dwGuildGold = *dwp;
	cp += 4;

    
	CGuild *pGuild = FindGuild(cGuildName);
	if(pGuild == NULL)
	{
		sprintf(G_cTxt,"(!!!) ModifyWealthHandler, Guild <%s> not found", cGuildName);
		PutLogList( G_cTxt);
		return;
	}
	
	if(bPlus)
	{
		pGuild->m_dwBlackPoint += dwGuildBlackPoint;
		pGuild->m_dwGold += dwGuildGold;
	}
	else
	{
		if(pGuild->m_dwBlackPoint < dwGuildBlackPoint)
			pGuild->m_dwBlackPoint = 0;
		else
			pGuild->m_dwBlackPoint -= dwGuildBlackPoint;

		if(pGuild->m_dwGold < dwGuildGold)
			pGuild->m_dwGold = 0;
		else
			pGuild->m_dwGold -= dwGuildGold;
	}

	pGuild->m_dwGuildPoint = pGuild->m_dwBlackPoint /10;

	SendGuildNotifyMsg(DEF_NG_MODIFYWEALTH, pGuild);

	//Save to file
	pGuild->bSaveToFile();

}

void CGame::ModifyGuildsmanTitleHandler(int iClientH, char *pData)
{
	char* cp = pData + DEF_INDEX2_MSGTYPE + 2;

	char cGuildName[21],cCharName[11],cTitle[21];
	ZeroMemory(cGuildName,sizeof(cGuildName));
	ZeroMemory(cCharName, sizeof(cCharName));
	ZeroMemory(cTitle,sizeof(cTitle));

	memcpy(cGuildName, cp, 20);
	cp += 20;
	memcpy(cCharName, cp, 10);
	cp += 10;
	memcpy(cTitle, cp, 20);
	cp += 20;

	CGuild *pGuild = FindGuild(cGuildName);
	if(pGuild == NULL)
	{
		sprintf(G_cTxt,"(!!!) ModifyGuildsmanTitleHandler, Guild <%s> not found", cGuildName);
		PutLogList( G_cTxt);
		return;
	}
	CGuildsman *pGuildsman = pGuild->FindGuildsman(cCharName);
	if(pGuildsman == NULL)
	{
		sprintf(G_cTxt,"(!!!) ModifyGuildsmanTitleHandler. Guildsman <%s> (guild< %s>) not found", cCharName, cGuildName);
		PutLogList( G_cTxt);
		return;
	}

	strcpy(pGuildsman->m_cTitle, cTitle);
	//Notify
	SendGuildNotifyMsg(DEF_NG_NOTIFYTITLE, pGuild, pGuildsman);
	//Save to file
	pGuild->bSaveToFile();
}


void CGame::GMCmdHandler(int iClientH, char* pData)
{
	char* cp = pData + DEF_INDEX2_MSGTYPE;
	WORD* wp = (WORD*) cp;
	WORD wMsgType = *wp;
	cp += 2;
	if(wMsgType == DEF_GMCMD_STARTVOTE || wMsgType == DEF_GMCMD_ENDVOTE)
	{
		char cLocation[11];
		ZeroMemory(cLocation, sizeof(cLocation));
		memcpy(cLocation, cp, 10);
		cp += 10;
		for(int i = 0;i < 2; i ++)
		{
			if(strcmp(m_pSovereign[i]->GetLocation(), cLocation) == 0)
			{
				char cMsg[50];
				ZeroMemory(cMsg, sizeof(cMsg));
				switch(wMsgType)
				{
				case DEF_GMCMD_STARTVOTE:
					strcpy(cMsg, "(!) GM cmd: start vote");
					m_pSovereign[i]->StartVote();
					break;
				case DEF_GMCMD_ENDVOTE:
					strcpy(cMsg, "(!) GM cmd: end vote");
					m_pSovereign[i]->EndVote();
					break;
				default:
					assert(false);
				}
				PutLogList(cMsg);
			}
		}
	}
}

//======================================================================
//		Send message
//======================================================================
BOOL CGame::bSendGuildMsgToHG(int iClientH, DWORD dwMsg, WORD wMsgType, CGuild* pGuild /* = NULL */, int iV1 /* = NULL */,char* pString /* = NULL */)
{
	char *cp;
	WORD *wp;
	DWORD* dwp;
	int iRet;

	assert(m_pClientList[iClientH]);
	if(m_pClientList[iClientH] == NULL)
		return FALSE;

	cp = m_pData;
	dwp = (DWORD*)cp;
	*dwp = dwMsg;
	cp += 4;
	wp =(WORD*) cp;
	*wp = wMsgType;
	cp += 2;

	switch(dwMsg)
	{ 
	case MSGID_RESPONSE_CREATENEWGUILD:
	case MSGID_RESPONSE_DISBANDGUILD:
		memcpy(cp, pString, 10);
		cp += 10;
		break;
	case MSGID_END_GUILDINFO:
	case MSGID_RESPONSE_REGISTERGAMESERVER:
		break;
	case MSGID_BEGIN_GUILDINFO:
		wp =(WORD*) cp;
		*wp = (WORD) iV1;
		cp += 2;
		break;
	case MSGID_GUILD_INFO:
		cp += pGuild->iComposeGuildInfo(cp, FALSE);
		break;
	default:
		sprintf(G_cTxt,"(!!!) SendMsg ,Unknown MSG :%d dwMsg",dwMsg);
		PutLogList( G_cTxt);
		return FALSE;
	}

	iRet = m_pClientList[iClientH]->iSendMsg(m_pData,(DWORD)(cp - m_pData));
	return bCheckSendMsgResult(iClientH,iRet);
}

BOOL CGame::bCheckSendMsgResult(int iClientH, int iRet)
{
	switch( iRet )
	{
	case DEF_XSOCKEVENT_QUENEFULL:
	case DEF_XSOCKEVENT_SOCKETERROR:
	case DEF_XSOCKEVENT_CRITICALERROR:
	case DEF_XSOCKEVENT_SOCKETCLOSED:
		wsprintf(G_cTxt, "<%d> HGServer connection lost! iRet<%d>", iClientH,iRet);
		PutLogList( G_cTxt);
		DeleteClient(iClientH);
		return FALSE;
	}
	return TRUE;
}

void CGame::SendGuildsInfoToHG(int iClientH)
{
	if(m_pClientList[iClientH] == NULL)
		return;

	if(bSendGuildMsgToHG(iClientH, MSGID_BEGIN_GUILDINFO,NULL,NULL,(int)m_mGuilds.size()) == FALSE)
		return;
	map<string,CGuild*>::iterator it;
	for(it = m_mGuilds.begin(); it != m_mGuilds.end();it ++)
	{
		bSendGuildMsgToHG(iClientH, MSGID_GUILD_INFO, NULL, it->second);
	}

	bSendGuildMsgToHG(iClientH,MSGID_END_GUILDINFO, NULL);
}

void CGame::SendSovMsgToHG(int iClientH, WORD wMsgType,char *pString /* = NULL */,int iV1 /* = NULL */)
{
	if(m_pClientList[iClientH] == NULL)
		return;

	short *sp;
	char *cp = m_pData;
	DWORD *dwp = (DWORD*) cp;
	*dwp = MSGID_SOVEREIGN;
	cp += 4;
	
	WORD *wp = (WORD*) cp;
	*wp = wMsgType;
	cp += 2;

	switch(wMsgType)
	{
	case DEF_SOV_INFO:
		for(int i = 0;i < 2;i ++)
			cp += m_pSovereign[i]->iComposeSovInfo(cp);
		break;
	case DEF_SOV_VOTERESPONSE:
		memcpy(cp, pString, 10);
		cp += 10;
		sp = (short*) cp;
		*sp = (short) iV1;
		cp += 2;
		break;
	}
    int iRet = m_pClientList[iClientH]->iSendMsg(m_pData,(DWORD)(cp - m_pData));
	bCheckSendMsgResult(iClientH, iRet);
}

void CGame::SendGuildNotifyMsg( WORD wMsgType, CGuild* pGuild /* = NULL */, CGuildsman* pGuildsman /* = NULL */, char* pString /* = NULL */, int iV1 /* = NULL */, int iV2 /* = NULL */, CItem* pItem /* = NULL */)
{
	char *cp;
	WORD *wp;
	DWORD* dwp;
	int *ip;
	short *sp;

	cp = m_pData;
	dwp = (DWORD*)cp;
	*dwp = MSGID_NOTIFY_GUILD;
	cp += 4;
	wp =(WORD*) cp;
	*wp = wMsgType;
	cp += 2;

	switch(wMsgType)
	{
	case DEF_NG_ADDNEWGUILD:
		memcpy(cp, pGuild->m_cGuildName, 20);
		cp += 20;
		
		ip = (int*)cp;
		*ip = pGuild->m_iGuildGUID;
		cp += 4;

		memcpy(cp,pGuild->m_cMasterName, 10);
		cp += 10;

		memcpy(cp, pGuild->m_cCreateTime, 10);
		cp += 10;

		memcpy(cp, pGuild->m_cLocation, 10);
		cp += 10;
		break;
	case DEF_NG_DELGUILD:
		memcpy(cp, pGuild->m_cGuildName, 20);
		cp += 20;
		break;
	case DEF_NG_ADDNEWGUILDSMAN:
		memcpy(cp, pGuild->m_cGuildName, 20);
		cp += 20;

		memcpy(cp, pGuildsman->m_cName, 10);
		cp += 10;
		break;
	case DEF_NG_DELGUILDSMAN:
		memcpy(cp, pGuild->m_cGuildName, 20);
		cp += 20;

		memcpy(cp, pString, 10);
		cp += 10;
		break;
	case DEF_NG_NOTIFYBULLETIN:
		{
			memcpy(cp, pGuild->m_cGuildName, 20);
			cp += 20;
			
			WORD wLen =(WORD)strlen(pGuild->m_cBulletin);

			wp = (WORD*) cp;
			*wp = wLen;
			cp += 2;

			memcpy(cp, pGuild->m_cBulletin, wLen);
			cp += wLen;

			break;
		}
	case DEF_NG_NOTIFYRANK:
		memcpy(cp, pGuild->m_cGuildName, 20);
		cp += 20;

		memcpy(cp, pGuildsman->m_cName, 10);
		cp += 10;

		*cp = pGuildsman->m_cRank;
		cp ++;
		break;
	case DEF_NG_CONTRIBUTE:
		memcpy(cp, pGuild->m_cGuildName, 20);
		cp += 20;

		memcpy(cp, pGuildsman->m_cName, 10);
		cp += 10;

		ip = (int*) cp;
		*ip = iV1;
		cp += 4;

		ip = (int*) cp;
		*ip = iV2;
		cp += 4;

		if(pItem == NULL)	{
			*cp = 0;
			cp ++;
		}
		else {
			*cp = 1;
			cp ++;

			cp += pItem->iComposeItemInfo(cp);
		}
		break;
	case DEF_NG_UPGRADEGUILD:
		memcpy(cp,pGuild->m_cGuildName, 20);
		cp += 20;

		*cp = pGuild->m_cLevel;
		cp ++;

		dwp = (DWORD*) cp;
		*dwp = pGuild->m_dwBlackPoint;
		cp += 4;

		dwp = (DWORD*) cp;
		*dwp = pGuild->m_dwGold;
		cp += 4;
		break;
	case DEF_NG_BUYWAREHOUSE:
		memcpy(cp,pGuild->m_cGuildName, 20);
		cp += 20;

		dwp = (DWORD*) cp;			//gold
		*dwp = pGuild->m_dwGold;
		cp += 4;
		break;
	case DEF_NG_GETGUILDITEM:
		memcpy(cp, pGuild->m_cGuildName, 20);
		cp += 20;

		memcpy(cp, pGuildsman->m_cName, 10);
		cp += 10;

		sp = (short*) cp;
		*sp = (short) iV1;
		cp += 2;

		cp += pItem->iComposeItemInfo(cp);
		break;
	case DEF_NG_MODIFYITEMLEVEL:
		memcpy(cp,pGuild->m_cGuildName, 20);
		cp += 20;

		sp = (short*) cp;
		*sp = (short) iV1;
		cp += 2;

		*cp = (char) iV2;
		cp ++;
		break;
	case DEF_NG_MODIFYWEALTH:
		memcpy(cp,pGuild->m_cGuildName, 20);
		cp += 20;
		//BlackPoint
		dwp = (DWORD*) cp;
		*dwp = pGuild->m_dwBlackPoint;
		cp += 4;
		//Gold
		dwp = (DWORD*) cp;
		*dwp = pGuild->m_dwGold;
		cp += 4;
		break;
	case DEF_NG_NOTIFYTITLE:
		memcpy(cp,pGuild->m_cGuildName, 20);
		cp += 20;
		//CharName
		memcpy(cp,pGuildsman->m_cName,10);
		cp += 10;
		//Title
		memcpy(cp,pGuildsman->m_cTitle,20);
		cp += 20;
		break;
	default:
		sprintf(G_cTxt, "(!!!) Unkown notify msg <%d>", wMsgType);
		PutLogList( G_cTxt);
		return;
	}
	
	SendMsgToAll(m_pData, (DWORD)(cp - m_pData));
}


void CGame::SendMsgToAll(char *pData, DWORD dwSize)
{
	for(int i = 0;i < DEF_MAXCLIENTS; i++)
	{
		if(m_pClientList[i] != NULL)
		{
			int iRet = m_pClientList[i]->iSendMsg(pData,dwSize);
			bCheckSendMsgResult(i, iRet);
		}
	}
}
//======================================================================
//		Utility
//======================================================================
__inline void CGame::DeleteClient(int iClientH)
{
	delete m_pClientList[iClientH];
	m_pClientList[iClientH] = NULL;
}

CGuild* CGame::FindGuild(char* pGuildName)
{
	string strGuildName(pGuildName);
	map<string,CGuild*>::iterator it = m_mGuilds.find(strGuildName);
	if(it == m_mGuilds.end())
		return NULL;
	return it->second;
}