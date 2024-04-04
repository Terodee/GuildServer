// Item.cpp: implementation of the CItem class.
//
//////////////////////////////////////////////////////////////////////

#include "Item.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CItem::CItem()
{
	ZeroMemory(m_cName, sizeof(m_cName));
	m_sSprite = 0;
	m_sSpriteFrame = 0;
											  
	m_sItemEffectValue1 = 0;
	m_sItemEffectValue2 = 0;
	m_sItemEffectValue3 = 0; 

	m_sItemEffectValue4 = 0;
	m_sItemEffectValue5 = 0;
	m_sItemEffectValue6 = 0; 

	m_dwCount = 1;
	m_sTouchEffectType   = 0;
	m_sTouchEffectValue1 = 0;
	m_sTouchEffectValue2 = 0;
	m_sTouchEffectValue3 = 0;
	
	m_cItemColor = 0;
	m_sItemSpecEffectValue1 = 0;
	m_sItemSpecEffectValue2 = 0;
	m_sItemSpecEffectValue3 = 0;

	m_sSpecialEffectValue1 = 0;
	m_sSpecialEffectValue2 = 0;
	
	m_wCurLifeSpan = 0;
	m_dwAttribute   = 0;

	m_cCategory = NULL;
	m_sIDnum    = 0;

	m_bIsForSale = FALSE;
}

CItem::~CItem()
{

}

//2006-3-27 added by jcfly
int CItem::iComposeItemInfo(char *pData)
{
	DWORD *dwp;
	short *sp;
	WORD *wp;
	char *cp = pData;

	strcpy(cp, m_cName);
	cp += 20;
	dwp = (DWORD*) cp;
	*dwp = m_dwCount;
	cp += 4;
	sp = (short*) cp;
	*sp = m_sTouchEffectType;
	cp += 2;
	sp = (short*) cp;
	*sp = m_sTouchEffectValue1;
	cp += 2;
	sp = (short*) cp;
	*sp = m_sTouchEffectValue2;
	cp += 2;
	sp = (short*) cp;
	*sp = m_sTouchEffectValue3;
	cp += 2;
	*cp = m_cItemColor;
	cp ++;
	sp = (short*) cp;
	*sp = m_sItemSpecEffectValue1;
	cp += 2;
	sp = (short*) cp;
	*sp = m_sItemSpecEffectValue2;
	cp += 2;
	sp = (short*) cp;
	*sp = m_sItemSpecEffectValue3;
	cp += 2;
	wp = (WORD*) cp;
	*wp = m_wCurLifeSpan;
	cp += 2;
	dwp = (DWORD*) cp;
	*dwp = m_dwAttribute;
	cp += 4;

	return (int)(cp - pData);
}

int CItem::iDecodeItemInfo(char* pData)
{
	DWORD *dwp;
	WORD *wp;
	short *sp;
	char *cp = pData;

	//the name must be in the beginning of pData
	strcpy(m_cName, cp);
	cp += 20;
	dwp = (DWORD*) cp;
	m_dwCount = *dwp;
	cp += 4;
	sp = (short*) cp;
	m_sTouchEffectType = *sp;
	cp += 2;
	sp = (short*) cp;
	m_sTouchEffectValue1 = *sp;
	cp += 2;
	sp = (short*) cp;
	m_sTouchEffectValue2 = *sp;
	cp += 2;
	sp = (short*) cp;
	m_sTouchEffectValue3 = *sp;
	cp += 2;
	m_cItemColor = *cp;
	cp ++;
	sp = (short*) cp;
	m_sItemSpecEffectValue1 = *sp;
	cp += 2;
	sp = (short*) cp;
	m_sItemSpecEffectValue2 = *sp;
	cp += 2;
	sp = (short*) cp;
	m_sItemSpecEffectValue3 = *sp;
	cp += 2;
	wp = (WORD*) cp;
	m_wCurLifeSpan = *wp;
	cp += 2;
	dwp = (DWORD*) cp;
	m_dwAttribute = *dwp;
	cp += 4;

	return (int)(cp - pData);
}

