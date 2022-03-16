/*
CC BY-NC-SA, Attribution-NonCommercial-ShareAlike
Author  - Byungkwon Choi
Contact - cbkbrad@kaist.ac.kr
https://github.com/nfsp3k/DFC
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>

#ifndef u64
#define u64 uint64_t
#endif

#ifndef u32
#define u32 uint32_t
#endif

#ifndef u16
#define u16 uint16_t
#endif

#ifndef u8
#define u8 uint8_t
#endif

/****************************************************/
/*         Parameters: DF size, CT size             */
/****************************************************/
#define DF_SIZE         0x10000
#define DF_SIZE_REAL    0x2000

#define CT_TYPE1_PID_CNT_MAX    200
#define CT1_TABLE_SIZE          256
#define CT2_TABLE_SIZE          0x1000
#define CT3_TABLE_SIZE          0x1000
#define CT4_TABLE_SIZE          0x20000
#define CT8_TABLE_SIZE          0x20000

#define RECURSIVE_CT_SIZE    4096

#define BTYPE    register u16

/****************************************************/

/****************************************************/
/*        You don't need to care about this         */
/****************************************************/
#define BINDEX(x)    ((x) >> 3)
#define BMASK(x)     (1 << ((x) & 0x7))

#define DF_MASK    (DF_SIZE - 1)

#define CT2_TABLE_SIZE_MASK    (CT2_TABLE_SIZE-1)
#define CT3_TABLE_SIZE_MASK    (CT3_TABLE_SIZE-1)
#define CT4_TABLE_SIZE_MASK    (CT4_TABLE_SIZE-1)
#define CT8_TABLE_SIZE_MASK    (CT8_TABLE_SIZE-1)

#ifndef likely
#define likely(expr)      __builtin_expect(!!(expr), 1)
#endif
#ifndef unlikely
#define unlikely(expr)    __builtin_expect(!!(expr), 0)
#endif

/****************************************************/
/* Compact Table Structures */
/****************************************************/
/* Compact Table (CT1) */
typedef struct pid_list_
{
	u32 pid[CT_TYPE1_PID_CNT_MAX];
	u16 cnt;
} CT_Type_1;

/****************************************************/
/*                For New designed CT2              */
/****************************************************/
typedef struct CT_Type_2_2B_Array_
{
	u16 pat;     // 2B pattern
	u32 cnt;     // Number of PIDs
	u32 *pid;	  // list of PIDs
} CT_Type_2_2B_Array;

/* Compact Table (CT2) */
typedef struct CT_Type_2_2B_
{
	u32 cnt;
	CT_Type_2_2B_Array *array;
} CT_Type_2_2B;
/****************************************************/

typedef struct CT_Type_2_Array_
{
	u32 pat;     // Maximum 4B pattern
	u32 cnt;     // Number of PIDs
	u32 *pid;	  // list of PIDs
	u8 *DirectFilter;
	CT_Type_2_2B *CompactTable;
} CT_Type_2_Array;

/* Compact Table (CT2) */
typedef struct CT_Type_2_
{
	u32 cnt;
	CT_Type_2_Array *array;
} CT_Type_2;

/****************************************************/
/*                For New designed CT8              */
/****************************************************/
typedef struct CT_Type_2_8B_Array_
{
	u64 pat;     // 8B pattern
	u32 cnt;     // Number of PIDs
	u32 *pid;	  // list of PIDs
	u8 *DirectFilter;
	CT_Type_2_2B *CompactTable;
} CT_Type_2_8B_Array;

/* Compact Table (CT2) */
typedef struct CT_Type_2_8B_
{
	u32 cnt;
	CT_Type_2_8B_Array *array;
} CT_Type_2_8B;
/****************************************************/

typedef struct _dfc_pattern
{
	struct _dfc_pattern *next;

	unsigned char       *patrn;     // upper case pattern
	unsigned char       *casepatrn; // original pattern
	int                  n;         // Patternlength
	int                  nocase;    // Flag for case-sensitivity. (0: case-sensitive pattern, 1: opposite)

	u32             sids_size;
	u32            *sids;      // external id (unique)
	u32             iid;       // internal id (used in DFC library only)

} DFC_PATTERN;


typedef struct
{
	DFC_PATTERN   ** init_hash; // To cull duplicate patterns
	DFC_PATTERN    * dfcPatterns;
	DFC_PATTERN   ** dfcMatchList;

	int          numPatterns;

	/* Direct Filter (DF1) for all patterns */
	u8 DirectFilter1[DF_SIZE_REAL];

	u8 cDF0[256];
	u8 cDF1[DF_SIZE_REAL];
	u8 cDF2[DF_SIZE_REAL];

	u8 ADD_DF_4_plus[DF_SIZE_REAL];
	u8 ADD_DF_4_1[DF_SIZE_REAL];

	u8 ADD_DF_8_1[DF_SIZE_REAL];
	u8 ADD_DF_8_2[DF_SIZE_REAL];

	/* Compact Table (CT1) for 1B patterns */
	CT_Type_1 CompactTable1[CT1_TABLE_SIZE];

	/* Compact Table (CT2) for 2B patterns */
	CT_Type_2 CompactTable2[CT2_TABLE_SIZE];

	/* Compact Table (CT4) for 4B ~ 7B patterns */
	CT_Type_2 CompactTable4[CT4_TABLE_SIZE];

	/* Compact Table (CT8) for 8B ~ patterns */
	CT_Type_2_8B CompactTable8[CT8_TABLE_SIZE];

} DFC_STRUCTURE;

/****************************************************/
typedef enum _dfcMemoryType
{
	DFC_MEMORY_TYPE__NONE = 0,
	DFC_MEMORY_TYPE__DFC,
	DFC_MEMORY_TYPE__PATTERN,
	DFC_MEMORY_TYPE__CT1,
	DFC_MEMORY_TYPE__CT2,
	DFC_MEMORY_TYPE__CT3,
	DFC_MEMORY_TYPE__CT4,
	DFC_MEMORY_TYPE__CT8
} dfcMemoryType;

typedef enum _dfcDataType
{
	DFC_NONE = 0,
	DFC_PID_TYPE,
	DFC_CT_Type_2_Array,
	DFC_CT_Type_2_2B_Array,
	DFC_CT_Type_2_8B_Array
} dfcDataType;
/****************************************************/

/****************************************************/
extern DFC_STRUCTURE * DFC_New(void);
extern void DFC_Free(DFC_STRUCTURE *dfc);

extern int DFC_AddPattern(DFC_STRUCTURE *dfc, unsigned char *pat, int n, int nocase, u32 sid);
extern int DFC_Compile(DFC_STRUCTURE *dfc);
extern int DFC_Search(DFC_STRUCTURE *dfc, unsigned char *buf, int buflen, void* r, void (*Match)(void*, unsigned char *, u32 *, u32));
/****************************************************/

#ifndef UINT32_C
#define UINT32_C(value) (value##UL)
#endif

/*************************************************************************************/
#define INIT_HASH_SIZE       65536
#define RECURSIVE_BOUNDARY   5
/*************************************************************************************/

/*************************************************************************************/
#define pattern_interval     32
#define min_pattern_interval 32
/*************************************************************************************/

static unsigned char xlatcase[256];

static int my_free(void *ptr)
{
	if (ptr)
	{
		free(ptr);
	}

	return 0;
}

static void *my_malloc(int size)
{
	return malloc(size);
}

static void *my_zalloc(int size)
{
	void *p_new = NULL;

	p_new = malloc(size);
	if (!p_new)
	{
		return NULL;
	}

	memset(p_new, 0, size);

	return p_new;
}

static void *my_realloc(void *ptr, size_t size)
{
	return realloc(ptr, size);
}

static inline int my_strncmp(unsigned char *a, unsigned char *b, int n)
{
	int i;
	for (i = 0; i < n ; i++)
	{
		if (a[i] != b[i])
		{
			return -1;
		}
	}
	return 0;
}

static inline int my_strncasecmp(unsigned char *a, unsigned char *b, int n)
{
	int i;
	for (i = 0; i < n ; i++)
	{
		if (tolower(a[i]) != tolower(b[i]))
		{
			return -1;
		}
	}
	return 0;
}

static u32 my_crc32_u8(u32 prevcrc, u8 v)
{
	int bit;

	u32 crc = prevcrc;
	crc ^= v;

	for (bit = 0 ; bit < 8 ; bit++)
	{
		if (crc & 1)
		{
			crc = (crc >> 1) ^ UINT32_C(0x82f63b78);
		}
		else
		{
			crc = (crc >> 1);
		}
	}
	return crc;
}

static u32 my_crc32_u16(u32 prevcrc, u16 v)
{
	u32 crc = prevcrc;
	crc = my_crc32_u8(crc, v & 0xff);
	crc = my_crc32_u8(crc, (v >> 8) & 0xff);
	return crc;
}

static u32 my_crc32_u32(u32 prevcrc, u32 v)
{
	u32 crc = prevcrc;
	crc = my_crc32_u16(crc, v & 0xffff);
	crc = my_crc32_u16(crc, (v >> 16) & 0xffff);
	return crc;
}


static u64 my_crc32_u64(u64 prevcrc, u64 v)
{
	u64 crc = prevcrc;
	crc = my_crc32_u32((u32) crc, v & 0xffffffff);
	crc = my_crc32_u32((u32) crc, (v >> 32) & 0xffffffff);
	return crc;
}

static void Build_pattern(DFC_PATTERN *p, u8 *flag, u8 *temp, u32 i, int j, int k)
{
	if (p->nocase)
	{
		if ((p->patrn[j] >= 65 && p->patrn[j] <= 90) || (p->patrn[j] >= 97 && p->patrn[j] <= 122))
		{
			if (flag[k] == 0)
			{
				temp[k] = tolower(p->patrn[j]);
			}
			else
			{
				temp[k] = toupper(p->patrn[j]);
			}
		}
		else
		{
			temp[k] = p->patrn[j];
		}
	}
	else
	{
		temp[k] = p->casepatrn[j]; // original pattern
	}

	return ;
}

