//mahui01@wind-mobi.com 2018/9/17 add for lenovo CSDK begin

#include <linux/kernel.h>
#include <linux/slab.h>

#include "wjfw.h"
#include "btree2.h"


BTREE_POS BTREE_ORDER;
BTREE_POS BTREE_ORDER_HALF;

inline BTREE *btree_new()
{
	BTREE *new = (BTREE *)MALLOC(sizeof(BTREE), ALLOCMEM_HIGH_FLAG | ALLOCMEM_ZERO_FLAG);
	if (new == NULL)
		printk("btree_new() failed\n");
	else {
		new->ptr = (BTREE **)MALLOC(sizeof(BTREE *)*BTREE_ORDER, ALLOCMEM_HIGH_FLAG | ALLOCMEM_ZERO_FLAG);
		if (new->ptr == NULL) {
			FREE(new);
			new = NULL;
		} else {
			new->key = (BTREE_KEY *)MALLOC(sizeof(BTREE_KEY)*(BTREE_ORDER-1),
							ALLOCMEM_HIGH_FLAG | ALLOCMEM_ZERO_FLAG);
			if (new->key == NULL) {
				FREE(new->ptr);
				FREE(new);
				new = NULL;
			} /*else
				memset(new->key, 0, sizeof(BTREE_KEY)*(BTREE_ORDER-1));*/
		}
	}
	return new;
}

BTREE *btree_find(PROC_CMP pCmp, BTREE *node, BTREE_KEY num, BTREE_POS *pos)
{
	BTREE_POS i;
	BTREE *prev=NULL;

	*pos = -1;


	while (node != NULL) {
		prev = node;
		for (i=0; i < node->count; ++i) {
			int r = pCmp(num, node->key[i]);
			if (r < 0) {
				node = node->ptr[i];
				break;
			} else if (r == 0) {
				if (BTREE_POS_STORED(node, i))
					*pos = i;
				node = NULL;
				break;
			}
		}
		if (node == prev && i == node->count)
			node = node->ptr[i];
	}

	return prev;
}

