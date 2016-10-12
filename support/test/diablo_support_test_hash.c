#include <diablosupport.h>
#include <string.h>

void StatusIo(t_uint32 id, t_string out)
{
	printf(out);
}

char *keys[] = {
	NULL,
	"Search",
	"seek",
	"and",
	"know",
	"how",
	"this",
	"foul",
	"murder",
	"comes",
	"-----",
	"Who",
	"seeks",
	"and",
	"will",
	"not",
	"take",
	"when",
	"once",
	"'tis",
	"offer'd",
	"Shall",
	"never",
	"find",
	"it",
	"more",
	"-----",
	"Search",
	"seek",
	"and",
	"know",
	"how",
	"this",
	"foul",
	"murder",
	"comes",
	NULL
};

typedef struct _test1
{
	t_hash_table_node node;
	char *string2;
} hntest1;

typedef struct _test2
{
	char *string2;
	t_hash_table_node node;
} hntest2;

	void
SP (void *in, void *ea)
{
	hntest1 *p = in;
	printf ("p->string=%s\n", p->string2);
}

	void
SP2 (void *in, void *ea)
{
	hntest2 *p = in;
	printf ("p->string=%s\n", p->string2);
}

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

int main()
{
	char **it = keys;
	hntest1 *insert;
	hntest2 *insert2;
	t_hash_table *test1;


	IoHandlerAdd(E_STATUS,StatusIo);
	IoHandlerAdd(E_FATAL,StatusIo);
	IoModifierAdd('%',0,NULL,IoModifierPrintf);



	printf ("Creating simple hash table\n");
	/* We use small hash table size to allow thourough testing */
	test1 = HashTableNew (3, 0, (t_hash_func) StringHash, (t_int32 (*) (void *, void *)) StringCmp, NULL);
	printf ("Done\n");

	it++;
	printf ("Inserting elements\n");
	do
	{
		insert = Malloc (sizeof (hntest1));
		printf ("   Insert: %s\n", *it);
		HASH_TABLE_NODE_SET_KEY(&insert->node,  *it);
		insert->string2 = StringDup (*it);
		HashTableInsert (test1, insert);
	}
	while (*(++it));

	it--;
	do
	{
		printf ("   Lookup: %s\n", *it);
		insert = HashTableLookup (test1, *it);
		printf ("   Verify: %s\n", insert->string2);
		if (strcmp (insert->string2, *it))
		{
			printf ("Failed!\n");
			exit (-1);
		}
	}
	while (*(--it));

	HashTableWalk (test1, SP, NULL);


	printf ("Prepending elements\n");
	printf ("Creating simple hash table\n");
	/* We use small hash table size to allow thourough testing */
	test1 = HashTableNew (3, 0, (t_hash_func) StringHash, (t_int32 (*) (void *, void *)) StringCmp, NULL);
	printf ("Done\n");

	it++;
	printf ("Inserting elements\n");
	do
	{
		insert = Malloc (sizeof (hntest1));
		printf ("   Prepend: %s\n", *it);
		HASH_TABLE_NODE_SET_KEY(&insert->node,  *it);
		insert->string2 = StringDup (*it);
		HashTablePrepend (test1, insert);
	}
	while (*(++it));

	it--;
	do
	{
		printf ("   Lookup: %s\n", *it);
		insert = HashTableLookup (test1, *it);
		printf ("   Verify: %s\n", insert->string2);
		if (strcmp (insert->string2, *it))
		{
			printf ("Failed!\n");
			exit (-1);
		}
	}
	while (*(--it));

	HashTableWalk (test1, SP, NULL);




	printf ("Creating displaced hash table\n");
	/* We use small hash table size to allow thourough testing */
	test1 =
		HashTableNew (3, offsetof (hntest2, node), (t_hash_func) StringHash, (t_int32 (*) (void *, void *)) StringCmp, NULL);
	printf ("Done\n");

	it++;
	printf ("Inserting elements\n");
	do
	{
		insert2 = Malloc (sizeof (hntest2));
		printf ("   Insert: %s\n", *it);
		HASH_TABLE_NODE_SET_KEY(&(insert2->node),  *it);
		insert2->string2 = StringDup (*it);
		HashTableInsert (test1, insert2);
	}
	while (*(++it));

	it--;
	do
	{
		printf ("   Lookup: %s\n", *it);
		insert2 = HashTableLookup (test1, *it);
		printf ("   Verify: %s\n", insert2->string2);
		if (strcmp (insert2->string2, *it))
		{
			printf ("Failed!\n");
			exit (-1);
		}
	}
	while (*(--it));

	HashTableWalk (test1, SP2, NULL);

	printf ("Creating displaced hash table\n");
	/* We use small hash table size to allow thourough testing */
	test1 =
		HashTableNew (3, offsetof (hntest2, node), (t_hash_func) StringHash, (t_int32 (*) (void *, void *)) StringCmp, NULL);
	printf ("Done\n");

	it++;
	printf ("Prepend elements\n");
	do
	{
		insert2 = Malloc (sizeof (hntest2));
		printf ("   Prepend: %s\n", *it);
		HASH_TABLE_NODE_SET_KEY(&(insert2->node),  *it);
		insert2->string2 = StringDup (*it);
		HashTablePrepend (test1, insert2);
	}
	while (*(++it));

	it--;
	do
	{
		printf ("   Lookup: %s\n", *it);
		insert2 = HashTableLookup (test1, *it);
		printf ("   Verify: %s\n", insert2->string2);
		if (strcmp (insert2->string2, *it))
		{
			printf ("Failed!\n");
			exit (-1);
		}
	}
	while (*(--it));

	HashTableWalk (test1, SP2, NULL);


	return 0;
}