DFC_STRUCTURE * DFC_New(void)
{
	DFC_STRUCTURE * p;

	int i;
	for (i = 0; i < 256; i++)
	{
		xlatcase[i] = (unsigned char)toupper(i);
	}

	p = (DFC_STRUCTURE *)my_malloc(sizeof(DFC_STRUCTURE));
	if (p)
	{
		memset(p, 0, sizeof(DFC_STRUCTURE));

		p->init_hash = my_malloc(sizeof(DFC_PATTERN *) * INIT_HASH_SIZE);
		if (p->init_hash == NULL)
		{
			my_free(p);
			return NULL;
		}

		memset(p->init_hash, 0, sizeof(DFC_PATTERN *) * INIT_HASH_SIZE);
	}

	return p;
}

void DFC_Free(DFC_STRUCTURE *dfc)
{
	u32 j, l;
	int i, k;

	if (dfc == NULL)
	{
		return;
	}

	if (dfc->dfcPatterns != NULL)
	{
		DFC_PATTERN *plist;
		DFC_PATTERN *p_next;

		for (plist = dfc->dfcPatterns; plist != NULL;)
		{
			if (plist->patrn != NULL)
			{
				my_free(plist->patrn);
			}

			if (plist->casepatrn != NULL)
			{
				my_free(plist->casepatrn);
			}

			if (plist->sids != NULL)
			{
				my_free(plist->sids);
			}

			p_next = plist->next;
			my_free(plist);
			plist = p_next;
		}
	}

	if (dfc->dfcMatchList != NULL)
	{
		my_free(dfc->dfcMatchList);
	}

	for (i = 0; i < CT2_TABLE_SIZE; i++)
	{
		for (j = 0; j < dfc->CompactTable2[i].cnt; j++)
		{
			my_free(dfc->CompactTable2[i].array[j].pid);

			if (dfc->CompactTable2[i].array[j].DirectFilter != NULL)
			{
				my_free(dfc->CompactTable2[i].array[j].DirectFilter);
			}

			if (dfc->CompactTable2[i].array[j].CompactTable != NULL)
			{
				for (k = 0; k < RECURSIVE_CT_SIZE; k++)
				{
					for (l = 0; l < dfc->CompactTable2[i].array[j].CompactTable[k].cnt; l++)
					{
						my_free(dfc->CompactTable2[i].array[j].CompactTable[k].array[l].pid);
					}
					my_free(dfc->CompactTable2[i].array[j].CompactTable[k].array);
				}
				my_free(dfc->CompactTable2[i].array[j].CompactTable);
			}
		}

		my_free(dfc->CompactTable2[i].array);
	}

	for (i = 0; i < CT4_TABLE_SIZE; i++)
	{
		for (j = 0; j < dfc->CompactTable4[i].cnt; j++)
		{
			my_free(dfc->CompactTable4[i].array[j].pid);

			if (dfc->CompactTable4[i].array[j].DirectFilter != NULL)
			{
				my_free(dfc->CompactTable4[i].array[j].DirectFilter);
			}

			if (dfc->CompactTable4[i].array[j].CompactTable != NULL)
			{
				for (k = 0; k < RECURSIVE_CT_SIZE; k++)
				{
					for (l = 0; l < dfc->CompactTable4[i].array[j].CompactTable[k].cnt; l++)
					{
						my_free(dfc->CompactTable4[i].array[j].CompactTable[k].array[l].pid);
					}
					my_free(dfc->CompactTable4[i].array[j].CompactTable[k].array);
				}
				my_free(dfc->CompactTable4[i].array[j].CompactTable);
			}
		}

		my_free(dfc->CompactTable4[i].array);
	}

	for (i = 0; i < CT8_TABLE_SIZE; i++)
	{
		for (j = 0; j < dfc->CompactTable8[i].cnt; j++)
		{
			my_free(dfc->CompactTable8[i].array[j].pid);

			if (dfc->CompactTable8[i].array[j].DirectFilter != NULL)
			{
				my_free(dfc->CompactTable8[i].array[j].DirectFilter);
			}

			if (dfc->CompactTable8[i].array[j].CompactTable != NULL)
			{
				for (k = 0; k < RECURSIVE_CT_SIZE; k++)
				{
					for (l = 0; l < dfc->CompactTable8[i].array[j].CompactTable[k].cnt; l++)
					{
						my_free(dfc->CompactTable8[i].array[j].CompactTable[k].array[l].pid);
					}
					my_free(dfc->CompactTable8[i].array[j].CompactTable[k].array);
				}
				my_free(dfc->CompactTable8[i].array[j].CompactTable);
			}
		}

		my_free(dfc->CompactTable8[i].array);
	}

	my_free(dfc);
}

static inline u32 DFC_InitHashRaw(u8 *pat, u16 patlen, int nocase)
{
	u32 hash  = 0;
	int i;

	for (i = 0; i < patlen; i++)
	{
		hash += pat[i];
	}

	hash += nocase;

	return (hash % INIT_HASH_SIZE);
}

static inline DFC_PATTERN *DFC_InitHashLookup(DFC_STRUCTURE *ctx, u8 *pat, u16 patlen, int nocase)
{
	u32 hash = DFC_InitHashRaw(pat, patlen, nocase);
	DFC_PATTERN *t;

	if (ctx->init_hash == NULL)
	{
		return NULL;
	}

	for (t = ctx->init_hash[hash]; t != NULL; t = t->next)
	{
		if (t->n == patlen &&
			memcmp(t->casepatrn, pat, patlen) == 0)
		{
			return t;
		}
	}

	return NULL;
}

static inline int DFC_InitHashAdd(DFC_STRUCTURE *ctx, DFC_PATTERN *p)
{
	DFC_PATTERN *tt;
	DFC_PATTERN *t;
	u32 hash;

	hash = DFC_InitHashRaw(p->casepatrn, p->n, p->nocase);

	if (ctx->init_hash == NULL)
	{
		return 0;
	}

	if (ctx->init_hash[hash] == NULL)
	{
		ctx->init_hash[hash] = p;
		return 0;
	}

	tt = NULL;
	t = ctx->init_hash[hash];

	/* get the list tail */
	do
	{
		tt = t;
		t = t->next;
	}
	while (t != NULL);

	tt->next = p;

	return 0;
}

/*
*  Add a pattern to the list of patterns
*
*
* \param dfc    Pointer to the DFC structure
* \param pat    Pointer to the pattern
* \param n      Pattern length
* \param nocase Flag for case-insensitive (0 means case-sensitive)
* \param sid    External id
*
* \retval   0 On success to add new pattern.
* \retval   1 On success to add sid.
*/
int DFC_AddPattern(DFC_STRUCTURE * dfc, unsigned char *pat, int n, int nocase, u32 sid)
{
	DFC_PATTERN * plist = DFC_InitHashLookup(dfc, pat, n, nocase);

	if (plist == NULL)
	{
		unsigned char *d;
		int x;

		plist = (DFC_PATTERN *) my_zalloc(sizeof(DFC_PATTERN));
		if (plist == NULL)
		{
			return -1;
		}

		memset(plist, 0, sizeof(DFC_PATTERN));

		plist->patrn = (unsigned char *)my_zalloc(n);
		if (plist->patrn == NULL)
		{
			my_free(plist);
			return -1;
		}

		plist->casepatrn = (unsigned char *)my_zalloc(n);
		if (plist->casepatrn == NULL)
		{
			my_free(plist);
			my_free(plist->patrn);
			return -1;
		}

		plist->sids = (u32 *) my_zalloc(sizeof(u32));
		if (plist->sids == NULL)
		{
			my_free(plist);
			my_free(plist->patrn);
			my_free(plist->casepatrn);
			return -1;
		}

		/* ConvertCaseEx */
		d = plist->patrn;

		for (x = 0; x < n; x++)
		{
			d[x] = xlatcase[ pat[x] ];
		}

		memcpy(plist->casepatrn, pat, n);

		plist->n      = n;
		plist->nocase = nocase;
		plist->iid    = dfc->numPatterns; // internal id
		plist->next   = NULL;

		DFC_InitHashAdd(dfc, plist);

		/* sid update */
		plist->sids_size = 1;

		plist->sids[0] = sid;

		/* Add this pattern to the list */
		dfc->numPatterns++;

		return 0;
	}
	else
	{
		int found = 0;
		u32 x = 0;

		for (x = 0; x < plist->sids_size; x++)
		{
			if (plist->sids[x] == sid)
			{
				found = 1;
				break;
			}
		}

		if (found == 0)
		{
			u32 *tmp = (u32 *)my_realloc(plist->sids, sizeof(u32) * (plist->sids_size + 1));
			if (tmp == NULL)
			{
				return -1;
			}

			plist->sids = tmp;
			plist->sids[plist->sids_size] = sid;
			plist->sids_size++;
		}

		return 1;
	}
}

