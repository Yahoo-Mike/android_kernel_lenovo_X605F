//mahui01@wind-mobi.com 2018/9/17 add for lenovo CSDK begin

#ifndef BTREE_H
#define BTREE_H


typedef unsigned long BTREE_STORED;
typedef void * BTREE_KEY;
typedef signed long BTREE_POS;

typedef int (* PROC_CMP)(void *, void *);

#define BTREE_ORDER_MAX 31
extern BTREE_POS BTREE_ORDER;
extern BTREE_POS BTREE_ORDER_HALF;

typedef struct btree_struct {
	BTREE_STORED stored;	
	BTREE_POS count;		
	struct btree_struct *parent;
	struct btree_struct **ptr;	
	BTREE_KEY *key;	
} BTREE;


BTREE *btree_new(void);
BTREE *btree_find(PROC_CMP pCmp, BTREE *node, BTREE_KEY num, BTREE_POS *pos);
BTREE *btree_add(PROC_CMP pCmp, BTREE *node, BTREE_KEY num, BTREE_POS *pos);
void btree_print(BTREE *node);
void btree_display(BTREE *node, int only_level);
long btree_count(BTREE *node);
int btree_levels(BTREE *node);
void btree_display2(BTREE *node, int level);
void btree_free(BTREE *node);
void btree_delete(BTREE *node, BTREE_POS pos);


#define BTREE_BIT(bit) ((BTREE_STORED)1 << (bit))


#define BTREE_BITMASK(bit) ( ( ( BTREE_BIT(bit)-1 ) << 1 ) + 1 )


#define BTREE_BITMASK_LOHI(bit1, bit2) ((bit1)==0 ? BTREE_BITMASK(bit2) \
	: (BTREE_BITMASK(bit2) ^ BTREE_BITMASK(bit1-1)))


#define BTREE_POS_STORED(node, pos) ((node)->stored & BTREE_BIT(pos))


#define BTREE_POS_CLEARED(node, pos) (!BTREE_POS_STORED((node), (pos)))


#endif
//mahui01@wind-mobi.com 2018/9/17 add for lenovo CSDK end