//mahui01@wind-mobi.com 2018/9/17 add for lenovo CSDK begin


#include <linux/types.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,18)
#include <linux/config.h>
#endif	
#if defined(CONFIG_SMP) && ! defined(__SMP__)
#define __SMP__
#endif	
#if defined(CONFIG_MODVERSIONS) && ! defined(MODVERSIONS)
#define MODVERSIONS
#endif	

#ifdef MODVERSIONS
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
#include <linux/modversions.h>
#else
#include <config/modversions.h>
#endif	
#endif	


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/skbuff.h>
#include <linux/jiffies.h>
#include <linux/fs.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/netdevice.h>
#include <linux/udp.h>
#include <net/sock.h>
#include <net/netlink.h>
#include <net/ip.h>

#ifdef __BTREE1__
#include "btreepriv1.h"
#endif

#ifdef __BTREE2__
#include "btree2.h"
#endif

#include "wjfw.h"

#define	SPIN_LOCK

#define MY_PROT_IP		0x0008		
#define BROADCAST_MASK	0xFF000000	
#define LOOP_MASK		0x0000007F	
#define INVALID_POS		-1			
#define UDP_HEADER_LEN	8			

#ifdef SPIN_LOCK
#define LOCK_IP_TREE		spin_lock_bh(&g_ip_tree_lock)
#define UNLOCK_IP_TREE		spin_unlock_bh(&g_ip_tree_lock)
#define LOCK_UID_TREE		spin_lock_bh(&g_uid_tree_lock)
#define UNLOCK_UID_TREE		spin_unlock_bh(&g_uid_tree_lock)
#define LOCK_HOST_TREE		spin_lock_bh(&g_host_tree_lock)
#define UNLOCK_HOST_TREE	spin_unlock_bh(&g_host_tree_lock)
#define LOCK_DCACHE_TREE	spin_lock_bh(&g_dcache_tree_lock)
#define UNLOCK_DCACHE_TREE	spin_unlock_bh(&g_dcache_tree_lock)
#define LOCK_DNSIP_TREE		spin_lock_bh(&g_dnsip_tree_lock)
#define UNLOCK_DNSIP_TREE	spin_unlock_bh(&g_dnsip_tree_lock)
#else
#define LOCK_IP_TREE		spin_lock_irq(&g_ip_tree_lock)
#define UNLOCK_IP_TREE		spin_unlock_irq(&g_ip_tree_lock)
#define LOCK_UID_TREE		spin_lock_irq(&g_uid_tree_lock)
#define UNLOCK_UID_TREE		spin_unlock_irq(&g_uid_tree_lock)
#define LOCK_HOST_TREE		spin_lock_irq(&g_host_tree_lock)
#define UNLOCK_HOST_TREE	spin_unlock_irq(&g_host_tree_lock)
#define LOCK_DCACHE_TREE	spin_lock_bh(&g_dcache_tree_lock)
#define UNLOCK_DCACHE_TREE	spin_unlock_bh(&g_dcache_tree_lock)
#define LOCK_DNSIP_TREE		spin_lock_bh(&g_dnsip_tree_lock)
#define UNLOCK_DNSIP_TREE	spin_unlock_bh(&g_dnsip_tree_lock)
#endif	

typedef struct
{
	SHost Host;		
	BTREE *pTree;
} SDnsCache;

typedef struct
{
	SHost Host;		
	int nCount;
	SIpAddr Items[MAX_ITEM_COUNT];
} SDnsCacheItem;

static struct nf_hook_ops nfho_in;	
static struct nf_hook_ops nfho_out;	

#ifdef __BTREE1__
static struct btree *g_ip_tree = NULL;		
#endif

#ifdef __BTREE2__
static BTREE *g_ip_tree = NULL;		
static BTREE *g_uid_tree = NULL;	
static BTREE *g_host_tree = NULL;	
static BTREE *g_dcache_tree = NULL;	
#ifdef __ALLOW_DIRECT_IP__
static BTREE *g_dnsip_tree = NULL;	
#endif 
#endif

#ifdef SPIN_LOCK
static spinlock_t g_ip_tree_lock;		
static spinlock_t g_uid_tree_lock;		
static spinlock_t g_host_tree_lock;		
static spinlock_t g_dcache_tree_lock;	
#ifdef __ALLOW_DIRECT_IP__
static spinlock_t g_dnsip_tree_lock;	
#endif 
#else
static struct mutex g_ip_tree_lock;
static struct mutex g_uid_tree_lock;
static struct mutex g_host_tree_lock;
static struct mutex g_dcache_tree_lock;
#ifdef __ALLOW_DIRECT_IP__
static struct mutex g_dnsip_tree_lock;
#endif 
#endif	
static SFilterPort g_FilterPort;	
static FLAG_FILTER g_DefaultFilter = FLAG_FILTER_BLACK;		


struct netlink_kernel_cfg nl_cfg;
static struct sock *g_socket = NULL;
static int g_nPid = 0;		
static int g_nEnabled = 0;	
static int g_nInUse = 0;

#ifdef __MY_THREAD__
struct task_struct *g_pThdTask = NULL;
#endif	


static void init_data(void)
{
#ifdef __BTREE1__
	g_ip_tree = bt_create(IP_Cmp, FreeKey, 128);	
#endif	

#ifdef __BTREE2__
	BTREE_ORDER = 15;
	BTREE_ORDER_HALF = BTREE_ORDER >> 1;
	g_ip_tree = btree_new();
	g_uid_tree = btree_new();
	g_host_tree = btree_new();
#endif	

	g_FilterPort.wlCnt = 0;
	g_FilterPort.blCnt = 0;
}

static void release_data(void)
{
	g_DefaultFilter = FLAG_FILTER_BLACK;
	g_nEnabled = 0;
	g_FilterPort.wlCnt = 0;
	g_FilterPort.blCnt = 0;

#ifdef __BTREE1__
	bt_free(g_ip_tree);
	g_ip_tree = NULL;
#endif	

#ifdef __BTREE2__
	btree_free(g_ip_tree);
	g_ip_tree = NULL;
	btree_free(g_uid_tree);
	g_uid_tree = NULL;
	btree_free(g_host_tree);
	g_host_tree = NULL;
#endif	
}

inline void SleepJiffies(int nJiffies)
{

	do {
		while (nJiffies > 0)
		{
			nJiffies = schedule_timeout(nJiffies);
		}
	} while(0);
}

typedef struct
{
	SIpAddr		IpBegin;	
	SIpAddr		IpEnd;		
	FLAG_FILTER	FlagFilter;	
	SFilterPort	FilterPort;	
	unsigned int uUid;		
	int		nTcpReqPassCnt;	
} SIpNode;

typedef SFilterUid SUidNode;



inline bool String2FilterPort(const char* sStr, bool bBL, SFilterPort *pFilterPort)
{
	bool bRet = false;
	const char *p = sStr;
	SPort Port;
	bool bPhase2 = false;

	if (bBL)
	{
		pFilterPort->blCnt = 0;
	}
	else
	{
		pFilterPort->wlCnt = 0;
	}
	Port.Begin = 0;
	Port.End = 0;
	while (true)
	{
		switch (*p)
		{
			case ',':
			case 0:
				if (Port.Begin != 0)
				{
					if (!bPhase2)
					{
						Port.End = Port.Begin;
					}
					if (bBL)
					{
						if (pFilterPort->blCnt >= MAX_ITEM_COUNT)
						{
							return true;
						}
						MEMCPY(&pFilterPort->BL[pFilterPort->blCnt], &Port, sizeof(Port));
						pFilterPort->blCnt ++;
					}
					else
					{
						if (pFilterPort->wlCnt >= MAX_ITEM_COUNT)
						{
							return true;
						}
						MEMCPY(&pFilterPort->WL[pFilterPort->wlCnt], &Port, sizeof(Port));
						pFilterPort->wlCnt ++;
					}
					Port.Begin = 0;
					Port.End = 0;
					bRet = true;
					bPhase2 = false;
				}
				break;
			case '-':
				bPhase2 = true;
				break;
			default:
				if (bPhase2)
				{
					Port.End = (Port.End * 10) + (*p - 0x30);
				}
				else
				{
					Port.Begin = (Port.Begin * 10) + (*p - 0x30);
				}
				break;
		}
		if (*p)
		{
			p ++;
		}
		else
		{
			break;
		}
	}

	return bRet;
}

inline FLAG_FILTER FindPort(const SFilterPort *pFilterPort, int nPort)
{
	int i;

	for (i = 0; i < pFilterPort->blCnt; ++ i)
	{
		if ((pFilterPort->BL[i].Begin <= nPort) && (pFilterPort->BL[i].End >= nPort))
		{
			return FLAG_FILTER_BLACK;
		}
	}

	for (i = 0; i < pFilterPort->wlCnt; ++ i)
	{
		if ((pFilterPort->WL[i].Begin <= nPort) && (pFilterPort->WL[i].End >= nPort))
		{
			return FLAG_FILTER_WHITE;
		}
	}

	return FLAG_FILTER_DEFAULT;	
}