static int Add_PID_to_2B_CT(CT_Type_2_2B * CompactTable, u8 *temp, u32 pid, dfcMemoryType type)
{
	u32 j;
	u32 k;
	u32 crc = my_crc32_u16(0, *(u16*)temp);

	crc &= CT2_TABLE_SIZE_MASK;

	if (CompactTable[crc].cnt != 0)
	{
		for (j = 0; j < CompactTable[crc].cnt; j++)
		{
			if (CompactTable[crc].array[j].pat == *(u16*)temp)
			{
				break;
			}
		}

		if (j == CompactTable[crc].cnt)  // If not found,
		{
			CT_Type_2_2B_Array *tmp;
			CompactTable[crc].cnt++;

			tmp = (CT_Type_2_2B_Array *)my_realloc((void*)CompactTable[crc].array, sizeof(CT_Type_2_2B_Array) * CompactTable[crc].cnt);
			if (tmp == NULL)
			{
				return -1;
			}

			CompactTable[crc].array = tmp;
			CompactTable[crc].array[CompactTable[crc].cnt - 1].pat = *(u16*)temp;
			CompactTable[crc].array[CompactTable[crc].cnt - 1].cnt = 1;

			CompactTable[crc].array[CompactTable[crc].cnt - 1].pid = (u32 *)my_zalloc(sizeof(u32));
			if (CompactTable[crc].array[CompactTable[crc].cnt - 1].pid == NULL)
			{
				return -1;
			}

			CompactTable[crc].array[CompactTable[crc].cnt - 1].pid[0] = pid;
		}
		else   // If found,
		{
			for (k = 0; k < CompactTable[crc].array[j].cnt; k++)
			{
				if (CompactTable[crc].array[j].pid[k] == pid)
				{
					break;
				}
			}
			if (k == CompactTable[crc].array[j].cnt)
			{
				u32 *tmp;
				CompactTable[crc].array[j].cnt++;

				tmp = (u32 *)my_realloc((void*)CompactTable[crc].array[j].pid, sizeof(u32) * CompactTable[crc].array[j].cnt);
				if (tmp == NULL)
				{
					return -1;
				}

				CompactTable[crc].array[j].pid = tmp;
				CompactTable[crc].array[j].pid[CompactTable[crc].array[j].cnt - 1] = pid;
			}
		}
	}
	else   // If there is no elements in the CT4,
	{
		CompactTable[crc].cnt = 1;

		CompactTable[crc].array = (CT_Type_2_2B_Array *)my_zalloc(sizeof(CT_Type_2_2B_Array));
		if (CompactTable[crc].array == NULL)
		{
			return -1;
		}

		memset(CompactTable[crc].array, 0, sizeof(CT_Type_2_2B_Array));

		CompactTable[crc].array[0].pat = *(u16*)temp;
		CompactTable[crc].array[0].cnt = 1;

		CompactTable[crc].array[0].pid = (u32 *)my_zalloc(sizeof(u32));
		if (CompactTable[crc].array[0].pid == NULL)
		{
			return -1;
		}

		CompactTable[crc].array[0].pid[0] = pid;
	}

	return 0;
}