BTREE *btree_add_internal(PROC_CMP pCmp, BTREE *node, BTREE *left, BTREE *right,
	BTREE_KEY num, BTREE_POS *pos)
{

	

	
	node = btree_find(pCmp, node, num, pos);

	if (*pos >= 0)
		return node;	
	else if (node == NULL) 
		return (BTREE *)NULL;	
	else {
		BTREE_POS i, j;

		int r = node->count > 0 ? pCmp(num, node->key[0]) : -1;
		for (i=0; i < node->count && r >= 0; ++i)
		{
			r = pCmp(num, node->key[i]);
			if (r == 0)
				*pos = i;
		}

		if (*pos >= 0) {
			
			node->stored = node->stored | BTREE_BIT(*pos);
		} else if (node->count < BTREE_ORDER-1) {
			
			for (i=0; i < node->count; ++i)
			{
				if (pCmp(num, node->key[i]) <= 0)
				{
					break;
				}
			}
			if (node->count-1 >= i) {
				for (j=node->count-1; j>=i; --j) {
					
					node->ptr[j+2] = node->ptr[j+1];
					node->key[j+1] = node->key[j];
				}
				
				node->ptr[j+2] = node->ptr[j+1];
			}

			
			node->stored = ((i==0)?0:(node->stored & BTREE_BITMASK(i-1)))
				| ((node->stored & BTREE_BITMASK_LOHI(i, node->count-1)) << 1)
				| BTREE_BIT(i);
			node->key[i] = num;
			*pos = i;
			++node->count;

			if (left != NULL) {
				
				node->ptr[*pos] = left;
				left->parent = node;
				node->ptr[(*pos)+1] = right;
				right->parent = node;
			}
		} else {
			
			
			BTREE *new = btree_new();
			BTREE *ptr = NULL;
			BTREE *added;
			BTREE_POS temp_pos;
			BTREE_KEY temp_num;

			if (new == NULL)
				return (BTREE *)NULL;

			
			new->parent = node->parent;

			
			new->count = BTREE_ORDER_HALF;

			
			
			for (i=0; i < BTREE_ORDER-1; ++i)
			{
				if (pCmp(num, node->key[i]) <= 0)
				{
					break;
				}
			}
			
			node->stored = ((i==0)?0:(node->stored & BTREE_BITMASK(i-1)))
				| ((node->stored & BTREE_BITMASK_LOHI(i, node->count-1)) << 1)
				| BTREE_BIT(i);

			
			
			new->stored = (node->stored
				& BTREE_BITMASK_LOHI(BTREE_ORDER_HALF+1, BTREE_ORDER-1))
				>> (BTREE_ORDER_HALF+1);

			
			for (j=BTREE_ORDER_HALF-1; j>=0; --j) {
				if (pCmp(num, node->key[j+BTREE_ORDER_HALF]) > 0) {
					new->key[j] = num;
					num = node->key[j+BTREE_ORDER_HALF];
					if (*pos < 0) {
						ptr = new;
						*pos = j;
					}
				} else
					new->key[j] = node->key[j+BTREE_ORDER_HALF];

				if (*pos >= 0) {
					if (j < BTREE_ORDER_HALF-1)
						new->ptr[j+1] = node->ptr[j+2+BTREE_ORDER_HALF];
				} else
					new->ptr[j+1] = node->ptr[j+1+BTREE_ORDER_HALF];
			}
			if (*pos >= 0)
				new->ptr[0] = node->ptr[BTREE_ORDER_HALF+1];
			else
				new->ptr[0] = node->ptr[BTREE_ORDER_HALF];

			
			for (j=0; j<=BTREE_ORDER_HALF; ++j) {
				if (new->ptr[j] != NULL)
					new->ptr[j]->parent = new;
			}

			
			node->count = BTREE_ORDER_HALF;
			for (j=0; j<BTREE_ORDER_HALF; ++j) {
				if (pCmp(num, node->key[j]) < 0) {
					temp_num = num;
					num = node->key[j];
					node->key[j] = temp_num;
					if (*pos < 0) {
						ptr = node;
						*pos = j;
					}
				}
			}
			
			for (j=BTREE_ORDER_HALF+1; j<BTREE_ORDER; ++j)
				node->ptr[j] = NULL;
			
			if (ptr == node)
				for (j=BTREE_ORDER_HALF; j > *pos+1; --j)
					node->ptr[j] = node->ptr[j-1];

			
			if (node->parent == NULL)
				node->parent = btree_new();
			else {
				
				for (i=0; i <= node->parent->count; ++i) {
					if (node->parent->ptr[i] == node) {
						node->parent->ptr[i] = NULL;
						break;
					}
				}
			}


			
			if (ptr == NULL) {
				added = btree_add_internal(pCmp, node->parent, node, new, num,
					&temp_pos);
				*pos = temp_pos;
			} else
				added = btree_add_internal(pCmp, node->parent, node, new, num,
					&temp_pos);


			
			if (BTREE_POS_CLEARED(node, BTREE_ORDER_HALF))
				added->stored = added->stored ^ BTREE_BIT(temp_pos);
			node->stored = node->stored & BTREE_BITMASK(BTREE_ORDER_HALF-1);

			if (left != NULL) {
				if (ptr == NULL) {
					node->ptr[BTREE_ORDER_HALF] = left;
					new->ptr[0] = right;
					right->parent = new;
					node = added;
				} else {
					
					ptr->ptr[*pos] = left;
					ptr->ptr[(*pos)+1] = right;
					left->parent = ptr;
					right->parent = ptr;
					node = ptr;
				}
			}
		}

		return node;
	}
}

BTREE *btree_add(PROC_CMP pCmp, BTREE *node, BTREE_KEY num, BTREE_POS *pos)
{
	return btree_add_internal(pCmp, node, (BTREE *)NULL, (BTREE *)NULL, num, pos);
}