static int Ip_Cmp(void *a, void *b)
{
	SIpNode *pNodeX = (SIpNode *) a;
	SIpNode *pNodeY = (SIpNode *) b;
	
        PRINTK(KERN_ALERT "Ip_Cmp, %08X %08X %08X %08X\n", pNodeX->IpBegin.uIpV4, pNodeX->IpEnd.uIpV4, pNodeY->IpBegin.uIpV4, pNodeY->IpEnd.uIpV4);
	if ((pNodeX->IpBegin.uIpV4 >= pNodeY->IpBegin.uIpV4) && (pNodeX->IpEnd.uIpV4 <= pNodeY->IpEnd.uIpV4))	
		return 0;
	if ((pNodeX->IpBegin.uIpV4 <= pNodeY->IpBegin.uIpV4) && (pNodeX->IpEnd.uIpV4 >= pNodeY->IpEnd.uIpV4))	
		return 0;
	if (pNodeX->IpBegin.uIpV4 < pNodeY->IpBegin.uIpV4)
		return -1;
	
		return 1;
}

static int Uid_Cmp(void *a, void *b)
{
	SUidNode *pNodeX = (SUidNode *) a;
	SUidNode *pNodeY = (SUidNode *) b;
	
	if (pNodeX->uUid < pNodeY->uUid)
		return -1;
	if (pNodeX->uUid == pNodeY->uUid)
		return 0;
	
		return 1;
}

static int FilterHost_Cmp(void *a, void *b)
{
	SFilterHost *pNodeX = (SFilterHost *)a;
	SFilterHost *pNodeY = (SFilterHost *)b;
	
	return Domain_Cmp(pNodeX->Host.sName, pNodeX->Host.nLen, pNodeY->Host.sName, pNodeY->Host.nLen);
}

static int DnsCache_Cmp(void *a, void *b)
{
	SDnsCache *pNodeX = (SDnsCache *)a;
	SDnsCache *pNodeY = (SDnsCache *)b;
	
	return Domain_Cmp(pNodeX->Host.sName, pNodeX->Host.nLen, pNodeY->Host.sName, pNodeY->Host.nLen);
}

static int DnsCacheItem_Cmp(void *a, void *b)
{
	SDnsCacheItem *pNodeX = (SDnsCacheItem *)a;
	SDnsCacheItem *pNodeY = (SDnsCacheItem *)b;
	
	return Domain_Cmp(pNodeX->Host.sName, pNodeX->Host.nLen, pNodeY->Host.sName, pNodeY->Host.nLen);
}

static int FilterHost_Cmp_A(void *a, void *b)
{
	SFilterHost *pNodeX = (SFilterHost *)a;
	SFilterHost *pNodeY = (SFilterHost *)b;
	int nLen1 = pNodeX->Host.nLen;
	int nLen2 = pNodeY->Host.nLen;
	const char *sStr1 = pNodeX->Host.sName;
	const char *sStr2 = pNodeY->Host.sName;
	
	-- nLen1;
	-- nLen2;
	while ((nLen1 >= 0) && (nLen2 >= 0))
	{
		if (*(sStr1 + nLen1) == *(sStr2 + nLen2))
		{
			-- nLen1;
			-- nLen2;
		}
		else if (*(sStr1 + nLen1) > *(sStr2 + nLen2))
		{
			return 1;
		}
		else
		{
			return -1;
		}
	}

	if (nLen1 < 0)
	{
		if (nLen2 < 0)	
		{
			return 0;
		}
		else	
		{
			return -1;
		}
	}
	else	
	{
		return 1;
	}
}

#ifdef __ALLOW_DIRECT_IP__
static int DnsIp_Cmp(void *a, void *b)
{
	TYPE_IP_V4 *pX = (TYPE_IP_V4 *) a;
	TYPE_IP_V4 *pY = (TYPE_IP_V4 *) b;
	
	if ((*pX) < (*pY))
		return -1;
	if ((*pX) > (*pY))
		return 1;

	return 0;
}
#endif 

#ifdef __BTREE1__
void FreeKey(void *pKey)
{
	if (pKey)
		FREE(pKey);
}
#endif	

#ifdef __BTREE2__
inline void *bt_insert(PROC_CMP pCmp, BTREE **btr, void *pNode)
{
	BTREE_POS pos;

	BTREE *p = btree_add(pCmp, *btr, pNode, &pos);	
	if (p)
	{
		if ((*btr)->parent != NULL)
			*btr = (*btr)->parent;

		return pNode;
	}

	return NULL;
}

inline void *bt_find(PROC_CMP pCmp, BTREE *btr, void *pNode)
{
	BTREE_POS pos;

	BTREE *p = btree_find(pCmp, btr, pNode, &pos);
	if (p && (pos != INVALID_POS))
	{
		return p->key[pos];
	}

	return NULL;
}
#endif	

inline SIpNode *AddNewIpNodeToTree(__u32 uIpBegin, __u32 uIpEnd, int nMemFlag)
{
	SIpNode *pNode = MALLOC(sizeof(SIpNode), nMemFlag);

	if (pNode)
	{
		pNode->IpBegin.uIpV4 = uIpBegin;
		pNode->IpEnd.uIpV4 = uIpEnd;
		if (!bt_insert(Ip_Cmp, &g_ip_tree, pNode))
		{
			FREE(pNode);
			pNode = NULL;
		}
	}

	return pNode;
}

inline SIpNode *AddNewIpFilter(__u32 uIpBegin, __u32 uIpEnd, FLAG_FILTER FlagFilter, const SFilterPort *pFP, bool bModifyOnly, int nMemFlag)
{
	SIpNode *pNode;
	SIpNode tmpNode;

	tmpNode.IpBegin.uIpV4 = uIpBegin;
	tmpNode.IpEnd.uIpV4 = uIpEnd;
	
	LOCK_IP_TREE;
	pNode = bt_find(Ip_Cmp, g_ip_tree, &tmpNode);
	if ((!pNode) && (!bModifyOnly))
	{
		pNode = AddNewIpNodeToTree(tmpNode.IpBegin.uIpV4, tmpNode.IpEnd.uIpV4, nMemFlag);
	}
	if (pNode)
	{
		pNode->FlagFilter = FlagFilter;
		if (pFP)
		{
			if (pFP->blCnt > 0)
			{
				pNode->FilterPort.blCnt = pFP->blCnt;
				MEMCPY(&pNode->FilterPort.BL[0], &pFP->BL[0], sizeof(SPort) * pFP->blCnt);
			}
			if (pFP->wlCnt > 0)
			{
				pNode->FilterPort.wlCnt = pFP->wlCnt;
				MEMCPY(&pNode->FilterPort.WL[0], &pFP->WL[0], sizeof(SPort) * pFP->wlCnt);
			}
		}
	}
	else if (!bModifyOnly)
	{
		printk(KERN_ERR "AddNewIpNodeToTree failed, ip is 0x%x\n", (int)tmpNode.IpBegin.uIpV4);
	}
	UNLOCK_IP_TREE;
	PRINTK(KERN_ALERT "AddNewIpFilter, 0x%X, 0x%x, %s\n", uIpBegin, uIpEnd, FlagFilter == FLAG_FILTER_WHITE ? "white":
									(FlagFilter == FLAG_FILTER_BLACK ? "Black" : "Default"));

	return pNode;
}

void FreeDCacheTree(BTREE *pTree)
{
	BTREE_POS i;

	if ((pTree == NULL) || (pTree->ptr == NULL))
		return;
	
	for (i = 0; i < pTree->count; ++ i)
	{
		if (pTree->key && BTREE_POS_STORED(pTree, i) && (pTree->key[i]))
		{
			SDnsCache *pDns = (SDnsCache *)pTree->key[i];
			btree_free(pDns->pTree);
			pDns->pTree = NULL;
			FREE(pTree->key[i]);
			pTree->key[i] = NULL;
		}
		if (pTree->ptr[i])
		{
			FreeDCacheTree(pTree->ptr[i]);
			pTree->ptr[i] = NULL;
		}
	}
	if (pTree->ptr[i])
	{
		FreeDCacheTree(pTree->ptr[i]);
		pTree->ptr[i] = NULL;
	}

	if (pTree->key)
	{
		FREE(pTree->key);
		pTree->key = NULL;
	}
	FREE(pTree->ptr);
	pTree->ptr = NULL;
	FREE(pTree);
}