int DFC_Compile(DFC_STRUCTURE* dfc)
{
	u32 i = 0;
	u32 alpha_cnt;

	int j, k, l;
	u32 m, n;
	DFC_PATTERN *plist;

	u8 temp[8], flag[8];
	u16 fragment_16;
	u32 fragment_32;
	u64 fragment_64;
	u32 byteIndex, bitMask;

	/* ####################################################################################### */
	/* ###############                  MatchList initialization              ################ */
	/* ####################################################################################### */

	int begin_node_flag = 1;

	for (i = 0; i < INIT_HASH_SIZE; i++)
	{
		DFC_PATTERN *node = dfc->init_hash[i], *prev_node;
		int first_node_flag = 1;

		while (node != NULL)
		{
			if (begin_node_flag)
			{
				begin_node_flag = 0;
				dfc->dfcPatterns = node;
			}
			else
			{
				if (first_node_flag)
				{
					first_node_flag = 0;
					prev_node->next = node;
				}
			}
			prev_node = node;
			node = node->next;
		}
	}

	my_free(dfc->init_hash);
	dfc->init_hash = NULL;

	dfc->dfcMatchList = (DFC_PATTERN **)my_zalloc(sizeof(DFC_PATTERN*) * dfc->numPatterns);
	if (dfc->dfcMatchList == NULL)
	{
		return -1;
	}

	for (plist = dfc->dfcPatterns; plist != NULL; plist = plist->next)
	{
		if (dfc->dfcMatchList[plist->iid] != NULL)
		{
			printf("Internal ID ERROR : %u\n", plist->iid);
		}
		dfc->dfcMatchList[plist->iid] = plist;
	}

	/* ####################################################################################### */

	/* ####################################################################################### */
	/* ###############              0. Direct Filters initialization          ################ */
	/* ####################################################################################### */

	/* Initializing Bloom Filter */
	for (i = 0; i < DF_SIZE_REAL; i++)
	{
		dfc->DirectFilter1[i] = 0;
		dfc->ADD_DF_4_plus[i] = 0;
		dfc->ADD_DF_8_1[i] = 0;
		dfc->ADD_DF_4_1[i] = 0;
		dfc->cDF2[i] = 0;
		dfc->ADD_DF_8_2[i] = 0;
		dfc->cDF1[i] = 0;
	}

	for (i = 0; i < 256; i++)
	{
		dfc->cDF0[i] = 0;
	}

	/* ####################################################################################### */

	/* ####################################################################################### */
	/* ###############               Direct Filters setup                     ################ */
	/* ####################################################################################### */

	memset(dfc->CompactTable1, 0, sizeof(CT_Type_1)*CT1_TABLE_SIZE);

	for (plist = dfc->dfcPatterns; plist != NULL; plist = plist->next)
	{
		/* 0. Initialization for DF8 (for 1B patterns)*/
		if (plist->n == 1)
		{
			temp[0] = plist->casepatrn[0];
			for (j = 0; j < 256; j++)
			{
				temp[1] = j;

				fragment_16 = (temp[1] << 8) | temp[0];
				byteIndex = (u32)BINDEX(fragment_16 & DF_MASK);
				bitMask = BMASK(fragment_16 & DF_MASK);

				dfc->DirectFilter1[byteIndex] |= bitMask;
			}

			dfc->cDF0[temp[0]] = 1;
			if (dfc->CompactTable1[temp[0]].cnt == 0)
			{
				dfc->CompactTable1[temp[0]].cnt++;
				dfc->CompactTable1[temp[0]].pid[0] = plist->iid;
			}
			else
			{
				for (k = 0; k < dfc->CompactTable1[temp[0]].cnt; k++)
				{
					if (dfc->CompactTable1[temp[0]].pid[k] == plist->iid)
					{
						break;
					}
				}
				if (k == dfc->CompactTable1[temp[0]].cnt)
				{
					dfc->CompactTable1[temp[0]].pid[dfc->CompactTable1[temp[0]].cnt++] = plist->iid;
					if (dfc->CompactTable1[temp[0]].cnt >= CT_TYPE1_PID_CNT_MAX)
					{
						printf("Too many PIDs in CT1. You should expand the size.\n");
					}
				}
			}

			if (plist->nocase)
			{
				if (plist->casepatrn[0] >= 97/*a*/ && plist->casepatrn[0] <= 122/*z*/)
				{
					/* when the pattern is lower case */
					temp[0] = toupper(plist->casepatrn[0]);
				}
				else
				{
					/* when the pattern is upper case */
					temp[0] = tolower(plist->casepatrn[0]);
				}

				for (j = 0; j < 256; j++)
				{
					temp[1] = j;

					fragment_16 = (temp[1] << 8) | temp[0];
					byteIndex = (u32)BINDEX(fragment_16 & DF_MASK);
					bitMask = BMASK(fragment_16 & DF_MASK);

					dfc->DirectFilter1[byteIndex] |= bitMask;
				}

				dfc->cDF0[temp[0]] = 1;
				if (dfc->CompactTable1[temp[0]].cnt == 0)
				{
					dfc->CompactTable1[temp[0]].cnt++;
					dfc->CompactTable1[temp[0]].pid[0] = plist->iid;
				}
				else
				{
					for (k = 0; k < dfc->CompactTable1[temp[0]].cnt; k++)
					{
						if (dfc->CompactTable1[temp[0]].pid[k] == plist->iid)
						{
							break;
						}
					}
					if (k == dfc->CompactTable1[temp[0]].cnt)
					{
						dfc->CompactTable1[temp[0]].pid[dfc->CompactTable1[temp[0]].cnt++] = plist->iid;
						if (dfc->CompactTable1[temp[0]].cnt >= CT_TYPE1_PID_CNT_MAX)
						{
							printf("Too many PIDs in CT1. You should expand the size.\n");
						}
					}
				}
			}
		}

		/* 1. Initialization for DF1 */
		if (plist->n > 1)
		{
			alpha_cnt = 0;

			do
			{
				for (j = 1, k = 0; j >= 0; --j, k++)
				{
					flag[k] = (alpha_cnt >> j) & 1;
				}

				if (plist->n == 2)
				{
					for (j = plist->n - 2, k = 0; j < plist->n; j++, k++)
					{
						Build_pattern(plist, flag, temp, i, j, k);
					}
				}
				else if (plist->n == 3)
				{
					//for (j=0 , k=0; j < 2; j++, k++){
					for (j = plist->n - 2, k = 0; j < plist->n; j++, k++)
					{
						Build_pattern(plist, flag, temp, i, j, k);
					}
				}
				else if (plist->n < 8)
				{
					for (j = plist->n - 4, k = 0; j < plist->n - 2; j++, k++)
					{
						Build_pattern(plist, flag, temp, i, j, k);
					}
				}
				else     // len >= 8
				{
					for (j = min_pattern_interval * (plist->n - 8) / pattern_interval, k = 0;
						 j < min_pattern_interval * (plist->n - 8) / pattern_interval + 2; j++, k++)
					{
						Build_pattern(plist, flag, temp, i, j, k);
					}
				}

				fragment_16 = (temp[1] << 8) | temp[0];
				byteIndex = (u32)BINDEX(fragment_16 & DF_MASK);
				bitMask = BMASK(fragment_16 & DF_MASK);

				dfc->DirectFilter1[byteIndex] |= bitMask;

				if (plist->n == 2 || plist->n == 3)
				{
					dfc->cDF1[byteIndex] |= bitMask;
				}

				alpha_cnt++;
			}
			while (alpha_cnt < 4);
		}

		/* Initializing 4B DF, 8B DF */
		if (plist->n >= 4)
		{
			alpha_cnt = 0;

			do
			{
				for (j = 3, k = 0; j >= 0; --j, k++)
				{
					flag[k] = (alpha_cnt >> j) & 1;
				}

				if (plist->n < 8)
				{
					for (j = plist->n - 4, k = 0; j < plist->n; j++, k++)
					{
						Build_pattern(plist, flag, temp, i, j, k);
					}
				}
				else
				{
					for (j = min_pattern_interval * (plist->n - 8) / pattern_interval, k = 0;
						 j < min_pattern_interval * (plist->n - 8) / pattern_interval + 4; j++, k++)
					{
						Build_pattern(plist, flag, temp, i, j, k);
					}
				}

				byteIndex = BINDEX((*(((u16*)temp) + 1)) & DF_MASK);
				bitMask = BMASK((*(((u16*)temp) + 1)) & DF_MASK);

				dfc->ADD_DF_4_plus[byteIndex] |= bitMask;
				if (plist->n >= 4 && plist->n < 8)
				{
					dfc->ADD_DF_4_1[byteIndex] |= bitMask;

					fragment_16 = (temp[1] << 8) | temp[0];
					byteIndex = BINDEX(fragment_16 & DF_MASK);
					bitMask = BMASK(fragment_16 & DF_MASK);

					dfc->cDF2[byteIndex] |= bitMask;
				}
				alpha_cnt++;
			}
			while (alpha_cnt < 16);
		}

		if (plist->n >= 8)
		{
			alpha_cnt = 0;

			do
			{
				for (j = 7, k = 0; j >= 0; --j, k++)
				{
					flag[k] = (alpha_cnt >> j) & 1;
				}

				for (j = min_pattern_interval * (plist->n - 8) / pattern_interval, k = 0;
					 j < min_pattern_interval * (plist->n - 8) / pattern_interval + 8; j++, k++)
				{
					Build_pattern(plist, flag, temp, i, j, k);
				}

				byteIndex = BINDEX((*(((u16*)temp) + 3)) & DF_MASK);
				bitMask = BMASK((*(((u16*)temp) + 3)) & DF_MASK);

				dfc->ADD_DF_8_1[byteIndex] |= bitMask;

				byteIndex = BINDEX((*(((u16*)temp) + 2)) & DF_MASK);
				bitMask = BMASK((*(((u16*)temp) + 2)) & DF_MASK);

				dfc->ADD_DF_8_2[byteIndex] |= bitMask;

				alpha_cnt++;
			}
			while (alpha_cnt < 256);
		}

	}

	//printf("DF Initialization is done.\n");

	/* ####################################################################################### */

	/* ####################################################################################### */
	/* ###############                Compact Tables initialization           ################ */
	/* ####################################################################################### */

	memset(dfc->CompactTable2, 0, sizeof(CT_Type_2)*CT2_TABLE_SIZE);
	memset(dfc->CompactTable4, 0, sizeof(CT_Type_2)*CT4_TABLE_SIZE);
	memset(dfc->CompactTable8, 0, sizeof(CT_Type_2_8B)*CT8_TABLE_SIZE);

	/* ####################################################################################### */

	/* ####################################################################################### */
	/* ###############                   Compact Tables setup                 ################ */
	/* ####################################################################################### */

	for (plist = dfc->dfcPatterns; plist != NULL; plist = plist->next)
	{
		if (plist->n == 2 || plist->n == 3)
		{
			alpha_cnt = 0;

			do
			{
				u32 crc;

				// 1.
				for (j = 1, k = 0; j >= 0; --j, k++)
				{
					flag[k] = (alpha_cnt >> j) & 1;
				}

				for (j = plist->n - 2, k = 0; j < plist->n; j++, k++)
				{
					Build_pattern(plist, flag, temp, i, j, k);
				}

				// 2.
				fragment_16 = (temp[1] << 8) | temp[0];
				crc = my_crc32_u16(0, fragment_16);

				// 3.
				crc &= CT2_TABLE_SIZE_MASK;

				// 4.
				if (dfc->CompactTable2[crc].cnt != 0)
				{
					for (n = 0; n < dfc->CompactTable2[crc].cnt; n++)
					{
						if (dfc->CompactTable2[crc].array[n].pat == fragment_16)
						{
							break;
						}
					}

					if (n == dfc->CompactTable2[crc].cnt)  // If not found,
					{
						CT_Type_2_Array *tmp;
						dfc->CompactTable2[crc].cnt++;

						tmp = (CT_Type_2_Array *)my_realloc((void*)dfc->CompactTable2[crc].array, sizeof(CT_Type_2_Array) * dfc->CompactTable2[crc].cnt);
						if (tmp == NULL)
						{
							return -1;
						}

						dfc->CompactTable2[crc].array = tmp;
						dfc->CompactTable2[crc].array[dfc->CompactTable2[crc].cnt - 1].pat = fragment_16;
						dfc->CompactTable2[crc].array[dfc->CompactTable2[crc].cnt - 1].cnt = 1;

						dfc->CompactTable2[crc].array[dfc->CompactTable2[crc].cnt - 1].pid = (u32 *)my_zalloc(sizeof(u32));
						if (dfc->CompactTable2[crc].array[dfc->CompactTable2[crc].cnt - 1].pid == NULL)
						{
							return -1;
						}

						dfc->CompactTable2[crc].array[dfc->CompactTable2[crc].cnt - 1].pid[0] = plist->iid;
						dfc->CompactTable2[crc].array[dfc->CompactTable2[crc].cnt - 1].DirectFilter = NULL;
						dfc->CompactTable2[crc].array[dfc->CompactTable2[crc].cnt - 1].CompactTable = NULL;
					}
					else   // If found,
					{
						for (m = 0; m < dfc->CompactTable2[crc].array[n].cnt; m++)
						{
							if (dfc->CompactTable2[crc].array[n].pid[m] == plist->iid)
							{
								break;
							}
						}
						if (m == dfc->CompactTable2[crc].array[n].cnt)
						{
							u32 *tmp;
							dfc->CompactTable2[crc].array[n].cnt++;

							tmp = (u32 *)my_realloc((void*)dfc->CompactTable2[crc].array[n].pid, sizeof(u32) * dfc->CompactTable2[crc].array[n].cnt);
							if (tmp == NULL)
							{
								return -1;
							}

							dfc->CompactTable2[crc].array[n].pid = tmp;
							dfc->CompactTable2[crc].array[n].pid[dfc->CompactTable2[crc].array[n].cnt - 1] = plist->iid;
						}
					}
				}
				else   // If there is no elements in the CT4,
				{
					dfc->CompactTable2[crc].cnt = 1;

					dfc->CompactTable2[crc].array = (CT_Type_2_Array *)my_zalloc(sizeof(CT_Type_2_Array));
					if (dfc->CompactTable2[crc].array == NULL)
					{
						return -1;
					}

					memset(dfc->CompactTable2[crc].array, 0, sizeof(CT_Type_2_Array));

					dfc->CompactTable2[crc].array[0].pat = fragment_16;
					dfc->CompactTable2[crc].array[0].cnt = 1;

					dfc->CompactTable2[crc].array[0].pid = (u32 *)my_zalloc(sizeof(u32));
					if (dfc->CompactTable2[crc].array[0].pid == NULL)
					{
						return -1;
					}

					dfc->CompactTable2[crc].array[0].pid[0] = plist->iid;
					dfc->CompactTable2[crc].array[0].DirectFilter = NULL;
					dfc->CompactTable2[crc].array[0].CompactTable = NULL;
				}

				alpha_cnt++;
			}
			while (alpha_cnt < 4);
		}

		/* CT4 initialization */
		if (plist->n >= 4 && plist->n < 8)
		{
			alpha_cnt = 0;
			do
			{
				u32 crc;

				// 1.
				for (j = 3, k = 0; j >= 0; --j, k++)
				{
					flag[k] = (alpha_cnt >> j) & 1;
				}

				for (j = plist->n - 4, k = 0; j < plist->n; j++, k++)
				{
					Build_pattern(plist, flag, temp, i, j, k);
				}

				// 2.
				fragment_32 = (temp[3] << 24) | (temp[2] << 16) | (temp[1] << 8) | temp[0];
				crc = my_crc32_u32(0, fragment_32);

				// 3.
				crc &= CT4_TABLE_SIZE_MASK;

				// 4.
				if (dfc->CompactTable4[crc].cnt != 0)
				{
					for (n = 0; n < dfc->CompactTable4[crc].cnt; n++)
					{
						if (dfc->CompactTable4[crc].array[n].pat == fragment_32)
						{
							break;
						}
					}

					if (n == dfc->CompactTable4[crc].cnt)  // If not found,
					{
						CT_Type_2_Array *tmp;
						dfc->CompactTable4[crc].cnt++;

						tmp = (CT_Type_2_Array *)my_realloc((void*)dfc->CompactTable4[crc].array, sizeof(CT_Type_2_Array) * dfc->CompactTable4[crc].cnt);
						if (tmp == NULL)
						{
							return -1;
						}
						dfc->CompactTable4[crc].array = tmp;

						dfc->CompactTable4[crc].array[dfc->CompactTable4[crc].cnt - 1].pat = fragment_32;
						dfc->CompactTable4[crc].array[dfc->CompactTable4[crc].cnt - 1].cnt = 1;

						dfc->CompactTable4[crc].array[dfc->CompactTable4[crc].cnt - 1].pid = (u32 *)my_zalloc(sizeof(u32));
						if (dfc->CompactTable4[crc].array[dfc->CompactTable4[crc].cnt - 1].pid == NULL)
						{
							return -1;
						}

						dfc->CompactTable4[crc].array[dfc->CompactTable4[crc].cnt - 1].pid[0] = plist->iid;
						dfc->CompactTable4[crc].array[dfc->CompactTable4[crc].cnt - 1].DirectFilter = NULL;
						dfc->CompactTable4[crc].array[dfc->CompactTable4[crc].cnt - 1].CompactTable = NULL;
					}
					else   // If found,
					{
						for (m = 0; m < dfc->CompactTable4[crc].array[n].cnt; m++)
						{
							if (dfc->CompactTable4[crc].array[n].pid[m] == plist->iid)
							{
								break;
							}
						}
						if (m == dfc->CompactTable4[crc].array[n].cnt)
						{
							u32 *tmp;
							dfc->CompactTable4[crc].array[n].cnt++;

							tmp = (u32 *)my_realloc((void*)dfc->CompactTable4[crc].array[n].pid, sizeof(u32) * dfc->CompactTable4[crc].array[n].cnt);
							if (tmp == NULL)
							{
								return -1;
							}

							dfc->CompactTable4[crc].array[n].pid = tmp;
							dfc->CompactTable4[crc].array[n].pid[dfc->CompactTable4[crc].array[n].cnt - 1] = plist->iid;
						}
					}
				}
				else   // If there is no elements in the CT4,
				{
					dfc->CompactTable4[crc].cnt = 1;

					dfc->CompactTable4[crc].array = (CT_Type_2_Array *)my_zalloc(sizeof(CT_Type_2_Array));
					if (dfc->CompactTable4[crc].array == NULL)
					{
						printf("Failed to allocate memory for recursive things.\n");
						return -1;
					}

					memset(dfc->CompactTable4[crc].array, 0, sizeof(CT_Type_2_Array));

					dfc->CompactTable4[crc].array[0].pat = fragment_32;
					dfc->CompactTable4[crc].array[0].cnt = 1;

					dfc->CompactTable4[crc].array[0].pid = (u32 *)my_zalloc(sizeof(u32));
					if (dfc->CompactTable4[crc].array[0].pid == NULL)
					{
						printf("Failed to allocate memory for recursive things.\n");
						return -1;
					}

					dfc->CompactTable4[crc].array[0].pid[0] = plist->iid;
					dfc->CompactTable4[crc].array[0].DirectFilter = NULL;
					dfc->CompactTable4[crc].array[0].CompactTable = NULL;
				}
				alpha_cnt++;
			}
			while (alpha_cnt < 16);
		}

		/* CT8 initialization */
		if (plist->n >= 8)
		{
			alpha_cnt = 0;
			do
			{
				u64 crc;

				for (j = 7, k = 0; j >= 0; --j, k++)
				{
					flag[k] = (alpha_cnt >> j) & 1;
				}

				for (j = min_pattern_interval * (plist->n - 8) / pattern_interval, k = 0;
					 j < min_pattern_interval * (plist->n - 8) / pattern_interval + 8; j++, k++)
				{
					temp[k] = plist->patrn[j];
				}

				// 1. Calulating Indice
				fragment_32 = (temp[7] << 24) | (temp[6] << 16) | (temp[5] << 8) | temp[4];
				fragment_64 = ((u64)fragment_32 << 32) | (temp[3] << 24) | (temp[2] << 16) | (temp[1] << 8) | temp[0];

				crc = my_crc32_u64(0, fragment_64);
				crc &= CT8_TABLE_SIZE_MASK;

				if (dfc->CompactTable8[crc].cnt != 0)
				{
					for (n = 0; n < dfc->CompactTable8[crc].cnt; n++)
					{
						if (dfc->CompactTable8[crc].array[n].pat == fragment_64)
						{
							break;
						}
					}

					if (n == dfc->CompactTable8[crc].cnt)  // If not found,
					{
						CT_Type_2_8B_Array *tmp;
						dfc->CompactTable8[crc].cnt++;

						tmp = (CT_Type_2_8B_Array *)my_realloc((void*)dfc->CompactTable8[crc].array, sizeof(CT_Type_2_8B_Array) * dfc->CompactTable8[crc].cnt);
						if (tmp == NULL)
						{
							return -1;
						}

						dfc->CompactTable8[crc].array = tmp;
						dfc->CompactTable8[crc].array[dfc->CompactTable8[crc].cnt - 1].pat = fragment_64;
						dfc->CompactTable8[crc].array[dfc->CompactTable8[crc].cnt - 1].cnt = 1;

						dfc->CompactTable8[crc].array[dfc->CompactTable8[crc].cnt - 1].pid = (u32 *)my_zalloc(sizeof(u32));
						if (dfc->CompactTable8[crc].array[dfc->CompactTable8[crc].cnt - 1].pid == NULL)
						{
							printf("Failed to allocate memory for recursive things.\n");
							return -1;
						}

						dfc->CompactTable8[crc].array[dfc->CompactTable8[crc].cnt - 1].pid[0] = plist->iid;
						dfc->CompactTable8[crc].array[dfc->CompactTable8[crc].cnt - 1].DirectFilter = NULL;
						dfc->CompactTable8[crc].array[dfc->CompactTable8[crc].cnt - 1].CompactTable = NULL;
					}
					else   // If found,
					{
						for (m = 0; m < dfc->CompactTable8[crc].array[n].cnt; m++)
						{
							if (dfc->CompactTable8[crc].array[n].pid[m] == plist->iid)
							{
								break;
							}
						}
						if (m == dfc->CompactTable8[crc].array[n].cnt)
						{
							u32 *tmp;
							dfc->CompactTable8[crc].array[n].cnt++;

							tmp = (u32 *)my_realloc((void*)dfc->CompactTable8[crc].array[n].pid, sizeof(u32) * dfc->CompactTable8[crc].array[n].cnt);
							if (tmp == NULL)
							{
								return -1;
							}

							dfc->CompactTable8[crc].array[n].pid = tmp;
							dfc->CompactTable8[crc].array[n].pid[dfc->CompactTable8[crc].array[n].cnt - 1] = plist->iid;
						}
					}
				}
				else   // If there is no elements in the CT8,
				{
					dfc->CompactTable8[crc].cnt = 1;

					dfc->CompactTable8[crc].array = (CT_Type_2_8B_Array *)my_zalloc(sizeof(CT_Type_2_8B_Array));
					if (dfc->CompactTable8[crc].array == NULL)
					{
						printf("Failed to allocate memory for recursive things.\n");
						return -1;
					}

					memset(dfc->CompactTable8[crc].array, 0, sizeof(CT_Type_2_8B_Array));

					dfc->CompactTable8[crc].array[0].pat = fragment_64;
					dfc->CompactTable8[crc].array[0].cnt = 1;

					dfc->CompactTable8[crc].array[0].pid = (u32 *)my_zalloc(sizeof(u32));
					if (dfc->CompactTable8[crc].array[0].pid == NULL)
					{
						printf("Failed to allocate memory for recursive things.\n");
						return -1;
					}

					dfc->CompactTable8[crc].array[0].pid[0] = plist->iid;
					dfc->CompactTable8[crc].array[0].DirectFilter = NULL;
					dfc->CompactTable8[crc].array[0].CompactTable = NULL;
				}
				alpha_cnt++;
			}
			while (alpha_cnt < 256);
		}
	}

	//printf("CT Initialization is done.\n");

	/* ####################################################################################### */

	/* ####################################################################################### */
	/* ###############                   Recursive filtering                  ################ */
	/* ####################################################################################### */

	// Only for CT2 firstly
	for (i = 0; i < CT2_TABLE_SIZE; i++)
	{
		for (n = 0; n < dfc->CompactTable2[i].cnt; n++)
		{
			/* If the number of PID is bigger than 3, do recursive filtering */
			if (dfc->CompactTable2[i].array[n].cnt >= RECURSIVE_BOUNDARY)
			{
				int temp_cnt = 0; // cnt for 2 byte patterns.
				u32 *tempPID;

				/* Initialization */
				dfc->CompactTable2[i].array[n].DirectFilter = (u8*)my_zalloc(sizeof(u8) * DF_SIZE_REAL);
				if (dfc->CompactTable2[i].array[n].DirectFilter == NULL)
				{
					printf("Failed to allocate memory for recursive things.\n");
					return -1;
				}

				dfc->CompactTable2[i].array[n].CompactTable = (CT_Type_2_2B*)my_zalloc(sizeof(CT_Type_2_2B) * RECURSIVE_CT_SIZE);
				if (dfc->CompactTable2[i].array[n].CompactTable == NULL)
				{
					printf("Failed to allocate memory for recursive things.\n");
					return -1;
				}

				tempPID = (u32*)my_zalloc(sizeof(u32) * dfc->CompactTable2[i].array[n].cnt);
				if (tempPID == NULL)
				{
					printf("Failed to allocate memory for recursive things.\n");
					return -1;
				}

				memcpy(tempPID, dfc->CompactTable2[i].array[n].pid, sizeof(u32) * dfc->CompactTable2[i].array[n].cnt);

				my_free(dfc->CompactTable2[i].array[n].pid);
				dfc->CompactTable2[i].array[n].pid = NULL;

				for (m = 0; m < dfc->CompactTable2[i].array[n].cnt; m++)
				{
					int pat_len = dfc->dfcMatchList[tempPID[m]]->n - 2;

					if (pat_len == 0) /* When pat length is 2 */
					{
						u32 *tmp;
						temp_cnt ++;

						tmp = (u32 *)my_realloc(dfc->CompactTable2[i].array[n].pid, sizeof(u32) * temp_cnt);
						if (tmp == NULL)
						{
							return -1;
						}

						dfc->CompactTable2[i].array[n].pid = tmp;
						dfc->CompactTable2[i].array[n].pid[temp_cnt - 1] = tempPID[m];
					}
					else if (pat_len == 1)   /* When pat length is 3 */
					{
						if (dfc->dfcMatchList[tempPID[m]]->nocase)
						{
							temp[1] = tolower(dfc->dfcMatchList[tempPID[m]]->patrn[0]);
							for (l = 0; l < 256; l++)
							{
								temp[0] = l;

								fragment_16 = (temp[1] << 8) | temp[0];
								byteIndex = BINDEX(fragment_16);
								bitMask = BMASK(fragment_16);

								dfc->CompactTable2[i].array[n].DirectFilter[byteIndex] |= bitMask;

								Add_PID_to_2B_CT(dfc->CompactTable2[i].array[n].CompactTable, temp,	tempPID[m],
												 DFC_MEMORY_TYPE__CT2);
							}

							temp[1] = toupper(dfc->dfcMatchList[tempPID[m]]->patrn[0]);
							for (l = 0; l < 256; l++)
							{
								temp[0] = l;

								fragment_16 = (temp[1] << 8) | temp[0];
								byteIndex = BINDEX(fragment_16);
								bitMask = BMASK(fragment_16);

								dfc->CompactTable2[i].array[n].DirectFilter[byteIndex] |= bitMask;

								Add_PID_to_2B_CT(dfc->CompactTable2[i].array[n].CompactTable, temp,	tempPID[m],
												 DFC_MEMORY_TYPE__CT2);
							}
						}
						else
						{
							temp[1] = dfc->dfcMatchList[tempPID[m]]->casepatrn[0];
							for (l = 0; l < 256; l++)
							{
								temp[0] = l;

								fragment_16 = (temp[1] << 8) | temp[0];
								byteIndex = BINDEX(fragment_16);
								bitMask = BMASK(fragment_16);

								dfc->CompactTable2[i].array[n].DirectFilter[byteIndex] |= bitMask;

								Add_PID_to_2B_CT(dfc->CompactTable2[i].array[n].CompactTable, temp,	tempPID[m],
												 DFC_MEMORY_TYPE__CT2);
							}
						}
					}
				}

				dfc->CompactTable2[i].array[n].cnt = temp_cnt;
				my_free(tempPID);
			}
		}
	}

	// Only for CT4 firstly
	for (i = 0; i < CT4_TABLE_SIZE; i++)
	{
		for (n = 0; n < dfc->CompactTable4[i].cnt; n++)
		{
			/* If the number of PID is bigger than 3, do recursive filtering */
			if (dfc->CompactTable4[i].array[n].cnt >= RECURSIVE_BOUNDARY)
			{
				int temp_cnt = 0; // cnt for 4 byte patterns.
				u32 *tempPID;

				/* Initialization */
				dfc->CompactTable4[i].array[n].DirectFilter = (u8*)my_zalloc(sizeof(u8) * DF_SIZE_REAL);
				if (dfc->CompactTable4[i].array[n].DirectFilter == NULL)
				{
					printf("Failed to allocate memory for recursive things.\n");
					return -1;
				}

				dfc->CompactTable4[i].array[n].CompactTable = (CT_Type_2_2B*)my_zalloc(sizeof(CT_Type_2_2B) * RECURSIVE_CT_SIZE);
				if (dfc->CompactTable4[i].array[n].CompactTable == NULL)
				{
					printf("Failed to allocate memory for recursive things.\n");
					return -1;
				}

				tempPID = (u32*)my_zalloc(sizeof(u32) * dfc->CompactTable4[i].array[n].cnt);
				if (tempPID == NULL)
				{
					printf("Failed to allocate memory for recursive things.\n");
					return -1;
				}

				memcpy(tempPID, dfc->CompactTable4[i].array[n].pid, sizeof(u32) * dfc->CompactTable4[i].array[n].cnt);

				my_free(dfc->CompactTable4[i].array[n].pid);
				dfc->CompactTable4[i].array[n].pid = NULL;

				for (m = 0; m < dfc->CompactTable4[i].array[n].cnt; m++)
				{
					//DFC_PATTERN *mlist = dfc->dfcMatchList[];
					int pat_len = dfc->dfcMatchList[tempPID[m]]->n - 4;

					if (pat_len == 0) /* When pat length is 4 */
					{
						u32 *tmp;
						temp_cnt ++;

						tmp = (u32 *)my_realloc(dfc->CompactTable4[i].array[n].pid, sizeof(u32) * temp_cnt);
						if (tmp == NULL)
						{
							return -1;
						}

						dfc->CompactTable4[i].array[n].pid = tmp;
						dfc->CompactTable4[i].array[n].pid[temp_cnt - 1] = tempPID[m];
					}
					else if (pat_len == 1)   /* When pat length is 5 */
					{
						if (dfc->dfcMatchList[tempPID[m]]->nocase)
						{
							temp[1] = tolower(dfc->dfcMatchList[tempPID[m]]->patrn[0]);
							for (l = 0; l < 256; l++)
							{
								temp[0] = l;

								fragment_16 = (temp[1] << 8) | temp[0];
								byteIndex = BINDEX(fragment_16);
								bitMask = BMASK(fragment_16);

								dfc->CompactTable4[i].array[n].DirectFilter[byteIndex] |= bitMask;

								Add_PID_to_2B_CT(dfc->CompactTable4[i].array[n].CompactTable, temp,	tempPID[m],
												 DFC_MEMORY_TYPE__CT4);
							}

							temp[1] = toupper(dfc->dfcMatchList[tempPID[m]]->patrn[0]);
							for (l = 0; l < 256; l++)
							{
								temp[0] = l;

								fragment_16 = (temp[1] << 8) | temp[0];
								byteIndex = BINDEX(fragment_16);
								bitMask = BMASK(fragment_16);

								dfc->CompactTable4[i].array[n].DirectFilter[byteIndex] |= bitMask;

								Add_PID_to_2B_CT(dfc->CompactTable4[i].array[n].CompactTable, temp,	tempPID[m],
												 DFC_MEMORY_TYPE__CT4);
							}
						}
						else
						{
							temp[1] = dfc->dfcMatchList[tempPID[m]]->casepatrn[0];
							for (l = 0; l < 256; l++)
							{
								temp[0] = l;

								fragment_16 = (temp[1] << 8) | temp[0];
								byteIndex = BINDEX(fragment_16);
								bitMask = BMASK(fragment_16);

								dfc->CompactTable4[i].array[n].DirectFilter[byteIndex] |= bitMask;

								Add_PID_to_2B_CT(dfc->CompactTable4[i].array[n].CompactTable, temp,	tempPID[m],
												 DFC_MEMORY_TYPE__CT4);
							}
						}

					}
					else    /* When pat length is 7 (pat_len is equal to 2(6) or 3(7)) */
					{
						if (dfc->dfcMatchList[tempPID[m]]->nocase)
						{
							alpha_cnt = 0;
							do
							{
								for (l = 1, k = 0; l >= 0; --l, k++)
								{
									flag[k] = (alpha_cnt >> l) & 1;
								}

								for (l = pat_len - 2, k = 0; l <= pat_len - 1; l++, k++)
								{
									Build_pattern(dfc->dfcMatchList[tempPID[m]], flag, temp, 0, l, k);
								}

								fragment_16 = (temp[1] << 8) | temp[0];
								byteIndex = BINDEX(fragment_16);
								bitMask = BMASK(fragment_16);

								dfc->CompactTable4[i].array[n].DirectFilter[byteIndex] |= bitMask;

								Add_PID_to_2B_CT(dfc->CompactTable4[i].array[n].CompactTable, temp, tempPID[m],
												 DFC_MEMORY_TYPE__CT4);

								alpha_cnt++;
							}
							while (alpha_cnt < 4);
						}
						else   /* case sensitive pattern */
						{
							temp[0] = dfc->dfcMatchList[tempPID[m]]->casepatrn[pat_len - 2];
							temp[1] = dfc->dfcMatchList[tempPID[m]]->casepatrn[pat_len - 1];

							fragment_16 = (temp[1] << 8) | temp[0];
							byteIndex = BINDEX(fragment_16);
							bitMask = BMASK(fragment_16);

							dfc->CompactTable4[i].array[n].DirectFilter[byteIndex] |= bitMask;

							Add_PID_to_2B_CT(dfc->CompactTable4[i].array[n].CompactTable, temp, tempPID[m],
											 DFC_MEMORY_TYPE__CT4);
						}
					}
				}

				dfc->CompactTable4[i].array[n].cnt = temp_cnt;
				my_free(tempPID);
			}
		}
	}

	/* For CT8 */
	for (i = 0; i < CT8_TABLE_SIZE; i++)
	{
		for (n = 0; n < dfc->CompactTable8[i].cnt; n++)
		{
			/* If the number of PID is bigger than RECURSIVE_BOUNDARY, do recursive filtering */
			if (dfc->CompactTable8[i].array[n].cnt >= RECURSIVE_BOUNDARY)
			{
				int temp_cnt = 0; // cnt for 8 byte patterns.
				u32 *tempPID;

				/* Initialization */
				dfc->CompactTable8[i].array[n].DirectFilter = (u8*)my_zalloc(DF_SIZE_REAL * sizeof(u8));
				if (dfc->CompactTable8[i].array[n].DirectFilter == NULL)
				{
					printf("Failed to allocate memory for recursive things.\n");
					return -1;
				}

				dfc->CompactTable8[i].array[n].CompactTable = (CT_Type_2_2B *)my_zalloc(sizeof(CT_Type_2_2B) * RECURSIVE_CT_SIZE);
				if (dfc->CompactTable8[i].array[n].CompactTable == NULL)
				{
					printf("Failed to allocate memory for recursive things.\n");
					return -1;
				}

				tempPID = (u32*)my_zalloc(sizeof(u32) * dfc->CompactTable8[i].array[n].cnt);
				if (tempPID == NULL)
				{
					printf("Failed to allocate memory for recursive things.\n");
					return -1;
				}

				memcpy(tempPID, dfc->CompactTable8[i].array[n].pid, sizeof(u32) * dfc->CompactTable8[i].array[n].cnt);

				my_free(dfc->CompactTable8[i].array[n].pid);
				dfc->CompactTable8[i].array[n].pid = NULL;

				for (m = 0; m < dfc->CompactTable8[i].array[n].cnt; m++)
				{
					//DFC_PATTERN *mlist = dfc->dfcMatchList[];
					int pat_len = dfc->dfcMatchList[tempPID[m]]->n - 8;

					if (pat_len == 0) /* When pat length is 8 */
					{
						u32 *tmp;
						temp_cnt ++;

						tmp = (u32 *)my_realloc(dfc->CompactTable8[i].array[n].pid, sizeof(u32) * temp_cnt);
						if (tmp == NULL)
						{
							printf("Failed to allocate memory for recursive things.\n");
							return -1;
						}

						dfc->CompactTable8[i].array[n].pid = tmp;
						dfc->CompactTable8[i].array[n].pid[temp_cnt - 1] = tempPID[m];
					}
					else if (pat_len == 1)   /* When pat length is 9 */
					{
						if (dfc->dfcMatchList[tempPID[m]]->nocase)
						{
							temp[1] = tolower(dfc->dfcMatchList[tempPID[m]]->patrn[0]);
							for (l = 0; l < 256; l++)
							{
								temp[0] = l;

								fragment_16 = (temp[1] << 8) | temp[0];
								byteIndex = BINDEX(fragment_16);
								bitMask = BMASK(fragment_16);

								dfc->CompactTable8[i].array[n].DirectFilter[byteIndex] |= bitMask;

								Add_PID_to_2B_CT(dfc->CompactTable8[i].array[n].CompactTable, temp,	tempPID[m],
												 DFC_MEMORY_TYPE__CT8);
							}

							temp[1] = toupper(dfc->dfcMatchList[tempPID[m]]->patrn[0]);
							for (l = 0; l < 256; l++)
							{
								temp[0] = l;

								fragment_16 = (temp[1] << 8) | temp[0];
								byteIndex = BINDEX(fragment_16);
								bitMask = BMASK(fragment_16);

								dfc->CompactTable8[i].array[n].DirectFilter[byteIndex] |= bitMask;

								Add_PID_to_2B_CT(dfc->CompactTable8[i].array[n].CompactTable, temp,	tempPID[m],
												 DFC_MEMORY_TYPE__CT8);
							}
						}
						else
						{
							temp[1] = dfc->dfcMatchList[tempPID[m]]->casepatrn[0];
							for (l = 0; l < 256; l++)
							{
								temp[0] = l;

								fragment_16 = (temp[1] << 8) | temp[0];
								byteIndex = BINDEX(fragment_16);
								bitMask = BMASK(fragment_16);

								dfc->CompactTable8[i].array[n].DirectFilter[byteIndex] |= bitMask;

								Add_PID_to_2B_CT(dfc->CompactTable8[i].array[n].CompactTable, temp,	tempPID[m],
												 DFC_MEMORY_TYPE__CT8);
							}
						}

					}
					else    /* longer than or equal to 10 */
					{
						if (dfc->dfcMatchList[tempPID[m]]->nocase)
						{
							alpha_cnt = 0;
							do
							{
								for (l = 1, k = 0; l >= 0; --l, k++)
								{
									flag[k] = (alpha_cnt >> l) & 1;
								}

								for (l = pat_len - 2, k = 0; l <= pat_len - 1; l++, k++)
								{
									Build_pattern(dfc->dfcMatchList[tempPID[m]], flag, temp, 0, l, k);
								}

								fragment_16 = (temp[1] << 8) | temp[0];
								byteIndex = BINDEX(fragment_16);
								bitMask = BMASK(fragment_16);

								dfc->CompactTable8[i].array[n].DirectFilter[byteIndex] |= bitMask;

								Add_PID_to_2B_CT(dfc->CompactTable8[i].array[n].CompactTable, temp, tempPID[m],
												 DFC_MEMORY_TYPE__CT8);

								alpha_cnt++;
							}
							while (alpha_cnt < 4);
						}
						else   /* case sensitive pattern */
						{
							temp[0] = dfc->dfcMatchList[tempPID[m]]->casepatrn[pat_len - 2];
							temp[1] = dfc->dfcMatchList[tempPID[m]]->casepatrn[pat_len - 1];

							fragment_16 = (temp[1] << 8) | temp[0];
							byteIndex = BINDEX(fragment_16);
							bitMask = BMASK(fragment_16);

							dfc->CompactTable8[i].array[n].DirectFilter[byteIndex] |= bitMask;

							Add_PID_to_2B_CT(dfc->CompactTable8[i].array[n].CompactTable, temp, tempPID[m],
											 DFC_MEMORY_TYPE__CT8);
						}
					}
				}

				dfc->CompactTable8[i].array[n].cnt = temp_cnt;
				my_free(tempPID);
			}
		}
	}

	return 0;
}

