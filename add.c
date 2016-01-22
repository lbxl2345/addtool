#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "elf.h"
/**file struct//
sgot_offset
jumptable
sgottable
***************/
int jumpgot_write(int n, FILE* fp)
{
	//file starts with the offsets of jump and sgot
	uint8_t *jump = malloc(JUMP_SIZE * n * sizeof(uint8_t));
	uint8_t *sgot = malloc(SGOT_SIZE * n * sizeof(uint8_t));
	struct js_header jshdr;
	jshdr.jump_size = JUMP_SIZE * n;
	jshdr.sgot_size = SGOT_SIZE * n;
	uint32_t page_size = getpagesize();
	uint32_t zero_size = page_size - (sizeof(jshdr) + jshdr.jump_size)%page_size;
	uint8_t *zero = malloc(zero_size * sizeof(uint8_t));
	memset(zero , 0, zero_size);
	printf("%d\n", zero_size);
	jshdr.zero_size = zero_size;
	jshdr.jump_off = sizeof(jshdr);
	jshdr.sgot_off = sizeof(jshdr) + jshdr.jump_size + zero_size;
	printf("jump_off%d\n", jshdr.jump_off);
	printf("sgot_off%d\n", jshdr.sgot_off);
	for(int i = 0; i < n; i++)
	{
		jumpgot_write_n(i, jump + i * JUMP_SIZE, jshdr);
	}
	memset(sgot, 0, SGOT_SIZE * n);
	fwrite(&jshdr, sizeof(jshdr), 1, fp);
	fwrite(jump, JUMP_SIZE * n, 1, fp);
	fwrite(zero, zero_size, 1, fp);
	fwrite(sgot, SGOT_SIZE * n, 1, fp);
	free(zero);
}
int jumpgot_write_n(int i, uint8_t *jump, struct js_header jshdr)
{
	int pos = 0;
	uint32_t offset = (jshdr.sgot_off + SGOT_SIZE * i) - (jshdr.jump_off + JUMP_SIZE * (i + 1));
	jump[pos++] = 0xff;
	jump[pos++] = 0x25;
	jump[pos++] = ((uint32_t)offset>>0) & 0xff;
	jump[pos++] = ((uint32_t)offset>>8) & 0xff;
	jump[pos++] = ((uint32_t)offset>>16) & 0xff;
	jump[pos++] = ((uint32_t)offset>>24) & 0xff;
}
int main(int argc, char* argv[])
{
	FILE *fp, *fa, *fw;
	fp = fopen(argv[1],"r");
	if(fp == NULL)
	{
		printf("can not open file %s\n", argv);
		return -1;
	}
	char *jumpgot_name = malloc(strlen(".jumpgot") + strlen(argv[1]) + 1);
	if(jumpgot_name == NULL)	
		return -1;
	strcpy(jumpgot_name, argv[1]);
	strcat(jumpgot_name, ".jumpgot");
	printf("%s\n", jumpgot_name);
	//jumpgot file name is file.jumpgot
	fw = fopen(jumpgot_name,"wb+");
	if(fw == NULL)
	{
		printf("can not create jumpgot file\n");
		return -1;
	}
	free(jumpgot_name);
	//get program elf header
	Elf64_Ehdr *ehdr = malloc(sizeof(Elf64_Ehdr));
	fread(ehdr, sizeof(Elf64_Ehdr),1,fp);
	//get program section header table
	Elf64_Shdr *shdr = malloc(sizeof(Elf64_Shdr) * ehdr->e_shnum);
	fseek(fp, ehdr->e_shoff, SEEK_SET);
	fread(shdr, sizeof(Elf64_Shdr), ehdr->e_shnum, fp);
	//get shstrn table section
	Elf64_Shdr *shstrn = shdr + ehdr->e_shstrndx;
	char *shstrtab = malloc(shstrn->sh_size);
	fseek(fp, shstrn->sh_offset, SEEK_SET);
	fread(shstrtab, shstrn->sh_size, 1,fp);
	Elf64_Shdr *sh_temp = shdr;
	for(int i = 0; i < ehdr->e_shnum; i++)
	{
		if(strcmp(shstrtab + sh_temp->sh_name, ".plt")==0)
		{
			printf("test success\n");
			break;
		}
		sh_temp++;
	}
	printf("plt size:%d\n",(unsigned)sh_temp->sh_size);
	int pltnum = sh_temp->sh_size/16 - 1;
	jumpgot_write(pltnum, fw);
	fclose(fp);
	fclose(fw);
	return 0;
}