inline void AddDnsCache(SDnsResp *pDnsResp)
{
	SDnsCache *pDnsTmp = MALLOC(sizeof(SDnsCache), ALLOCMEM_HIGH_FLAG);
	SDnsCache *pDns = NULL;
	SDnsCacheItem *pItemTmp = NULL;
	SDnsCacheItem *pItem = NULL;

	if (!pDnsTmp)
	{
		printk(KERN_ERR "AddDnsCache: MALLOC SDnsCache failed!");

		return;
	}
	pItemTmp = MALLOC(sizeof(SDnsCacheItem), ALLOCMEM_HIGH_FLAG);
	if (!pItemTmp)
	{
		printk(KERN_ERR "AddDnsCache: MALLOC SDnsCacheItem failed!");
		FREE(pDnsTmp);

		return;
	}

	if (pDnsResp->Host.sName[0] == '*')	
	{
		MEMCPY(pDnsTmp->Host.sName, pDnsResp->Host.sName, pDnsResp->Host.nLen);
		pDnsTmp->Host.nLen = pDnsResp->Host.nLen;
		pDnsTmp->Host.sName[pDnsResp->Host.nLen] = 0;
		PRINTK(KERN_ALERT "Add Dns Cache(*): %s, nDotCnt is %d\n", pDnsTmp->Host.sName, pDnsResp->nDotCnt);
	}
	else 
	{
		if (pDnsResp->nDotCnt < 2)	
		{
			pDnsTmp->Host.sName[0] = '*';
			pDnsTmp->Host.sName[1] = '.';
			MEMCPY(pDnsTmp->Host.sName + 2, pDnsResp->Host.sName, pDnsResp->Host.nLen);
			pDnsTmp->Host.nLen = 2 + pDnsResp->Host.nLen;
		}
		else
		{
			int nCnt = 0;
			int nDotCnt = 0;
			int nReqDotCnt = 1;
			if (pDnsResp->nDotCnt > 3)
			{
				nReqDotCnt = pDnsResp->nDotCnt - 2;
			}
			while (nCnt < pDnsResp->Host.nLen)
			{
				if (pDnsResp->Host.sName[nCnt] == '.')
					++ nDotCnt;
				if (nDotCnt != nReqDotCnt)
				{
					++ nCnt;
				}
				else
				{
					break;
				}
			}
			if (nCnt >= pDnsResp->Host.nLen)	
			{
				FREE(pDnsTmp);
				FREE(pItemTmp);

				return;
			}
			pDnsTmp->Host.sName[0] = '*';
			pDnsTmp->Host.nLen = pDnsResp->Host.nLen - nCnt;
			MEMCPY(pDnsTmp->Host.sName + 1, pDnsResp->Host.sName + nCnt, pDnsTmp->Host.nLen);
			++ pDnsTmp->Host.nLen;	
		}
		pDnsTmp->Host.sName[pDnsTmp->Host.nLen] = 0;
		PRINTK(KERN_ALERT "Add Dns Cache: %s, nDotCnt is %d\n", pDnsTmp->Host.sName, pDnsResp->nDotCnt);
	}
	MEMCPY(pItemTmp->Host.sName, pDnsResp->OriHost.sName, pDnsResp->OriHost.nLen);
	pItemTmp->Host.nLen = pDnsResp->OriHost.nLen;
	pItemTmp->Host.sName[pItemTmp->Host.nLen] = 0;
	LOCK_DCACHE_TREE;
	pDns = bt_find(DnsCache_Cmp, g_dcache_tree, pDnsTmp);
	if (pDns)	
	{
		FREE(pDnsTmp);
		pItem = bt_find(DnsCacheItem_Cmp, pDns->pTree, pItemTmp);
		if (pItem)	
		{
			FREE(pItemTmp);
			MEMCPY(pItem->Items, pDnsResp->Items, sizeof(pItem->Items[0]) * pDnsResp->nCount);
			pItem->nCount = pDnsResp->nCount;
		}
		else	
		{
			MEMCPY(pItemTmp->Items, pDnsResp->Items, sizeof(pItemTmp->Items[0]) * pDnsResp->nCount);
			pItemTmp->nCount = pDnsResp->nCount;
			if (!bt_insert(DnsCacheItem_Cmp, &pDns->pTree, pItemTmp))
			{
				printk(KERN_ERR "bt_insert failed, for host name: %s\n", pItemTmp->Host.sName);
				FREE(pItemTmp);
			}
		}
	}
	else	
	{
		if (!bt_insert(DnsCache_Cmp, &g_dcache_tree, pDnsTmp))
		{
			printk(KERN_ERR "bt_insert failed, for host name: %s\n", pDnsTmp->Host.sName);
			FREE(pDnsTmp);
			FREE(pItemTmp);
		}
		else
		{
			pDnsTmp->pTree = btree_new();
			MEMCPY(pItemTmp->Items, pDnsResp->Items, sizeof(pItemTmp->Items[0]) * pDnsResp->nCount);
			pItemTmp->nCount = pDnsResp->nCount;
			if (!bt_insert(DnsCacheItem_Cmp, &pDnsTmp->pTree, pItemTmp))
			{
				printk(KERN_ERR "bt_insert failed, for host name: %s\n", pItemTmp->Host.sName);
				FREE(pItemTmp);
			}
		}
	}
	UNLOCK_DCACHE_TREE;
}

inline int SendNLMsg(int nMsgType, char *pMsg, int nLen, int nMemFlag)
{
	struct sk_buff *skb_1;
	struct nlmsghdr *nlh;
	int len = NLMSG_SPACE(nLen);
	
	if ((!pMsg) || (!g_socket) || (!g_nPid))
	{
		printk(KERN_ERR "SendNLMsg: invalid parameter!\n");
		return -1;
	}
	skb_1 = alloc_skb(len, nMemFlag);

	if (!skb_1)
	{
		printk(KERN_ERR "SendNLMsg:alloc_skb_1 error\n");
		return -2;
	}
	nlh = nlmsg_put(skb_1, 0, 0, nMsgType, nLen, 0);

	NETLINK_CB(skb_1).creds.pid = 0;
	NETLINK_CB(skb_1).dst_group = 0;

	MEMCPY(NLMSG_DATA(nlh), pMsg, nLen);

	len = netlink_unicast(g_socket, skb_1, g_nPid, MSG_DONTWAIT);
	if (len <= 0)
	{
		printk(KERN_ERR "netlink_unicast failed on SendNLMsg! Return value is %d\n", len);
	}

	return len;
}

void AddAllIpFilter(BTREE *pNode, const SHost *pHost, FLAG_FILTER FlagFilter, const SFilterPort *pFP, SDnsResp2 *pDnsResp)
{
	BTREE_POS i;

	if ((pNode == NULL) || (pNode->ptr == NULL))
		return;
	
	for (i = 0; i < pNode->count; ++ i)
	{
		if (pNode->key && BTREE_POS_STORED(pNode, i) && (pNode->key[i]))
		{
			int k;
			SDnsCacheItem *pItem = (SDnsCacheItem *)pNode->key[i];

			if (Domain_Cmp(pItem->Host.sName, pItem->Host.nLen, pHost->sName, pHost->nLen) == 0)
			{
				for (k = 0; k < pItem->nCount; ++ k)
				{
					if (((FlagFilter == FLAG_FILTER_WHITE) ||
						((FlagFilter == FLAG_FILTER_DEFAULT) && (g_DefaultFilter == FLAG_FILTER_WHITE)))
						&& (pDnsResp->nCount < MAX_ITEM_COUNT2))
					{
						pDnsResp->Items[pDnsResp->nCount].uIpV4 = pItem->Items[k].uIpV4;
						++ pDnsResp->nCount;
					}

					if ((FlagFilter == FLAG_FILTER_WHITE)	
						|| (FlagFilter == FLAG_FILTER_BLACK)
						|| (pFP->blCnt > 0)
						|| (pFP->wlCnt > 0))	
					{
						AddNewIpFilter(pItem->Items[k].uIpV4, pItem->Items[k].uIpV4, FlagFilter, pFP, false, ALLOCMEM_MIDD_FLAG | ALLOCMEM_ZERO_FLAG);
					}
					else	
					{
						AddNewIpFilter(pItem->Items[k].uIpV4, pItem->Items[k].uIpV4, FlagFilter, pFP, true, ALLOCMEM_MIDD_FLAG | ALLOCMEM_ZERO_FLAG);
					}
				}
			}
		}
		if (pNode->ptr[i])
		{
			AddAllIpFilter(pNode->ptr[i], pHost, FlagFilter, pFP, pDnsResp);
		}
	}
	if (pNode->ptr[i])
	{
		AddAllIpFilter(pNode->ptr[i], pHost, FlagFilter, pFP, pDnsResp);
	}
}