static int Verification_CT1(DFC_STRUCTURE *dfc,
							unsigned char *buf,
							int matches,
							void* r,
							void (*Match)(void*, unsigned char *, u32 *, u32),
							const unsigned char *starting_point)
{
	int i;
	for (i = 0; i < dfc->CompactTable1[*(buf - 2)].cnt; i++)
	{
		u32 pid = dfc->CompactTable1[*(buf - 2)].pid[i];
		DFC_PATTERN *mlist = dfc->dfcMatchList[pid];

		Match(r, mlist->casepatrn, mlist->sids, mlist->sids_size);
		matches += mlist->sids_size;
	}
	return matches;
}

static int Verification_CT2(DFC_STRUCTURE *dfc,
							unsigned char *buf,
							int matches,
							void* r,
							void (*Match)(void*, unsigned char *, u32 *, u32),
							const unsigned char *starting_point)
{
	u32 crc = my_crc32_u16(0, *(u16*)(buf - 2));
	u32 i;

	// 2. calculate index
	crc &= CT2_TABLE_SIZE_MASK;

	for (i = 0; i < dfc->CompactTable2[crc].cnt; i++)
	{
		if (dfc->CompactTable2[crc].array[i].pat == *(u16*)(buf - 2))
		{
			u32 j;
			if (dfc->CompactTable2[crc].array[i].DirectFilter == NULL)
			{
				for (j = 0; j < dfc->CompactTable2[crc].array[i].cnt; j++)
				{
					u32 pid = dfc->CompactTable2[crc].array[i].pid[j];
					DFC_PATTERN *mlist = dfc->dfcMatchList[pid];

					if (buf - starting_point >= mlist->n)
					{
						if (mlist->nocase)
						{
							if (my_strncasecmp(buf - (mlist->n), mlist->casepatrn, mlist->n - 2) == 0)
							{
								Match(r, mlist->casepatrn, mlist->sids, mlist->sids_size);
								matches += mlist->sids_size;
							}
						}
						else
						{
							if (my_strncmp(buf - (mlist->n), mlist->casepatrn, mlist->n - 2) == 0)
							{
								Match(r, mlist->casepatrn, mlist->sids, mlist->sids_size);
								matches += mlist->sids_size;
							}
						}
					}
				}
			}
			else
			{
				u16 data;
				BTYPE index;
				BTYPE mask;

				for (j = 0; j < dfc->CompactTable2[crc].array[i].cnt; j++)
				{
					u32 pid = dfc->CompactTable2[crc].array[i].pid[j];
					DFC_PATTERN *mlist = dfc->dfcMatchList[pid];

					Match(r, mlist->casepatrn, mlist->sids, mlist->sids_size);
					matches += mlist->sids_size;
				}

				data = *(u16*)(buf - 4);
				index = BINDEX(data);
				mask = BMASK(data);

				if (dfc->CompactTable2[crc].array[i].DirectFilter[index] & mask)
				{
					u32 crc2 = my_crc32_u16(0, data);
					u32 k;

					// 2. calculate index
					crc2 &= CT2_TABLE_SIZE_MASK;

					for (k = 0; k < dfc->CompactTable2[crc].array[i].CompactTable[crc2].cnt; k++)
					{
						if (dfc->CompactTable2[crc].array[i].CompactTable[crc2].array[k].pat == data)
						{
							u32 l;
							for (l = 0; l < dfc->CompactTable2[crc].array[i].CompactTable[crc2].array[k].cnt; l++)
							{
								u32 pid = dfc->CompactTable2[crc].array[i].CompactTable[crc2].array[k].pid[l];
								DFC_PATTERN *mlist = dfc->dfcMatchList[pid];

								Match(r, mlist->casepatrn, mlist->sids, mlist->sids_size);
								matches += mlist->sids_size;
							}
							break;
						}
					}
				}
			}
			break;
		}
	}
	return matches;
}

