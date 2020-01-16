//mahui01@wind-mobi.com 2018/9/18 add for lenovo CSDK begin

#ifndef __PACKFILTER_h_Include__
#define __PACKFILTER_h_Include__


#define MAX_NAME_LENGTH			128					
#define MAX_ITEM_COUNT			28					
#define MAX_ITEM_COUNT2			152
#define MAX_CACHE_HOSTID_CNT	16					
#define DEV_PACKFILTER_NAME		"wjfirewall"		
#define DEV_PF_FULLNAME			"/dev/wjfirewall"	
#define NETLINK_PACKFILTER		29					
#define ALLOCMEM_HIGH_FLAG		GFP_ATOMIC			
#define ALLOCMEM_MIDD_FLAG		GFP_ATOMIC			
#define ALLOCMEM_ZERO_FLAG		__GFP_ZERO			

#ifdef __DEBUG__
#define PRINTK			printk
#else
#define PRINTK(...)
#endif	

#define MALLOC(size, flag)	kmalloc(size, flag)
#define FREE(p)				kfree(p)

typedef int32_t TYPE_IP_V4;
typedef char TYPE_NAME[MAX_NAME_LENGTH];
typedef int FLAG_FILTER;

#define FLAG_FILTER_DEFAULT	0x00	
#define	FLAG_FILTER_WHITE	0x01	
#define FLAG_FILTER_BLACK	0x02	


typedef int IPADDR_TYPE;

#define IPADDR_TYPE_V4	0x00	
#define IPADDR_TYPE_V6	0x01	

typedef struct
{

	union
	{
		TYPE_IP_V4 uIpV4;	
	};
} SIpAddr;

typedef struct
{
	int nLen;		
	TYPE_NAME sName;	
} SHost;

typedef struct
{
	int	Begin;
	int	End;
} SPort;

typedef struct
{
	SPort	BL[MAX_ITEM_COUNT];	
	int	blCnt;	
	SPort	WL[MAX_ITEM_COUNT];	
	int	wlCnt;	
} SFilterPort;

typedef struct
{
	SIpAddr		IpBegin;	
        SIpAddr		IpEnd;		
	FLAG_FILTER	FlagFilter;	
	SFilterPort	FilterPort;	
} SFilterIp;

typedef struct
{
	__kernel_uid32_t uUid;	
	FLAG_FILTER FlagFilter;	
} SFilterUid;

typedef struct
{
	SHost OriHost;		
	SHost Host;		
	int nCount;
	SIpAddr Items[MAX_ITEM_COUNT];
	int nDotCnt;		
} SDnsResp;

typedef struct
{
	SHost OriHost;		
	SHost Host;		
	int nCount;
	SIpAddr Items[MAX_ITEM_COUNT2];
	int nDotCnt;		
} SDnsResp2;

typedef struct
{
	SHost Host;			
	FLAG_FILTER	FlagFilter;	
	SFilterPort	FilterPort;	
} SFilterHost;


#if 0
#define PACKFILTER_IOCTL_BASE	'y'
#define PACKFILTER_IOCTL_SET_FILTERIP 	_IOW(PACKFILTER_IOCTL_BASE, 0x80, SFilterItem)	
#define PACKFILTER_IOCTL_SET_FILTERHOST _IOW(PACKFILTER_IOCTL_BASE, 0x81, SFilterItem)	
#define PACKFILTER_IOCTL_GET_NEWIP 		_IOR(PACKFILTER_IOCTL_BASE, 0x82, SHostId)		
#define PACKFILTER_IOCTL_GET_IPSTATRSU	_IOWR(PACKFILTER_IOCTL_BASE, 0x83, SNodeStatRsu)
#define PACKFILTER_IOCTL_SET_DEFAULTFILTER	_IOW(PACKFILTER_IOCTL_BASE, 0x84, FLAG_FILTER)	
#define PACKFILTER_IOCTL_GET_DNS_REQ_PACK	_IOWR(PACKFILTER_IOCTL_BASE, 0x85, void *)		
#define PACKFILTER_IOCTL_GET_DNS_RES_PACK	_IOWR(PACKFILTER_IOCTL_BASE, 0x86, void *)		
#define PACKFILTER_IOCTL_SET_ENABLED		_IOW(PACKFILTER_IOCTL_BASE, 0x87, int)			
#define PACKFILTER_IOCTL_SET_REINIT			_IOW(PACKFILTER_IOCTL_BASE, 0x88, )				
#define PACKFILTER_IOCTL_SET_REINIT			_IOW(PACKFILTER_IOCTL_BASE, 0x89, SFilterUid)	
#else
#define PACKFILTER_IOCTL_SET_FILTERIP	 	0x80	
#define PACKFILTER_IOCTL_SET_FILTERHOST 	0x81	
#define PACKFILTER_IOCTL_GET_NEWIP	 	0x82	
#define PACKFILTER_IOCTL_SET_DEFAULTFILTER	0x84	
#define PACKFILTER_IOCTL_GET_DNS_REQ_PACK	0x85	
#define PACKFILTER_IOCTL_GET_DNS_RES_PACK	0x86	
#define PACKFILTER_IOCTL_SET_ENABLED		0x87	
#define PACKFILTER_IOCTL_SET_REINIT		0x88	
#define PACKFILTER_IOCTL_SET_FILTERUID	 	0x89	
#endif	

static inline void MEMCPY(void *pDest, const void *pSrc, int nLen)
{
	int i;

	for (i = 0; i < nLen; ++ i)
	{
		*((unsigned char *)pDest + i) = *((unsigned char *)pSrc + i);
	}
}


static inline int Domain_Cmp(const char* sStr1, int nLen1, const char* sStr2, int nLen2)
{
	-- nLen1;
	-- nLen2;
	while ((nLen1 >= 0) && (nLen2 >= 0))
	{
		if (*(sStr1 + nLen1) == *(sStr2 + nLen2))
		{
			-- nLen1;
			-- nLen2;
		}
		else if ((*(sStr1 + nLen1) == '*') || (*(sStr2 + nLen2) == '*'))	
		{
			return 0;
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
		else if ((*(sStr2 + nLen2) == '*')	
			|| ((nLen2 == 1) && (*sStr2 == '*') && (*(sStr2 + 1) == '.')))	
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
		if ((*(sStr1 + nLen1) == '*')	
			|| ((nLen1 == 1) && (*sStr1 == '*') && (*(sStr1 + 1) == '.')))	
		{
			return 0;
		}
		else
		{
			return 1;
		}
	}
}

#endif	
//mahui01@wind-mobi.com 2018/9/18 add for lenovo CSDK end