inline void HandleDnsCache(SFilterHost *pFH)
{
	SDnsCache DnsTmp;
	SDnsCache *pDns = NULL;

	MEMCPY(DnsTmp.Host.sName, pFH->Host.sName, pFH->Host.nLen);
	DnsTmp.Host.nLen = pFH->Host.nLen;
	DnsTmp.Host.sName[pFH->Host.nLen] = 0;
	LOCK_DCACHE_TREE;
	pDns = bt_find(DnsCache_Cmp, g_dcache_tree, &DnsTmp);
	if (pDns)
	{
		static SDnsResp2 DnsResp;

		MEMCPY(DnsResp.Host.sName, pFH->Host.sName, pFH->Host.nLen);
		DnsResp.Host.nLen = pFH->Host.nLen;
		DnsResp.Host.sName[pFH->Host.nLen] = 0;
		DnsResp.nCount = 0;

		if (pFH->Host.sName[0] == '*')	
		{
			PRINTK(KERN_ALERT "Handle Dns Cache(*): %s\n", pFH->Host.sName);
			AddAllIpFilter(pDns->pTree, &pFH->Host, pFH->FlagFilter, &pFH->FilterPort, &DnsResp);
		}
		else	
		{
			SDnsCacheItem ItemTmp;
			SDnsCacheItem *pItem = NULL;

			PRINTK(KERN_ALERT "Handle Dns Cache: %s\n", pFH->Host.sName);
			MEMCPY(ItemTmp.Host.sName, pFH->Host.sName, pFH->Host.nLen);
			ItemTmp.Host.nLen = pFH->Host.nLen;
			ItemTmp.Host.sName[pFH->Host.nLen] = 0;
			pItem = bt_find(DnsCacheItem_Cmp, pDns->pTree, &ItemTmp);
			if (pItem)	
			{
				int i;

				for (i = 0; i < pItem->nCount; ++ i)
				{
					if (((pFH->FlagFilter == FLAG_FILTER_WHITE) ||
						((pFH->FlagFilter == FLAG_FILTER_DEFAULT) && (g_DefaultFilter == FLAG_FILTER_WHITE)))
						&& (DnsResp.nCount < MAX_ITEM_COUNT2))
					{
						DnsResp.Items[DnsResp.nCount].uIpV4 = pItem->Items[i].uIpV4;
						++ DnsResp.nCount;
					}

					if ((pFH->FlagFilter == FLAG_FILTER_WHITE)	
						|| (pFH->FlagFilter == FLAG_FILTER_BLACK)
						|| (pFH->FilterPort.wlCnt > 0)
						|| (pFH->FilterPort.blCnt > 0))	
					{
						AddNewIpFilter(pItem->Items[i].uIpV4, pItem->Items[i].uIpV4, pFH->FlagFilter, &pFH->FilterPort, false, ALLOCMEM_MIDD_FLAG | ALLOCMEM_ZERO_FLAG);
					}					
					else	
					{
						AddNewIpFilter(pItem->Items[i].uIpV4, pItem->Items[i].uIpV4, pFH->FlagFilter, &pFH->FilterPort, true, ALLOCMEM_MIDD_FLAG | ALLOCMEM_ZERO_FLAG);
					}
				}
			}
		}

	}
	UNLOCK_DCACHE_TREE;
}

#ifdef __DROP_ONDNSREQ__
inline FLAG_FILTER HandleDnsReq(const unsigned char *pData, int nLen)
{
	int i;
	int nReqCnt;
	FLAG_FILTER nRet = FLAG_FILTER_DEFAULT;
	const unsigned char *pEnd = pData + nLen;

	if (nLen < 12)	
	{
		return nRet;
	}

	pData += 2;		
	if (*pData & 0x80)	
	{
		return nRet;
	}
	PRINTK(KERN_ALERT "Enter HandleDnsReq\n");	
	pData += 2;	
	nReqCnt = (*pData << 8) | *(pData + 1);	
	if (nReqCnt > 0)	
	{
		pData += 8;	
		for (i = 0; i < nReqCnt; ++ i)
		{
			SFilterHost fh;
			int nCnt = 0;
			short nType;

			fh.Host.nLen = 0;
			while (*pData && (*pData < 192) && (pData < pEnd))
			{
				if (fh.Host.nLen == 0)
				{
					if (nCnt > 0)
					{
						*(fh.Host.sName + nCnt) = '.';
						++ nCnt;
					}
			
					MEMCPY(fh.Host.sName + nCnt, pData + 1, *pData);
					nCnt += *pData;
				}
			
				pData += (*pData + 1);
			}		
			*pData ? pData += 2 : ++ pData;

			nType = (*pData << 8) | *(pData + 1);	
			pData += 4;
			if (fh.Host.nLen == 0)
			{
				if ((nType == 0x01))	
				{
					SFilterHost *pNode;

					*(fh.Host.sName + nCnt) = 0;
					fh.Host.nLen = nCnt;
					PRINTK(KERN_ALERT "DNS Req, host: %s, Name len:%d\n", fh.Host.sName, fh.Host.nLen);

					LOCK_HOST_TREE;
					pNode = bt_find(FilterHost_Cmp, g_host_tree, &fh);
					if (pNode)      
					{
						nRet = pNode->FlagFilter;						
						UNLOCK_HOST_TREE;
						PRINTK(KERN_ALERT "DNS Req, host: %s, flag: %s\n", fh.Host.sName,
							(nRet == FLAG_FILTER_BLACK) ? "BLACK" : 
							((nRet == FLAG_FILTER_WHITE) ? "WHITE" : "DEFAULT"));

						return nRet;
					}
					else
					{
						UNLOCK_HOST_TREE;
					}
				}
			}
		}
	}
	PRINTK(KERN_ALERT ", set as default\n");	

	return nRet;
}
#endif	

inline void HandleDnsResp(const unsigned char *pData, int nLen, SDnsResp *pDnsResp, SFilterHost *pFH)
{
	int i;
	const unsigned char *pEnd = pData + nLen;
	int nReqCnt, nAnsCnt;	
	int nDotCnt = 0;

	pDnsResp->nCount = 0;
	pDnsResp->nDotCnt = 0;
	pFH->FlagFilter = FLAG_FILTER_DEFAULT;
	pFH->FilterPort.wlCnt = 0;
	pFH->FilterPort.blCnt = 0;
	if (nLen < 12)	
	{
		return;
	}

	pData += 2;		
	if (!(*pData & 0x80))	
	{
		return;
	}
	pData += 2;	
	nReqCnt = (*pData << 8) | *(pData + 1);	
	if (nReqCnt < 1)
	{
		return;
	}
	pData += 2;	
	nAnsCnt = (*pData << 8) | *(pData + 1);	
	if (nAnsCnt < 1)
	{
		return;
	}
	pData += 6;	

	pFH->Host.nLen = 0;
	nDotCnt = 0;
	for (i = 0; i < nReqCnt; ++ i)
	{
		int nCnt = 0;
		short nType;

		while (*pData && (*pData < 192) && (pData < pEnd))
		{
			if (pFH->Host.nLen == 0)
			{
				if (nCnt > 0)
				{
					if (nCnt >= (MAX_NAME_LENGTH - 1))
					{
						PRINTK(KERN_ALERT "Domain name is too long(1), give up\n");
						return;
					}
					*(pFH->Host.sName + nCnt) = '.';
					++ nDotCnt;
					++ nCnt;
				}
			
				if ((nCnt + *pData) >= MAX_NAME_LENGTH)
				{
					PRINTK(KERN_ALERT "Domain name is too long(2), give up\n");
					return;
				}
				MEMCPY(pFH->Host.sName + nCnt, pData + 1, *pData);
				nCnt += *pData;
			}
			
			pData += (*pData + 1);
		}		
		*pData ? pData += 2 : ++ pData;

		nType = (*pData << 8) | *(pData + 1);	
		pData += 4;
		if (pFH->Host.nLen == 0)
		{
			if ((nType == 0x01))	
			{
				*(pFH->Host.sName + nCnt) = 0;
				pFH->Host.nLen = nCnt;				
			}
		}
	}
	if (pFH->Host.nLen == 0)	
	{
		PRINTK(KERN_ALERT "DnsResp no query found\n");

		return;
	}
	else
	{
		SFilterHost *pNode;

		LOCK_HOST_TREE;
		pNode = bt_find(FilterHost_Cmp, g_host_tree, pFH);
		if (pNode)      
		{
			pFH->FlagFilter = pNode->FlagFilter;
			if (pNode->FilterPort.blCnt > 0)
			{
				pFH->FilterPort.blCnt = pNode->FilterPort.blCnt;
				MEMCPY(&pFH->FilterPort.BL[0], &pNode->FilterPort.BL[0], sizeof(SPort) * pNode->FilterPort.blCnt);
			}
			if (pNode->FilterPort.wlCnt > 0)
			{
				pFH->FilterPort.wlCnt = pNode->FilterPort.wlCnt;
				MEMCPY(&pFH->FilterPort.WL[0], &pNode->FilterPort.WL[0], sizeof(SPort) * pNode->FilterPort.wlCnt);
			}
			MEMCPY(pDnsResp->Host.sName, pNode->Host.sName, pNode->Host.nLen);
			pDnsResp->Host.nLen = pNode->Host.nLen;
			UNLOCK_HOST_TREE;
		}
		else
		{
			UNLOCK_HOST_TREE;
			MEMCPY(pDnsResp->Host.sName, pFH->Host.sName, pFH->Host.nLen);
			pDnsResp->Host.nLen = pFH->Host.nLen;
		}
		pDnsResp->Host.sName[pDnsResp->Host.nLen] = 0;
		MEMCPY(pDnsResp->OriHost.sName, pFH->Host.sName, pFH->Host.nLen);
		pDnsResp->OriHost.nLen = pFH->Host.nLen;
		pDnsResp->OriHost.sName[pFH->Host.nLen] = 0;
		pDnsResp->nDotCnt = nDotCnt;
	}
	PRINTK(KERN_ALERT "Query host resp: %s (%s), addr count : %d\n", pDnsResp->Host.sName, pDnsResp->OriHost.sName, nAnsCnt);
	
	for (i = 0; i < nAnsCnt; ++ i)
	{
		short nType,  nLength;

		while (*pData && (*pData < 192) && (pData < pEnd))	
		{
			pData += (*pData + 1);
		}
		*pData ? pData += 2 : ++ pData;
		if ((pData + 10) >= pEnd)	
		{
			return;
		}

		nType = (*pData << 8 ) | *(pData + 1);
		pData += 8;
		nLength = (*pData << 8 ) | *(pData + 1);
		pData += 2;
		PRINTK(KERN_ALERT "Type: %d, Length: %d, sizeof(TYPE_IP_V4): %lu\n", nType, nLength, sizeof(TYPE_IP_V4));
		switch (nType) 
		{
		case 1: 
			if (nLength == sizeof(TYPE_IP_V4))
			{
				MEMCPY(&pDnsResp->Items[pDnsResp->nCount].uIpV4, pData, sizeof(TYPE_IP_V4));
				pDnsResp->Items[pDnsResp->nCount].uIpV4 = ntohl(pDnsResp->Items[pDnsResp->nCount].uIpV4);
				pData += sizeof(TYPE_IP_V4);
				if ((pFH->FlagFilter == FLAG_FILTER_WHITE)	
					|| (pFH->FlagFilter == FLAG_FILTER_BLACK)
					|| (pFH->FilterPort.wlCnt > 0)
					|| (pFH->FilterPort.blCnt > 0))
				{
					AddNewIpFilter(pDnsResp->Items[pDnsResp->nCount].uIpV4, pDnsResp->Items[pDnsResp->nCount].uIpV4,
							pFH->FlagFilter, &pFH->FilterPort, false,	ALLOCMEM_HIGH_FLAG | ALLOCMEM_ZERO_FLAG);
				}
				else
				{
					AddNewIpFilter(pDnsResp->Items[pDnsResp->nCount].uIpV4, pDnsResp->Items[pDnsResp->nCount].uIpV4,
							pFH->FlagFilter, &pFH->FilterPort, true,	ALLOCMEM_HIGH_FLAG | ALLOCMEM_ZERO_FLAG);
				}
				PRINTK(KERN_ALERT "No.%d Ip addr is: 0x%08X\n", pDnsResp->nCount + 1, (unsigned int)pDnsResp->Items[pDnsResp->nCount].uIpV4);
#ifdef __ALLOW_DIRECT_IP__
				{
					TYPE_IP_V4 *pDnsIp = MALLOC(sizeof(TYPE_IP_V4), ALLOCMEM_HIGH_FLAG | ALLOCMEM_ZERO_FLAG);
					if (pDnsIp)
					{
						*pDnsIp = pDnsResp->Items[pDnsResp->nCount].uIpV4;
						LOCK_DNSIP_TREE;
						if (!bt_insert(DnsIp_Cmp, &g_dnsip_tree, pDnsIp))
						{
							FREE(pDnsIp);
						}
						UNLOCK_DNSIP_TREE;
					}
				}
#endif 
				++ pDnsResp->nCount;
				if (pDnsResp->nCount >= MAX_ITEM_COUNT)
				{
					return;
				}
			}
			else
			{
				PRINTK(KERN_ALERT "Not a ip v4 address\n");
				return;
			}
			break;
		default:
			if ((pData + nLength) < pEnd)
			{
				pData += nLength;
			}
			else
			{
				return;
			}
			break;
		}
	}
}