static int Verification_CT4_7(DFC_STRUCTURE *dfc,
							  unsigned char *buf,
							  int matches,
							  void* r,
							  void (*Match)(void*, unsigned char *, u32 *, u32),
							  const unsigned char *starting_point)
{
	// 1. Convert payload to uppercase
	unsigned char *temp = buf - 2;
	u32 i;

	// 2. calculate crc
	u32 crc = my_crc32_u32(0, *(u32*)temp);

	// 3. calculate index
	crc &= CT4_TABLE_SIZE_MASK;

	// 4.
	for (i = 0; i < dfc->CompactTable4[crc].cnt; i++)
	{
		if (dfc->CompactTable4[crc].array[i].pat == *(u32*)temp)
		{
			u32 j;
			if (dfc->CompactTable4[crc].array[i].DirectFilter == NULL)
			{
				for (j = 0; j < dfc->CompactTable4[crc].array[i].cnt; j++)
				{
					u32 pid = dfc->CompactTable4[crc].array[i].pid[j];
					DFC_PATTERN *mlist = dfc->dfcMatchList[pid];

					if (buf - starting_point >= mlist->n - 2)
					{
						if (mlist->nocase)
						{
							if (my_strncasecmp(buf - (mlist->n - 2), mlist->casepatrn, mlist->n - 4) == 0)
							{
								Match(r, mlist->casepatrn, mlist->sids, mlist->sids_size);
								matches += mlist->sids_size;
							}
						}
						else
						{
							if (my_strncmp(buf - (mlist->n - 2), mlist->casepatrn, mlist->n - 4) == 0)
							{
								Match(r, mlist->casepatrn, mlist->sids, mlist->sids_size);
								matches += mlist->sids_size;
							}
						}
					}
				}
			}
			else
			{
				u16 data;
				BTYPE index;
				BTYPE mask;

				for (j = 0; j < dfc->CompactTable4[crc].array[i].cnt; j++)
				{
					u32 pid = dfc->CompactTable4[crc].array[i].pid[j];
					DFC_PATTERN *mlist = dfc->dfcMatchList[pid];

					Match(r, mlist->casepatrn, mlist->sids, mlist->sids_size);
					matches += mlist->sids_size;
				}

				data = *(u16*)(buf - 4);
				index = BINDEX(data);
				mask = BMASK(data);

				if (dfc->CompactTable4[crc].array[i].DirectFilter[index] & mask)
				{
					u32 crc2 = my_crc32_u16(0, data);
					u32 k;

					// 2. calculate index
					crc2 &= CT2_TABLE_SIZE_MASK;

					for (k = 0; k < dfc->CompactTable4[crc].array[i].CompactTable[crc2].cnt; k++)
					{
						if (dfc->CompactTable4[crc].array[i].CompactTable[crc2].array[k].pat == data)
						{
							u32 l;
							for (l = 0; l < dfc->CompactTable4[crc].array[i].CompactTable[crc2].array[k].cnt; l++)
							{
								u32 pid = dfc->CompactTable4[crc].array[i].CompactTable[crc2].array[k].pid[l];
								DFC_PATTERN *mlist = dfc->dfcMatchList[pid];

								if (mlist->nocase)
								{
									if (my_strncasecmp(buf - (mlist->n - 2), mlist->casepatrn, mlist->n - 6) == 0)
									{
										Match(r, mlist->casepatrn, mlist->sids, mlist->sids_size);
										matches += mlist->sids_size;
									}
								}
								else
								{
									if (my_strncmp(buf - (mlist->n - 2), mlist->casepatrn, mlist->n - 6) == 0)
									{
										Match(r, mlist->casepatrn, mlist->sids, mlist->sids_size);
										matches += mlist->sids_size;
									}
								}
							}
							break;
						}
					}
				}
			}
			break;
		}
	}
	return matches;
}