//2006-3-29 added by jcfly
int CItem::iComposeClientItemInfo(char *pData)
{
	DWORD *dwp;
	WORD *wp;
	short *sp;

	char *cp = pData;

	memcpy( cp, m_cName, 20 );
	cp += 20;

	dwp  = (DWORD *)cp;
	*dwp = m_dwCount;
	cp += 4;

	*cp = m_cItemType;
	cp++;

	*cp = m_cEquipPos;
	cp++;

	*cp = (char)0; 
	cp++;

	sp  = (short *)cp;
	*sp = m_sLevelLimit;
	cp += 2;

	*cp = m_cGenderLimit;
	cp++;

	wp = (WORD *)cp;
	*wp = m_wCurLifeSpan;
	cp += 2;

	wp = (WORD *)cp;
	*wp = m_wWeight;
	cp += 2;

	sp  = (short *)cp;
	*sp = m_sSprite;
	cp += 2;

	sp  = (short *)cp;
	*sp = m_sSpriteFrame;
	cp += 2;

	*cp = m_cItemColor; 
	cp++;

	*cp = m_cSpeed;
	cp ++;

	sp =(short*) cp;
	*sp = m_sItemSpecEffectValue1; 
	cp += 2;

	sp =(short*) cp;
	*sp = m_sItemSpecEffectValue2; 
	cp += 2;

	sp =(short*) cp;
	*sp = m_sItemEffectValue1; 
	cp += 2;

	sp =(short*) cp;
	*sp = m_sItemEffectValue2; 
	cp += 2;

	sp =(short*) cp;
	*sp = m_sItemEffectValue3; 
	cp += 2;

	sp =(short*) cp;
	*sp = m_sItemEffectValue4; 
	cp += 2;

	sp =(short*) cp;
	*sp = m_sItemEffectValue5; 
	cp += 2;

	sp =(short*) cp;
	*sp = m_sItemEffectValue6; 
	cp += 2;

	dwp = (DWORD *)cp;
	*dwp = m_dwAttribute;
	cp += 4;

	return (int)(cp - pData);
}

int CItem::iDecodeClientItemInfo(char *pData)
{
	DWORD *dwp;
	WORD *wp;
	short *sp;

	char *cp = pData;

	ZeroMemory(m_cName, sizeof(m_cName));
	memcpy(m_cName, cp, 20);
	cp += 20;

	dwp = (DWORD *)cp;
	m_dwCount = *dwp;
	cp += 4;

	m_cItemType= *cp;
	cp++;

	m_cEquipPos= *cp;
	cp++;

	BOOL bIsEquipped = (BOOL)*cp;
	cp++;

	sp = (short *)cp;
	m_sLevelLimit = *sp;
	cp += 2;

	m_cGenderLimit = *cp;
	cp++;

	wp = (WORD *)cp;
	m_wCurLifeSpan = *wp;
	cp += 2;

	wp = (WORD *)cp;
	m_wWeight = *wp;
	cp += 2;

	sp = (short *)cp;
	m_sSprite = *sp;
	cp += 2;

	sp = (short *)cp;
	m_sSpriteFrame = *sp;
	cp += 2;

	m_cItemColor = *cp;
	cp++;

	m_cSpeed = *cp;
	cp ++;

	sp = (short*) cp;
	m_sItemSpecEffectValue1= *sp;
	cp +=2;

	sp = (short*) cp;
	m_sItemSpecEffectValue2 = *sp;
	cp +=2;

	sp = (short*) cp;
	m_sItemEffectValue1 = *sp;
	cp +=2;

	sp = (short*) cp;
	m_sItemEffectValue2 = *sp;
	cp +=2;

	sp = (short*) cp;
	m_sItemEffectValue3 = *sp;
	cp +=2;

	sp = (short*) cp;
	m_sItemEffectValue4 = *sp;
	cp +=2;

	sp = (short*) cp;
	m_sItemEffectValue5 = *sp;
	cp +=2;

	sp = (short*) cp;
	m_sItemEffectValue6 = *sp;
	cp +=2;

	dwp = (DWORD *)cp;
	m_dwAttribute = *dwp;
	cp += 4;

	return (int)(cp - pData);
}