void btree_delete(BTREE *node, BTREE_POS pos)
{

	
	node->stored = (node->stored | BTREE_BIT(pos)) ^ BTREE_BIT(pos);
}

void btree_print(BTREE *node)
{
	BTREE_POS i;

	if (node == NULL || node->count == 0) {
		printk("EMPTY\n");
		return;
	}

	for (i=0; i < node->count; ++i) {
		if (node->ptr[i] != NULL) {
			btree_print(node->ptr[i]);
		}
		printk("%ld ", (long)node->key[i]);
	}
	if (node->ptr[i] != NULL) {
		btree_print(node->ptr[i]);
	}
}

int btree_levels(BTREE *node)
{
	BTREE_POS i;
	int levels;
	int max=1;

	if (node == NULL)
		return 0;
	for (i=0; i <= node->count; ++i) {
		levels = 1 + btree_levels(node->ptr[i]);
		if (levels > max)
			max = levels;
	}
	return max;
}

long btree_count(BTREE *node)
{
	BTREE_POS i;
	long count=0;

	if (node == NULL)
		return 0;

	for (i=0; i <= node->count; ++i) {
		if (BTREE_POS_STORED(node, i))
			++count;
		count += btree_count(node->ptr[i]);
	}

	return count;
}

void btree_display(BTREE *node, int only_level)
{
	BTREE_POS i;
	int level=1;
	char s[12];

	if (node == NULL || node->count == 0) {
		printk("EMPTY\n");
		return;
	}

	for (i=0; i < node->count; ++i) {
		if (node->ptr[i] != NULL) {
			btree_display(node->ptr[i], only_level-1);
		}
		sprintf(s, "%ld ", (long)node->key[i]);
		if (level != only_level)
			memset(s, ' ', strlen(s));
		printk("%s", s);
	}
	if (node->ptr[i] != NULL) {
		btree_display(node->ptr[i], only_level-1);
	}
}

void btree_display2(BTREE *node, int level)
{
	BTREE_POS i, j;

	if (node == NULL || node->count == 0) {
		printk("EMPTY\n");
		return;
	}

	for (i=0; i < node->count; ++i) {
		if (node->ptr[i] != NULL) {
			btree_display2(node->ptr[i], level+1);
		}
		for (j=0; j<level*6; ++j)
			printk(" ");
		printk("%5ld%c\n", (long)node->key[i],
			BTREE_POS_STORED(node, i) ? ' ' : '*');
	}
	if (node->ptr[i] != NULL) {
		btree_display2(node->ptr[i], level+1);
	}
}

void btree_free(BTREE *node)
{
	BTREE_POS i;

	if ((node == NULL) || (node->ptr == NULL))
		return;
	
	for (i = 0; i < node->count; ++ i)
	{
		if (node->key && BTREE_POS_STORED(node, i) && (node->key[i]))
		{
			FREE(node->key[i]);
			node->key[i] = NULL;
		}
		if (node->ptr[i])
		{
			btree_free(node->ptr[i]);
			node->ptr[i] = NULL;
		}
	}
	if (node->ptr[i])
	{
		btree_free(node->ptr[i]);
		node->ptr[i] = NULL;
	}

	if (node->key)
	{
		FREE(node->key);
		node->key = NULL;
	}
	FREE(node->ptr);
	node->ptr = NULL;
	FREE(node);
}

void btree_check(BTREE *node)
{
	BTREE_POS i, count;

	if (node == NULL || node->count == 0)
		return;


	for (i=count=0; i <= node->count; ++i) {
		if (i < node->count && BTREE_POS_STORED(node, i))
			++count;
		if (node->ptr[i] != NULL) {
			btree_check(node->ptr[i]);
		}
	}


}
//mahui01@wind-mobi.com 2018/9/17 add for lenovo CSDK end