#ifdef __MY_THREAD__
int ThreadTask(void *pData)
{
	while (!kthread_should_stop())
	{

		
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout(HZ);
	}

	return 0;
}
#endif	

bool RedirectDns(struct sk_buff *skb, SFilterHost *pFH)
{
	struct iphdr *iph = NULL;	
	struct udphdr *uph = NULL;
	unsigned char *pDns = NULL;
	short nOriLen = 0;	
	short nNewLen = 42 + pFH->Host.nLen;	
	short nValue16 = htons(nNewLen);
	int nCnt = 0;
	unsigned char *pSrc, *pDest, *pEnd, *pDot;

	if (0 != skb_linearize(skb))
	{
		printk(KERN_ERR "RedirectDns: skb_linearize failed!\n");

		return false;
	}

	iph = ip_hdr(skb);	
	uph = (struct udphdr *)((char *)iph + sizeof(*iph));
	pDns = (char *)uph + sizeof(*uph);
	nOriLen = ntohs(uph->len);	

	if (nNewLen > nOriLen)
	{
		printk(KERN_ERR "RedirectDns: nNewLen is %d, nOriLen is %d!\n", nNewLen, nOriLen);
		
		return false;
	}


	uph->len = nValue16;

	*(pDns + 2) = 0x81;		
	*(pDns + 3) = 0x80;		
	*(pDns + 4) = 0x00;		
	*(pDns + 5) = 0x01;		
	*(pDns + 6) = 0x00;		
	*(pDns + 7) = 0x01;		
	*(pDns + 8) = 0x00;		
	*(pDns + 9) = 0x00;		
	*(pDns + 10) = 0x00;	
	*(pDns + 11) = 0x00;	

	pSrc = pFH->Host.sName;
	pEnd = pFH->Host.sName + pFH->Host.nLen;
	pDot = pDns + 12;
	pDest = pDns + 13;
	while (pSrc < pEnd)
	{
		if (*pSrc != '.')
		{
			*pDest = *pSrc;
			++ nCnt;
		}
		else
		{
			*pDot = nCnt;
			nCnt = 0;
			pDot = pDest;
		}
		++ pSrc;
		++ pDest;
	}
	*pDot = nCnt;
	*(pDns + 13 + pFH->Host.nLen) = 0;

	*(pDns + 14 + pFH->Host.nLen) = 0x00;	
	*(pDns + 15 + pFH->Host.nLen) = 0x01;	
	*(pDns + 16 + pFH->Host.nLen) = 0x00;	
	*(pDns + 17 + pFH->Host.nLen) = 0x01;	

	*(pDns + 18 + pFH->Host.nLen) = 0xC0;	
	*(pDns + 19 + pFH->Host.nLen) = 0x0C;	
	*(pDns + 20 + pFH->Host.nLen) = 0x00;	
	*(pDns + 21 + pFH->Host.nLen) = 0x01;	
	*(pDns + 22 + pFH->Host.nLen) = 0x00;	
	*(pDns + 23 + pFH->Host.nLen) = 0x01;	
	*(pDns + 24 + pFH->Host.nLen) = 0x00;	
	*(pDns + 25 + pFH->Host.nLen) = 0x00;	
	*(pDns + 26 + pFH->Host.nLen) = 0x00;	
	*(pDns + 27 + pFH->Host.nLen) = 0x05;	
	*(pDns + 28 + pFH->Host.nLen) = 0x00;	
	*(pDns + 29 + pFH->Host.nLen) = 0x04;	
	*(pDns + 30 + pFH->Host.nLen) = 0;		
	*(pDns + 31 + pFH->Host.nLen) = 0;		
	*(pDns + 32 + pFH->Host.nLen) = 0;		
	*(pDns + 33 + pFH->Host.nLen) = 0;		

	skb->len -= (nOriLen - nNewLen);
	skb->tail -= (nOriLen - nNewLen);
	iph->tot_len = htons(skb->len);

	uph->check = 0;
	skb->csum = csum_partial((char *)uph, nNewLen, 0);	
	uph->check = csum_tcpudp_magic(iph->saddr, iph->daddr, nNewLen, IPPROTO_UDP, skb->csum);	
	if (uph->check == 0)
		uph->check = -1;


	iph->check = 0;
	ip_send_check(iph);


	return true;
}

bool PakConfirm2Refuse(struct sk_buff *skb, struct iphdr *iph)
{
	int nTcpLen = 0;
	struct tcphdr *tph = (struct tcphdr *)((char *)iph + (iph->ihl << 2));


	tph->syn = 0;	
	tph->rst = 1;	
	tph->ack = 1;	


	nTcpLen = ntohs(iph->tot_len) - (iph->ihl << 2);
	tph->check = 0;
	skb->csum = csum_partial((char *)tph, nTcpLen, 0);	
	tph->check = csum_tcpudp_magic(iph->saddr, iph->daddr, nTcpLen, IPPROTO_TCP, skb->csum);	
	if (tph->check == 0)
		tph->check = 0xFFFF;

	iph->check = 0;
	ip_send_check(iph);



	return true;
}