static int Verification_CT8_plus(DFC_STRUCTURE *dfc,
								 unsigned char *buf,
								 int matches,
								 void* r,
								 void (*Match)(void*, unsigned char *, u32 *, u32),
								 const unsigned char *starting_point)
{
	u32 fragment_32;
	u64 fragment_64;
	u64 crc;
	u32 i;

	// 1. Convert payload to uppercase
	unsigned char temp[8];
	unsigned char *s = buf - 2;
	int x;

	for (x = 0; x < 8; x++)
	{
		temp[x] = xlatcase[ s[x] ];
	}

	// 2. calculate crc
	fragment_32 = (temp[7] << 24) | (temp[6] << 16) | (temp[5] << 8) | temp[4];
	fragment_64 = ((u64)fragment_32 << 32) | (temp[3] << 24) | (temp[2] << 16) | (temp[1] << 8) | temp[0];
	crc = my_crc32_u64(0, fragment_64);

	// 3. calculate index
	crc &= CT8_TABLE_SIZE_MASK;

	for (i = 0; i < dfc->CompactTable8[crc].cnt; i++)
	{
		if (dfc->CompactTable8[crc].array[i].pat == fragment_64)
		{
			//matches++;	break;

			u32 j;
			if (dfc->CompactTable8[crc].array[i].DirectFilter == NULL)
			{
				for (j = 0; j < dfc->CompactTable8[crc].array[i].cnt; j++)
				{
					u32 pid = dfc->CompactTable8[crc].array[i].pid[j];
					DFC_PATTERN *mlist = dfc->dfcMatchList[pid];

					int comparison_requirement = min_pattern_interval * (mlist->n - 8) / pattern_interval + 2;
					if (buf - starting_point >= comparison_requirement)
					{
						if (mlist->nocase)
						{
							if (my_strncasecmp(buf - comparison_requirement, mlist->casepatrn, mlist->n) == 0)
							{
								Match(r, mlist->casepatrn, mlist->sids, mlist->sids_size);
								matches += mlist->sids_size;
							}
						}
						else
						{
							if (my_strncmp(buf - comparison_requirement, mlist->casepatrn, mlist->n) == 0)
							{
								Match(r, mlist->casepatrn, mlist->sids, mlist->sids_size);
								matches += mlist->sids_size;
							}
						}
					}
				}
				#if 1
			}
			else
			{
				u16 data;
				BTYPE index;
				BTYPE mask;

				for (j = 0; j < dfc->CompactTable8[crc].array[i].cnt; j++)
				{
					u32 pid = dfc->CompactTable8[crc].array[i].pid[j];
					DFC_PATTERN *mlist = dfc->dfcMatchList[pid];

					int comparison_requirement = min_pattern_interval * (mlist->n - 8) / pattern_interval + 2;
					if (mlist->nocase)
					{
						if (my_strncasecmp(buf - comparison_requirement, mlist->casepatrn, mlist->n) == 0)
						{
							Match(r, mlist->casepatrn, mlist->sids, mlist->sids_size);
							matches += mlist->sids_size;
						}
					}
					else
					{
						if (my_strncmp(buf - comparison_requirement, mlist->casepatrn, mlist->n) == 0)
						{
							Match(r, mlist->casepatrn, mlist->sids, mlist->sids_size);
							matches += mlist->sids_size;
						}
					}
				}

				data = *(u16*)(buf - 4);
				index = BINDEX(data);
				mask = BMASK(data);

				if (dfc->CompactTable8[crc].array[i].DirectFilter[index] & mask)
				{
					u32 crc2 = my_crc32_u16(0, data);
					u32 k;

					// 2. calculate index
					crc2 &= CT2_TABLE_SIZE_MASK;

					for (k = 0; k < dfc->CompactTable8[crc].array[i].CompactTable[crc2].cnt; k++)
					{
						if (dfc->CompactTable8[crc].array[i].CompactTable[crc2].array[k].pat == data)
						{
							u32 l;
							for (l = 0; l < dfc->CompactTable8[crc].array[i].CompactTable[crc2].array[k].cnt; l++)
							{
								u32 pid = dfc->CompactTable8[crc].array[i].CompactTable[crc2].array[k].pid[l];
								DFC_PATTERN *mlist = dfc->dfcMatchList[pid];

								int comparison_requirement = min_pattern_interval * (mlist->n - 8) / pattern_interval + 2;
								if (buf - starting_point >= comparison_requirement)
								{
									if (mlist->nocase)
									{
										if (my_strncasecmp(buf - comparison_requirement, mlist->casepatrn, mlist->n) == 0)
										{
											Match(r, mlist->casepatrn, mlist->sids, mlist->sids_size);
											matches += mlist->sids_size;
										}
									}
									else
									{
										if (my_strncmp(buf - comparison_requirement, mlist->casepatrn, mlist->n) == 0)
										{
											Match(r, mlist->casepatrn, mlist->sids, mlist->sids_size);
											matches += mlist->sids_size;
										}
									}

								}
							}
							break;
						}
					}
				}
			}
				#endif
			break;
		}
	}

	return matches;
}

