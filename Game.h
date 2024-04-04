//2006-3-18 created by jcfly
#pragma once 
#include <string>
#include <map>
#include <Windows.h>
using namespace std;

#define DEF_MAXCLIENTS	20


class XSocket;
class CClient;
class CGuildsman;
class CGuild;
class CItem;
class CSovereign;

class CGame
{
public:
	CGame();
	~CGame();

	BOOL	bInit(HWND hWnd);
	BOOL	bOnClose();
	BOOL	bAccept();
	BOOL	DisplayInfo(){}

	void	OnTimer();
	void	OnClientSocketEvent( UINT message, WPARAM wParam, LPARAM lParam );

	void	SendMsgToAll(char *pData, DWORD dwSize);
private:
	BOOL	bReadConfigFile(const char* pFileName);
	void	OnClientRead(int iClientH);
	void	DeleteClient(int iClientH);
	//读取所有行会的信息，返回工会的数量
	int		iReadGuildsInfo();
	//Net message handler.
	void	CreateNewGuildHandler(int iClientH, char* pData);
	void	DisbandGuildHandler(int iClientH, char* pData);
	void	NewGuildsmanHandler(int iClientH, char* pData);
	void	DelGuildsmanHandler(int iClientH, char* pData);
	void	RegisterGameServerHandler(int iClientH, char* pData);
	void	NotifyGuildsmanRankHandler(int iClientH, char* pData);
	void	UpgradeGuildHandler(int iClientH, char* pData);
	void	ModifyBulletinHandler(int iClientH, char* pData);
	void	GuildContributeHandler(int iClientH, char* pData);
	void	VoteHandler(int iClientH, char *pData);
	void	BuyWarehouseHandler(int iClientH, char *pData);
	void	GetGuildItemHandler(int iClientH, char *pData);
	void	ModifyItemLevelHandler(int iClientH, char *pData);
	void	TransmitHandler(int iClientH, char *pData, DWORD dwMsgSize);
	void	ModifyWealthHandler(int iClientH, char *pData);
	void	ModifyGuildsmanTitleHandler(int iClientH, char *pData);
	void	GMCmdHandler(int iClientH, char* pData);
	//Send message
	BOOL	bSendGuildMsgToHG(int iClientH, DWORD dwMsg, WORD wMsgType, CGuild* pGuild = NULL, int iV1 = NULL,char* pString = NULL);
	BOOL	bCheckSendMsgResult(int iClientH, int iRet);
	void	SendGuildsInfoToHG(int iClientH);			//发送所有工会信息到 HGServer
	void	SendGuildNotifyMsg( WORD wMsgType, CGuild* pGuild = NULL, CGuildsman* pGuildsman = NULL, char* pString = NULL, int iV1 = NULL, int iV2 = NULL, CItem* pItem = NULL);	//send to all game servers

	void	SendSovMsgToHG(int iClientH, WORD wMsgType,char *pString = NULL,int iV1 = NULL);
	//utility
	CGuild* FindGuild(char* pGuildName);
private:

	char	m_cServerAddr[17];
	int		m_iPort;
	HWND	m_hWnd;
	XSocket*	m_pListenSock;
	CClient*	m_pClientList[DEF_MAXCLIENTS];
	//All guilds
	map<string, CGuild*>	m_mGuilds;

	char*	m_pData;

	CSovereign	*m_pSovereign[2];
};

