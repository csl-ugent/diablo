#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <diabloobject.h>
#include <diabloar.h>
#include <diabloelf.h>


#define MAX_NR_SECTIONS	10000


void RewriteOneElf(FILE * fp, char * name, t_uint32 filesize)
{
	Elf32_Ehdr hdr;
	Elf32_Shdr shdr;
	Elf32_Shdr strhdr;
	char * strtbl;
	int tel,tel2;
	char * names[MAX_NR_SECTIONS];
	t_uint32 offsets[MAX_NR_SECTIONS];
	t_bool dupsfound;

	t_uint32 fpos = ftell(fp);
	int n=0;

	VERBOSE(0, ("Processing %s", name));

	if (1!=fread(&hdr,sizeof(Elf32_Ehdr),1,fp))
	{
		FATAL(("Could not read file header of %s\nFile is probably truncated",name));
	}

	if ((hdr.e_ident[EI_MAG0]!=ELFMAG0) || (hdr.e_ident[EI_MAG1]!=ELFMAG1) || (hdr.e_ident[EI_MAG2]!=ELFMAG2) || (hdr.e_ident[EI_MAG3]!=ELFMAG3))
	{
		FATAL(("ELF Magic of %s is wrong\nFile is probably not an ELF object file", name));
	}

	if (hdr.e_shnum >= MAX_NR_SECTIONS)
	{
		FATAL(("This tool currently only supports %d sections per object file. This object has %d sections. Increase MAX_NR_SECTIONS and recompile.\n", MAX_NR_SECTIONS, hdr.e_shnum));
	}


	if (!filesize)
	{
	  fseek(fp, 0, SEEK_END);
	  filesize = ftell(fp);
	}

	VERBOSE(0, ("File is %d bytes large", filesize));

	fseek(fp,fpos + hdr.e_shoff+sizeof(Elf32_Shdr)*hdr.e_shstrndx,SEEK_SET);

	if (1!=fread(&strhdr,sizeof(Elf32_Shdr),1,fp))
	{
		FATAL(("Could not read string table of %s\nFile is probably truncated\n",name));
	}

	VERBOSE(0, ("String table is at %d and %d bytes large", strhdr.sh_offset, strhdr.sh_size));
	fseek(fp,fpos + strhdr.sh_offset,SEEK_SET);
	strtbl=(char *) malloc(strhdr.sh_size);
	if (!strtbl) 
	{ 
		printf("Could not allocate space for string table\n");
		exit(0);
	}
	if (strhdr.sh_size!=fread(strtbl,1,strhdr.sh_size,fp))
	{
		printf("Could not read string table of %s\nFile is probably truncated\n",name);
		exit(0);
	}
	fseek(fp,fpos + hdr.e_shoff,SEEK_SET);
	for (tel=0; tel<hdr.e_shnum; tel++)
	{
		if (1!=fread(&shdr,sizeof(Elf32_Shdr),1,fp))
		{
			printf("Could not read section header %d of %s\nFile is probably truncated\n",tel,name);
			exit(0);
		}

		names[tel] = malloc(strlen(strtbl+shdr.sh_name)+1);
		offsets[tel] = shdr.sh_name;
		strcpy(names[tel],strtbl+shdr.sh_name);  

	}

	/* First pass */

	do
	{
		dupsfound=FALSE;
	for (tel=0; tel<hdr.e_shnum; tel++)
	{
		for (tel2=tel+1; tel2<hdr.e_shnum; tel2++)
		{
			if (strncmp(names[tel],".rel.",5)==0) continue;
			if (strcmp(names[tel],names[tel2])==0)
			{
				ASSERT(offsets[tel] != offsets[tel2], ("section header string table compaction occured: sections %s (%d) and %s (%d) use the same section header string table entry (%x). We need a full rewriter to handle this. This is currently not implemented!", names[tel], tel, names[tel2], tel2, offsets[tel]));
				n++;
				printf("%d %d\n",n,((int) log10(n)));
				sprintf(names[tel2]+strlen(names[tel2])-((int) log10(n))-1,"%d",n);
				printf("In %s: Section %d and %d: %s -> %s\n", name,tel,tel2,names[tel],names[tel2]);
				dupsfound=TRUE;
			}
		}
	}
	}
	while(dupsfound);

	fseek(fp,fpos + hdr.e_shoff,SEEK_SET);
	for (tel=0; tel<hdr.e_shnum; tel++)
	{
		if (1!=fread(&shdr,sizeof(Elf32_Shdr),1,fp))
		{
			printf("Could not read section header %d of %s\nFile is probably truncated\n",tel,name);
			exit(0);
		}

		strcpy(strtbl+shdr.sh_name,names[tel]);  
	}


	fseek(fp,fpos + strhdr.sh_offset,SEEK_SET);
	if (strhdr.sh_size!=fwrite(strtbl,1,strhdr.sh_size,fp))
	{
		perror("Hello");
		printf("Could not write string table of %s\nFile is probably truncated\n",name);
		exit(0);
	}

}

void walker(void * node, void *extra)
{
	FILE * fp = extra;
	t_archive_object_hash_table_node * ao = node;
	printf("%p\n", fp);
	printf("%s %d\n", (t_string) HASH_TABLE_NODE_KEY(&ao->node), ao->fpos );
	fseek(fp, ao->fpos, SEEK_SET);
	RewriteOneElf(fp, (t_string) HASH_TABLE_NODE_KEY(&ao->node), ao->size);
}



int 
main(int argc, char ** argv)
{
	FILE * fp;
	t_archive * arch;

	DiabloObjectInit (argc, argv);
	DiabloArInit (argc, argv);



	if (argc!=2)
	{
		printf("Usage: %s <ads_object_or_library_file_to_patch>\n",argv[0]);
		exit(0);
	}

	arch=ArchiveOpenCached(argv[1]);

	if (arch)
	{
		fp=fopen(argv[1],"r+");
		ASSERT(fp, ("Could not open %s for writing", argv[1]));
		HashTableWalk(arch->objects,walker,fp);

	}
	else
	{
		fp=fopen(argv[1],"r+");
		ASSERT(fp, ("Could not open %s for writing", argv[1]));
		RewriteOneElf(fp, argv[1], 0);
	}

	fclose(fp);

	return 0;
}