void NL_receive_data(struct sk_buff *__skb)
{
	struct sk_buff *skb;
	struct nlmsghdr *nlh;
	skb = skb_get(__skb);
	nlh = nlmsg_hdr(skb);
	if ((skb->len == NLMSG_SPACE(0))	
		&& (nlh->nlmsg_type == 0))
	{
		g_nPid = nlh->nlmsg_pid;
		PRINTK(KERN_ALERT "User space process id received: %d\n", g_nPid);
	}
	else
	{
		const char *pData = NLMSG_DATA(nlh);

		switch (nlh->nlmsg_type)	
		{
		case PACKFILTER_IOCTL_SET_FILTERIP:
			{				
				long nValue = 0;
				long i;
				SFilterIp fi;

				MEMCPY(&nValue, pData, sizeof(nValue));	
				pData += sizeof(nValue);
				for (i = 0; i < nValue; ++ i)
				{
					MEMCPY(&fi, pData, sizeof(fi));
					pData += sizeof(fi);
					{
						AddNewIpFilter(fi.IpBegin.uIpV4, fi.IpEnd.uIpV4, fi.FlagFilter, &fi.FilterPort, false,
								ALLOCMEM_MIDD_FLAG | ALLOCMEM_ZERO_FLAG);
						PRINTK(KERN_ALERT "AddFilterIp, 0x%08X, %s\n", (unsigned int)fi.IpBegin.uIpV4, fi.FlagFilter == FLAG_FILTER_WHITE ? "white":
												(fi.FlagFilter == FLAG_FILTER_BLACK ? "Black" : "Default"));
					}
				}
			}
			break;
		case PACKFILTER_IOCTL_SET_FILTERHOST:
			{
				SFilterHost *pNode2 = NULL;
				SFilterHost *pNodeTmp = MALLOC(sizeof(SFilterHost), ALLOCMEM_MIDD_FLAG);

				if (pNodeTmp)
				{
					MEMCPY(pNodeTmp, pData, sizeof(*pNodeTmp));
					if ((pNodeTmp->Host.sName[0] == '*') && (pNodeTmp->Host.nLen == 1)) 
					{
						if ((pNodeTmp->FilterPort.blCnt <= 0) && (pNodeTmp->FilterPort.wlCnt <= 0))	
						{
							g_DefaultFilter = pNodeTmp->FlagFilter;
							PRINTK(KERN_ALERT "Set Default Filter, %s\n", g_DefaultFilter == FLAG_FILTER_WHITE ? "white":
											(g_DefaultFilter == FLAG_FILTER_BLACK ? "Black" : "Default"));
						}
						else
						{
							if (pNodeTmp->FilterPort.blCnt > 0)
							{
								g_FilterPort.blCnt = pNodeTmp->FilterPort.blCnt;
								MEMCPY(&g_FilterPort.BL[0], &pNodeTmp->FilterPort.BL[0], sizeof(SPort) * g_FilterPort.blCnt);
							}
							if (pNodeTmp->FilterPort.wlCnt > 0)
							{
								g_FilterPort.wlCnt = pNodeTmp->FilterPort.wlCnt;
								MEMCPY(&g_FilterPort.WL[0], &pNodeTmp->FilterPort.WL[0], sizeof(SPort) * g_FilterPort.wlCnt);
							}
							PRINTK(KERN_ALERT "Set Default Port Filter, BL: %d, WL: %d\n", g_FilterPort.blCnt, g_FilterPort.wlCnt);
						}
						FREE(pNodeTmp);
					}
					else
					{
						HandleDnsCache(pNodeTmp);	
						LOCK_HOST_TREE;
						pNode2 = bt_find(FilterHost_Cmp_A, g_host_tree, pNodeTmp);
						if (pNode2)	
						{
							PRINTK(KERN_ALERT "host name: %s 's rule changed\n", pNodeTmp->Host.sName);
							pNode2->FlagFilter = pNodeTmp->FlagFilter;
							if (pNodeTmp->FilterPort.blCnt > 0)
							{
								pNode2->FilterPort.blCnt = pNodeTmp->FilterPort.blCnt;
								MEMCPY(&pNode2->FilterPort.BL[0], &pNodeTmp->FilterPort.BL[0], sizeof(SPort) * pNodeTmp->FilterPort.blCnt);
							}
							if (pNodeTmp->FilterPort.wlCnt > 0)
							{
								pNode2->FilterPort.wlCnt = pNodeTmp->FilterPort.wlCnt;
								MEMCPY(&pNode2->FilterPort.WL[0], &pNodeTmp->FilterPort.WL[0], sizeof(SPort) * pNodeTmp->FilterPort.wlCnt);
							}
							FREE(pNodeTmp);
						}
						else	
						{
							if (!bt_insert(FilterHost_Cmp, &g_host_tree, pNodeTmp))
							{
								printk(KERN_ERR "bt_insert failed, for host name: %s\n", pNodeTmp->Host.sName);
								FREE(pNodeTmp);
							}
						}
						UNLOCK_HOST_TREE;
						PRINTK(KERN_ALERT "AddFilterHost, %s, %s\n", pNodeTmp->Host.sName, pNodeTmp->FlagFilter == FLAG_FILTER_WHITE ? "white":
												(pNodeTmp->FlagFilter == FLAG_FILTER_BLACK ? "Black" : "Default"));
					}
				}
				else
				{
					printk(KERN_ERR "Malloc memory failed for PACKFILTER_IOCTL_SET_FILTERHOST\n");
				}
			}
			break;
		case PACKFILTER_IOCTL_SET_DEFAULTFILTER:
			MEMCPY(&g_DefaultFilter, pData, sizeof(g_DefaultFilter));
			PRINTK(KERN_ALERT "Set Default Filter, %s\n", g_DefaultFilter == FLAG_FILTER_WHITE ? "white":
											(g_DefaultFilter == FLAG_FILTER_BLACK ? "Black" : "Default"));
			break;
		case PACKFILTER_IOCTL_SET_ENABLED:
			MEMCPY(&g_nEnabled, pData, sizeof(g_nEnabled));
			PRINTK(KERN_ALERT "Filter Enable: %s\n", g_nEnabled ? "on": "off");
			break;
		case PACKFILTER_IOCTL_SET_REINIT:
			LOCK_IP_TREE;
			LOCK_UID_TREE;
			LOCK_HOST_TREE;
			release_data();
			init_data();
			UNLOCK_IP_TREE;
			UNLOCK_UID_TREE;
			UNLOCK_HOST_TREE;
			PRINTK(KERN_ALERT "Filter Reinited\n");
			break;
		case PACKFILTER_IOCTL_SET_FILTERUID:
			{
				SUidNode *pNode2 = NULL;
				SUidNode *pNodeTmp = MALLOC(sizeof(SUidNode), ALLOCMEM_MIDD_FLAG | ALLOCMEM_ZERO_FLAG);
				g_nInUse = 1;

				if (pNodeTmp)
				{
					MEMCPY(pNodeTmp, pData, sizeof(*pNodeTmp));
					LOCK_UID_TREE;
					pNode2 = bt_find(Uid_Cmp, g_uid_tree, pNodeTmp);
					if (pNode2)	
					{
						PRINTK(KERN_ALERT "UID: %d 's rule changed\n", pNodeTmp->uUid);
						pNode2->FlagFilter = pNodeTmp->FlagFilter;
						FREE(pNodeTmp);
					}
					else	
					{
						if (!bt_insert(Uid_Cmp, &g_uid_tree, pNodeTmp))
						{
							printk(KERN_ERR "bt_insert failed, for user uid: %u\n", pNodeTmp->uUid);
							FREE(pNodeTmp);
						}
					}
					UNLOCK_UID_TREE;
					PRINTK(KERN_ALERT "AddFilterUid, %d, %s\n", pNodeTmp->uUid, pNodeTmp->FlagFilter == FLAG_FILTER_WHITE ? "white":
												(pNodeTmp->FlagFilter == FLAG_FILTER_BLACK ? "Black" : "Default"));
				}
				else
				{
					printk(KERN_ERR "Malloc memory failed for PACKFILTER_IOCTL_SET_FILTERUID\n");
				}
			}
			break;
		default:
			printk(KERN_ERR "WJFirewall: Unknown command type (0x%X)\n", nlh->nlmsg_type);
			break;
		}
	}
	kfree_skb(skb);
}

inline void AddNewToQueue(SIpAddr *pIpAddr)
{
	SendNLMsg(PACKFILTER_IOCTL_GET_NEWIP, (char *)pIpAddr, sizeof(SIpAddr), ALLOCMEM_HIGH_FLAG);
}

inline FLAG_FILTER ProcessFilter(struct sk_buff *skb, __kernel_uid32_t *pUid)
{
	FLAG_FILTER nUidFF = FLAG_FILTER_DEFAULT;
	*pUid = 0;

	if (skb->sk)
	{
		if (skb->sk->sk_socket)
		{
			if (skb->sk->sk_socket->file)
			{	
				if (skb->sk->sk_socket->file->f_cred)
				{			
					SUidNode *pUidNode;
					SUidNode tmpUidNode;
#ifdef __KERNEL_3_10__ 
					*pUid = skb->sk->sk_socket->file->f_cred->uid;
#else
					*pUid = skb->sk->sk_socket->file->f_cred->uid.val;
#endif	
					tmpUidNode.uUid = *pUid;
					PRINTK(KERN_ALERT "uid: %d, euid: %d, cred_uid: %d, len: %u, data_len: %u\n",
						skb->sk->sk_socket->file->f_owner.uid.val,
						skb->sk->sk_socket->file->f_owner.euid.val,
						skb->sk->sk_socket->file->f_cred->uid.val,
						skb->mac_len ? skb->len + skb->mac_len : skb->len + 14, skb->data_len);

					LOCK_UID_TREE;
					pUidNode = bt_find(Uid_Cmp, g_uid_tree, &tmpUidNode);
					if (pUidNode)      
					{
						nUidFF = pUidNode->FlagFilter;
						UNLOCK_UID_TREE;
						PRINTK(KERN_ALERT "ProcessFilter: uid %d found! Flag is %d\n", *pUid, nUidFF);
					}
					else if (*pUid < 1000)	
					{
						*pUid = 0;

						UNLOCK_UID_TREE;
						nUidFF = FLAG_FILTER_WHITE;
						PRINTK(KERN_ALERT "ProcessFilter: uid %d is for system! Flag is White\n", *pUid);
					}
					else
					{
						nUidFF = FLAG_FILTER_DEFAULT;
						UNLOCK_UID_TREE;
					}
				}
				else
				{
					PRINTK(KERN_ALERT "socket file cred is null\n");
				}
			}
			else
			{
				PRINTK(KERN_ALERT "socket file is null\n");
			}
			
		}
		else
		{
			PRINTK(KERN_ALERT "socket is null\n");
		}
	}

	return nUidFF;
}