static inline int Progressive_Filtering(DFC_STRUCTURE *dfc,
										unsigned char *buf,
										int matches,
										BTYPE idx,
										BTYPE msk,
										void* r,
										void (*Match)(void*, unsigned char *, u32 *, u32),
										const unsigned char *starting_point,
										int rest_len)
{
	if (dfc->cDF0[*(buf - 2)])
	{
		matches = Verification_CT1(dfc, buf, matches, r, Match, starting_point);
	}

	if (unlikely(dfc->cDF1[idx] & msk))
	{
		matches = Verification_CT2(dfc, buf, matches, r, Match, starting_point);
	}

	if (rest_len >= 4)
	{
		u16 data = *(u16*)(buf);
		BTYPE index = BINDEX(data);
		BTYPE mask = BMASK(data);

		if (unlikely((mask & dfc->ADD_DF_4_plus[index])))
		{
			u16 data8;
			BTYPE index8;
			BTYPE mask8;

			if (unlikely(mask & dfc->ADD_DF_4_1[index]))
			{
				matches = Verification_CT4_7(dfc, buf, matches, r, Match, starting_point);
			}

			data8 = *(u16*)(&buf[4]);
			index8 = BINDEX(data8);
			mask8 = BMASK(data8);

			if (unlikely(mask8 & dfc->ADD_DF_8_1[index8]))
			{
				data8 = *(u16*)(&buf[2]);
				index8 = BINDEX(data8);
				mask8 = BMASK(data8);

				if (unlikely(mask8 & dfc->ADD_DF_8_2[index8]))
				{
					if ((rest_len >= 8))
					{
						matches = Verification_CT8_plus(dfc, buf, matches, r, Match, starting_point);
						//matches ++;
					}
				}
			}
		}
	}

	return matches;
}

int DFC_Search(DFC_STRUCTURE *dfc, unsigned char *buf, int buflen, void* r, void (*Match)(void*, unsigned char *, u32 *, u32))
{
	u8 *DirectFilter1 = dfc->DirectFilter1;

	int i;
	int matches = 0;

	if (unlikely(buflen <= 0))
	{
		return 0;
	}

	for (i = 0; i < buflen - 1; i++)
	{
		u16 data = *(u16*)(&buf[i]);
		BTYPE index = BINDEX(data);
		BTYPE mask = BMASK(data);

		if (unlikely(DirectFilter1[index] & mask))
		{
			matches = Progressive_Filtering(dfc, &buf[i + 2], matches, index, mask, r, Match, buf, buflen - i);
		}
	}

	/* It is needed to check last 1 byte from payload */
	if (dfc->cDF0[buf[buflen - 1]])
	{
		for (i = 0; i < dfc->CompactTable1[buf[buflen - 1]].cnt; i++)
		{
			u32 pid = dfc->CompactTable1[buf[buflen - 1]].pid[i];
			DFC_PATTERN *mlist = dfc->dfcMatchList[pid];

			Match(r, mlist->casepatrn, mlist->sids, mlist->sids_size);
			matches += mlist->sids_size;
		}
	}

	return matches;
}

static void dfc_rule_match(void* r, unsigned char *casepatrn, u32 *sids, u32 sids_size)
{
	int i;
	int* eval_data = r;

	for (i = 0; i < sids_size; i++)
	{
		printf("sid %d\n", sids[i]);
		(*eval_data) ++;
	}
}

int main(int argc, char **argv)
{
	struct rule
	{
		int sid;
		int len;
		unsigned char content[32];
	};

	DFC_STRUCTURE *dfc;
	int eval_data = 0;

	unsigned char str[24] = "1234567890abcdefghijkl";
	int str_len = 20;

	int num_patterns = 5;

	struct rule rules[5] =
	{
		{1,  2, "AB"},
		{2,  2, "AB"},
		{3,  2, "EF"},
		{4,  2, "G8"},
		{5, 10, "ABCDEFGHIJ"},
	};

	int r = 0;
	int i;

	dfc = DFC_New();
	if (dfc == NULL)
	{
		return -1;
	}

	for (i = 0; r >= 0 && i < num_patterns; i++)
	{
		r = DFC_AddPattern(dfc, rules[i].content, rules[i].len, 1 /* case-insensitive */, rules[i].sid);
	}

	if (r < 0)
	{
		printf("DFC_AddPattern error");
		goto ERR;
	}

	printf("compiler\n");
	r = DFC_Compile(dfc);
	if (r < 0)
	{
		printf("DFC_Compile error");
		goto ERR;
	}

	printf("search start\n");
	DFC_Search(dfc, str, str_len, &eval_data, dfc_rule_match);

	printf("search finish, match count %d\n", eval_data);
	DFC_Free(dfc);

	return 0;

ERR:

	printf("DFC error\n");
	DFC_Free(dfc);

	return 0;
}