#ifdef __KERNEL_3_10__ 
static unsigned int hook_func_in(unsigned int hooknum,
#else
static unsigned int hook_func_in(const struct nf_hook_ops *ops,
#endif 
				struct sk_buff *skb,
				const struct net_device *in,
				const struct net_device *out,
				int (*okfn)(struct sk_buff *))
{
	struct iphdr *iph = ip_hdr(skb);

	if ((skb->protocol != MY_PROT_IP)	
			|| ((iph->daddr & LOOP_MASK) == LOOP_MASK)	
			|| ((iph->daddr & BROADCAST_MASK) == BROADCAST_MASK))	
	{
		PRINTK(KERN_ALERT "in... packet from %08X to %08X, accept !!!\n", ntohl(iph->saddr), ntohl(iph->daddr));
		return NF_ACCEPT;
	}
	
	if (iph->protocol == IPPROTO_UDP)
	{
		unsigned char *pUdp = skb->data + (iph->ihl << 2);
		unsigned short uPort = *pUdp;

		uPort <<= 8;
		uPort += *(pUdp + 1);

		PRINTK(KERN_ALERT "IN UDP found! source port is %d\n", uPort);
		if (uPort == 53)	
		{
			short nLen = *(pUdp + 4);
			nLen <<= 8;
			nLen += *(pUdp + 5);
			if (nLen > UDP_HEADER_LEN)
			{
				SDnsResp DnsResp;
				static SFilterHost fh;					

				HandleDnsResp(pUdp + UDP_HEADER_LEN, nLen - UDP_HEADER_LEN, &DnsResp, &fh);
				if (DnsResp.nCount > 0)
				{

					AddDnsCache(&DnsResp);
				}
			}
		}
	}

	if (!g_nEnabled)	
	{
		return NF_ACCEPT;
	}
	else
	{
		int nDrop = 0;
		SIpNode *pNode = NULL;
		FLAG_FILTER nIpFF = FLAG_FILTER_DEFAULT;
		SIpNode tmpNode;

		PRINTK(KERN_ALERT "in... packet from %08X to %08X\n", ntohl(iph->saddr), ntohl(iph->daddr));

		LOCK_IP_TREE;
		if (iph->protocol == IPPROTO_TCP)	
		{
			unsigned char *pTcp = skb->data + (iph->ihl << 2);
			unsigned char uFlag = *(pTcp + 13) & 0x3F;	
			unsigned short uPort = *pTcp;
			FLAG_FILTER nUidFF = FLAG_FILTER_DEFAULT;

			uPort <<= 8;
			uPort += *(pTcp + 1);

			tmpNode.IpBegin.uIpV4 = ntohl(iph->saddr);
			tmpNode.IpEnd.uIpV4 = tmpNode.IpBegin.uIpV4;
			pNode = bt_find(Ip_Cmp, g_ip_tree, &tmpNode);
			if (pNode)	
			{
				FLAG_FILTER flg1;

				nIpFF = pNode->FlagFilter;
	                        PRINTK(KERN_ALERT "Found ip %08X", ntohl(iph->saddr));
				flg1 = FindPort(&pNode->FilterPort, uPort);
				if (flg1 != FLAG_FILTER_DEFAULT)
				{
					nIpFF = flg1;
				}
			}
			else
			{
				FLAG_FILTER flg2 = FindPort(&g_FilterPort, uPort);
				if (flg2 != FLAG_FILTER_DEFAULT)
				{
					nIpFF = flg2;
				}
			}
				
#ifdef __ALLOW_DIRECT_IP__
			if (nIpFF == FLAG_FILTER_DEFAULT)
			{
				TYPE_IP_V4 *pDnsIp = NULL;

				LOCK_DNSIP_TREE;
				pDnsIp = bt_find(DnsIp_Cmp, g_dnsip_tree, &tmpNode.IpAddr.uIpV4);
				UNLOCK_DNSIP_TREE;
				if (!pDnsIp)
				{
					nIpFF = FLAG_FILTER_WHITE;
				}
			}
#endif 


			if (uFlag == 0x12)	
			{
				if (pNode && pNode->nTcpReqPassCnt)	
				{
					if (pNode->nTcpReqPassCnt > 0)	
					{
						-- pNode->nTcpReqPassCnt;
						nUidFF = FLAG_FILTER_WHITE;
					}
					else if (pNode->nTcpReqPassCnt < 0)	
					{
						++ pNode->nTcpReqPassCnt;
						nUidFF = FLAG_FILTER_BLACK;
					}
					PRINTK(KERN_ALERT "In, Uid for TCP, IP: 0x%08X, nTcpReqPassCnt(after): %d\n", (unsigned int)pNode->IpBegin.uIpV4, pNode->nTcpReqPassCnt);
				}

				if ((nIpFF == FLAG_FILTER_BLACK) 
					|| (nUidFF == FLAG_FILTER_BLACK)	
					|| ((nIpFF == FLAG_FILTER_DEFAULT) && (nUidFF == FLAG_FILTER_DEFAULT) && (g_DefaultFilter == FLAG_FILTER_BLACK)))	
				{
					nDrop = 1;
				}
			}
		}
		UNLOCK_IP_TREE;

		if (nDrop)	
		{
			PakConfirm2Refuse(skb, iph);
			PRINTK(KERN_ALERT "in... packet from %08X to %08X, drop it\n", ntohl(iph->saddr), ntohl(iph->daddr));
		}
	}

	return NF_ACCEPT;
}

#ifdef __KERNEL_3_10__ 
static unsigned int hook_func_out(unsigned int hooknum,
#else
static unsigned int hook_func_out(const struct nf_hook_ops *ops,
#endif 
				struct sk_buff *skb,
				const struct net_device *in,
				const struct net_device *out,
				int (*okfn)(struct sk_buff *))
{
	const struct iphdr *iph = ip_hdr(skb);

	if ((skb->protocol != MY_PROT_IP)	
		|| ((iph->daddr & LOOP_MASK) == LOOP_MASK)	
		|| (!g_nEnabled)	
		|| ((iph->daddr & BROADCAST_MASK) == BROADCAST_MASK))	
	{
		PRINTK(KERN_ALERT "out... packet from %08X to %08X, accept!!!\n", ntohl(iph->saddr), ntohl(iph->daddr));
		return NF_ACCEPT;
	}
	else
	{
		unsigned int uRet = NF_ACCEPT;
		SIpNode *pNode = NULL;
		FLAG_FILTER nIpFF = FLAG_FILTER_DEFAULT;
		SIpNode tmpNode;
		__kernel_uid32_t uUid = 0;
	
		FLAG_FILTER nUidFF = ProcessFilter(skb, &uUid);

		PRINTK(KERN_ALERT "out... packet from %08X to %08X\n", ntohl(iph->saddr), ntohl(iph->daddr));

		LOCK_IP_TREE;
		if (iph->protocol == IPPROTO_TCP)
		{
			unsigned char *pTcp = skb->data + (iph->ihl << 2);
			unsigned short uPort = *(pTcp + 2);

			uPort <<= 8;
			uPort += *(pTcp + 3);

			tmpNode.IpBegin.uIpV4 = ntohl(iph->daddr);
			tmpNode.IpEnd.uIpV4 = tmpNode.IpBegin.uIpV4;
			pNode = bt_find(Ip_Cmp, g_ip_tree, &tmpNode);
			if (pNode)      
			{
				FLAG_FILTER flg1;

				nIpFF = pNode->FlagFilter;
	                        PRINTK(KERN_ALERT "Found ip %08X", ntohl(iph->daddr));
				if (uUid != 0)
				{
					pNode->uUid = uUid;
				}
				flg1 = FindPort(&pNode->FilterPort, uPort);
				if (flg1 != FLAG_FILTER_DEFAULT)
				{
					nIpFF = flg1;
				}
				PRINTK(KERN_ALERT "out... TCP packet from %08X to %08X, dest port %u, %s\n", ntohl(iph->saddr), ntohl(iph->daddr), uPort, pTcp + 60);
			}
			else
			{
				FLAG_FILTER flg2 = FindPort(&g_FilterPort, uPort);
				if (flg2 != FLAG_FILTER_DEFAULT)
				{
					nIpFF = flg2;
				}
			}
		}
			
#ifdef __DEBUG__
		if (iph->protocol == IPPROTO_TCP)
		{
			unsigned char *pTcp = skb->data + (iph->ihl << 2);
			unsigned short uPort = *(pTcp + 2);

			uPort <<= 8;
			uPort += *(pTcp + 3);
			PRINTK(KERN_ALERT "out... TCP packet from %08X to %08X, dest port %u, %s\n", ntohl(iph->saddr), ntohl(iph->daddr), uPort, pTcp + 60);
		}
#endif	

		if (iph->protocol == IPPROTO_UDP) 
		{
			unsigned char *pUdp = skb->data + (iph->ihl << 2);
			unsigned short uPort = *(pUdp + 2);

			uPort <<= 8;
			uPort += *(pUdp + 3);
			PRINTK(KERN_ALERT "out... UDP packet from %08X to %08X, dest port %u\n", ntohl(iph->saddr), ntohl(iph->daddr), uPort);
			if (uPort != 53)	
			{
				if ((nUidFF == FLAG_FILTER_BLACK) || (nIpFF == FLAG_FILTER_BLACK)
#ifndef __ALLOW_DIRECT_IP__
						|| ((nUidFF == FLAG_FILTER_DEFAULT) && (nIpFF == FLAG_FILTER_DEFAULT)
						&& (g_DefaultFilter == FLAG_FILTER_BLACK)) 
#endif	
					)
				{
					uRet = NF_DROP;
					PRINTK(KERN_ALERT "out... UDP packet from %08X to %08X, drop it\n", ntohl(iph->saddr), ntohl(iph->daddr));
				}
			}
#ifdef __DROP_ONDNSREQ__
			else
			{
				short nLen = *(pUdp + 4);
				nLen <<= 8;
				nLen += *(pUdp + 5);
				if (nLen > UDP_HEADER_LEN)
				{

					FLAG_FILTER nHostFF = HandleDnsReq(pUdp + UDP_HEADER_LEN, nLen - UDP_HEADER_LEN);
					PRINTK(KERN_ALERT "nIpFF:%d, nHostFF:%d, g_DefaultFilter:%d\n",
							nIpFF, nHostFF, g_DefaultFilter);
					if ((nUidFF == FLAG_FILTER_BLACK) || (nIpFF == FLAG_FILTER_BLACK)
							|| (nHostFF == FLAG_FILTER_BLACK)
							|| ((nUidFF == FLAG_FILTER_DEFAULT) 
							&& (nIpFF == FLAG_FILTER_DEFAULT) && (nHostFF == FLAG_FILTER_DEFAULT)
							&& (g_DefaultFilter == FLAG_FILTER_BLACK))) 
					{
						uRet = NF_DROP;
						PRINTK(KERN_ALERT "out... UDP packet(DNS Req) from %08X to %08X, drop it\n", ntohl(iph->saddr), ntohl(iph->daddr));
					}
				}
			}
#endif	
		}
		else
		{
#ifdef __DROP_ONSEND__
			if ((nIpFF == FLAG_FILTER_BLACK) || (nUidFF == FLAG_FILTER_BLACK)
#ifndef __ALLOW_DIRECT_IP__
				|| ((nIpFF == FLAG_FILTER_DEFAULT) && (nUidFF == FLAG_FILTER_DEFAULT)
				&& (g_DefaultFilter == FLAG_FILTER_BLACK)) 
#endif	
				)
			{
				uRet = NF_DROP;
				PRINTK(KERN_ALERT "out... TCP packet from %08X to %08X, drop it\n", ntohl(iph->saddr), ntohl(iph->daddr));
			}
#endif	
		}

		if ((uRet == NF_ACCEPT) && (iph->protocol == IPPROTO_TCP)
					&& (nIpFF == FLAG_FILTER_DEFAULT)	
					&& ((nUidFF == FLAG_FILTER_WHITE) || (nUidFF == FLAG_FILTER_BLACK)))	
		{
			if (!pNode) 
			{
				pNode = AddNewIpNodeToTree(tmpNode.IpBegin.uIpV4, tmpNode.IpEnd.uIpV4, ALLOCMEM_HIGH_FLAG | ALLOCMEM_ZERO_FLAG);
				if (pNode)
				{
					pNode->FlagFilter = FLAG_FILTER_DEFAULT;
					if (uUid != 0)
					{
						pNode->uUid = uUid;
					}
					PRINTK(KERN_ALERT "To IP not found! Add new one success! Ptr is 0x%p\n", pNode);				
				}
				else
				{
					printk(KERN_ERR "From IP not found! Add new one failed!\n");
				}
			}

			if (pNode)
			{
				unsigned char *pTcp = skb->data + (iph->ihl << 2);
				unsigned int nConfirmNo = 0;	
				unsigned char uFlag = *(pTcp + 13) & 0x3F;	

				MEMCPY(&nConfirmNo, pTcp + 8, sizeof(nConfirmNo));
				if ((nConfirmNo == 0) && (uFlag == 0x02))	
				{
					if (nUidFF == FLAG_FILTER_BLACK)	
					{
						if (pNode->nTcpReqPassCnt > 0)
						{
							pNode->nTcpReqPassCnt = -1;
						}
						else
						{
							-- pNode->nTcpReqPassCnt;
						}
					}
					else	
					{
						if (pNode->nTcpReqPassCnt < 0)
						{
							pNode->nTcpReqPassCnt = 1;
						}
						else
						{
							++ pNode->nTcpReqPassCnt;
						}
					}
					PRINTK(KERN_ALERT "Out, Uid for TCP, IP: 0x%08X, nTcpReqPassCnt: %d\n", (unsigned int)pNode->IpBegin.uIpV4, pNode->nTcpReqPassCnt);
				}
			}
		}
		UNLOCK_IP_TREE;

		return uRet;
	}
}

inline void ClearUp(void)
{
#ifdef __MY_THREAD__
	if (g_pThdTask)
	{
		kthread_stop(g_pThdTask);
		g_pThdTask = NULL;
	}
#endif	

	nf_unregister_hook(&nfho_in);
	nf_unregister_hook(&nfho_out);

	LOCK_IP_TREE;
	LOCK_UID_TREE;
	LOCK_HOST_TREE;
#ifdef __ALLOW_DIRECT_IP__
	LOCK_DNSIP_TREE;
#endif 
	release_data();
	g_nPid = 0;
#ifdef __ALLOW_DIRECT_IP__
	UNLOCK_DNSIP_TREE;
#endif 
	UNLOCK_IP_TREE;
	UNLOCK_UID_TREE;
	UNLOCK_HOST_TREE;

#ifdef __ALLOW_DIRECT_IP__
	LOCK_DNSIP_TREE;
	btree_free(g_dnsip_tree);
	g_dnsip_tree = NULL;
	UNLOCK_DNSIP_TREE;
#endif 

	LOCK_DCACHE_TREE;
	FreeDCacheTree(g_dcache_tree);
	g_dcache_tree = NULL;
	UNLOCK_DCACHE_TREE;

	if (g_socket != NULL)
	{
		sock_release(g_socket->sk_socket);
		g_socket = NULL;
	}
	g_nInUse = 0;
}


static int __init fi_init_module(void)
{
#ifdef __MY_THREAD__
	int err = 0;
#endif	
	
	g_nInUse = 0;
	g_nPid = 0;
	init_data();
	g_dcache_tree = btree_new();
#ifdef __ALLOW_DIRECT_IP__
	g_dnsip_tree = btree_new();
#endif 

#ifdef SPIN_LOCK
	spin_lock_init(&g_ip_tree_lock);
	spin_lock_init(&g_uid_tree_lock);
	spin_lock_init(&g_host_tree_lock);
	spin_lock_init(&g_dcache_tree_lock);
#ifdef __ALLOW_DIRECT_IP__
	spin_lock_init(&g_dnsip_tree_lock);
#endif 
#else
	mutex_init(&g_ip_tree_lock);
	mutex_init(&g_uid_tree_lock);
	mutex_init(&g_host_tree_lock);
	mutex_init(&g_dcache_tree_lock);
#ifdef __ALLOW_DIRECT_IP__
	mutex_init(&g_dnsip_tree_lock);
#endif 
#endif	


	
	nfho_in.hook     = hook_func_in;         
	nfho_in.hooknum  = NF_INET_LOCAL_IN;
	nfho_in.owner = THIS_MODULE;
	nfho_in.pf       = PF_INET;
	nfho_in.priority = NF_IP_PRI_FIRST;   

	nf_register_hook(&nfho_in);


	
	nfho_out.hook     = hook_func_out;         
	nfho_out.hooknum  = NF_INET_POST_ROUTING;
	nfho_out.owner = THIS_MODULE;
	nfho_out.pf       = PF_INET;
	nfho_out.priority = NF_IP_PRI_FIRST;   

	nf_register_hook(&nfho_out);

	nl_cfg.input = NL_receive_data;
	nl_cfg.groups = 1;
        nl_cfg.cb_mutex = NULL;

	g_socket = netlink_kernel_create(&init_net, NETLINK_PACKFILTER, &nl_cfg);
	if (!g_socket)
	{
		printk(KERN_ERR "netlink_kernel_create failed!\n");
	}

#ifdef __MY_THREAD__
	g_pThdTask = kthread_create(ThreadTask, NULL, "WJFirewallTask");
	if (IS_ERR(g_pThdTask))
	{
		err = PTR_ERR(g_pThdTask);
		printk(KERN_ERR "Unable to start kernel thread g_pThdTask, err is %d.\n", err);
		g_pThdTask = NULL;

		ClearUp();
		
		return err;
	}
	wake_up_process(g_pThdTask);
#endif	

	printk(KERN_ERR "WJFirewall install into kernel!\n");

	return 0;
}


static void __exit fi_cleanup_module(void)
{
	ClearUp();

	printk(KERN_ERR "WJFirewall removed from kernel!\n");
}


module_init(fi_init_module);
module_exit(fi_cleanup_module);

MODULE_AUTHOR("ivan liu <ljxp@263.net>");
MODULE_DESCRIPTION("High performance firewall for LENOVO customing");
MODULE_LICENSE(" ");

//mahui01@wind-mobi.com 2018/9/17 add for lenovo CSDK end