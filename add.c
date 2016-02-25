#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "elf.h"
/**file struct//
jshdr
jumptable
sgottable
backtable
***************/
int jumpgot_write(int n, FILE* fp)
{
	//file starts with the offsets of jump and sgot
	uint8_t *jump_resolve = malloc(JUMP_RESOLVE_SIZE * sizeof(uint8_t));
	uint8_t *jump = malloc(JUMP_SIZE * n * sizeof(uint8_t));
	uint8_t *sgot = malloc(SGOT_SIZE * (n + 1) * sizeof(uint8_t));
	uint8_t *back = malloc(BACK_SIZE * sizeof(uint8_t));
	struct js_header jshdr;
	jshdr.jump_resolve_size = JUMP_RESOLVE_SIZE;
	jshdr.jump_size = JUMP_SIZE * n;
	jshdr.sgot_size = SGOT_SIZE * (n + 1);
	jshdr.back_size = BACK_SIZE;
	uint32_t page_size = getpagesize();
	uint32_t zero_size = page_size - (sizeof(jshdr) + jshdr.jump_resolve_size + jshdr.jump_size)%page_size;
	uint8_t *zero = malloc(zero_size * sizeof(uint8_t));
	memset(zero , 0, zero_size);
	printf("%d\n", zero_size);
	jshdr.zero_size = zero_size;
	//get offset of part
	jshdr.jump_resolve_off = sizeof(jshdr);
	jshdr.jump_off = sizeof(jshdr) + jshdr.jump_resolve_size;
	jshdr.sgot_off = sizeof(jshdr) + jshdr.jump_resolve_size + jshdr.jump_size + zero_size;
	jshdr.back_off = jshdr.sgot_off + jshdr.sgot_size;
	printf("back_off%d\n", jshdr.back_off);
	printf("jump_off%d\n", jshdr.jump_off);
	printf("sgot_off%d\n", jshdr.sgot_off);
	jumpgot_write_resolve(jump_resolve, jshdr);
	for(int i = 0; i < n; i++)
	{
		printf("handle No:%d\n", i);
		jumpgot_write_n(i, jump + i * JUMP_SIZE, jshdr);
	}
	memset(sgot, 0, SGOT_SIZE * n);
	jumpgot_write_back(back);
	fwrite(&jshdr, sizeof(jshdr), 1, fp);
	fwrite(jump_resolve, JUMP_RESOLVE_SIZE, 1, fp);
	fwrite(jump, JUMP_SIZE * n, 1, fp);
	fwrite(zero, zero_size, 1, fp);
	fwrite(sgot, SGOT_SIZE * (n + 1), 1, fp);
	fwrite(back, BACK_SIZE, 1, fp);
	free(zero);
}
int jumpgot_write_back(uint8_t *back)
{
	int pos = 0;
	
	back[pos++] = 0x50;	//push rax
	back[pos++] = 0x51;	//push rcx
	
	

	back[pos++] = 0xb8;	//mov eax, 0
	back[pos++] = 0x00;
	back[pos++] = 0x00;
	back[pos++] = 0x00;
	back[pos++] = 0x00;
	back[pos++] = 0xb9;	//mov ecx, 1
	back[pos++] = 0x01;
	back[pos++] = 0x00;
	back[pos++] = 0x00;
	back[pos++] = 0x00;
	back[pos++] = 0x0f;	//vmfunc
	back[pos++] = 0x01;
	back[pos++] = 0xd4;
	
	//nop
	/*
	back[pos++] = 0x90;
	back[pos++] = 0x90;
	back[pos++] = 0x90;
	back[pos++] = 0x90;
	back[pos++] = 0x90;
	back[pos++] = 0x90;
	back[pos++] = 0x90;
	back[pos++] = 0x90;
	back[pos++] = 0x90;
	back[pos++] = 0x90;
	back[pos++] = 0x90;
	back[pos++] = 0x90;
	back[pos++] = 0x90;
	*/
	
	/*for test length:38*/
	back[pos++] = 0x57;	//push rdi
	back[pos++] = 0x56;	//push rsi
	back[pos++] = 0x52;	//push rdx
	back[pos++] = 0x6a;	//push 0x40
	back[pos++] = 0x40;
	back[pos++] = 0x48;	//mov rax, 0x1
	back[pos++] = 0xc7;
	back[pos++] = 0xc0;
	back[pos++] = 0x01;
	back[pos++] = 0x00;
	back[pos++] = 0x00;
	back[pos++] = 0x00;
	back[pos++] = 0x48;	//mov rdi, 0x1
	back[pos++] = 0xc7;
	back[pos++] = 0xc7;
	back[pos++] = 0x01;
	back[pos++] = 0x00;
	back[pos++] = 0x00;
	back[pos++] = 0x00;
	back[pos++] = 0x48;	//mov rsi, rsp
	back[pos++] = 0x89;
	back[pos++] = 0xe6;
	back[pos++] = 0x48;	//mov rdx, 0x1
	back[pos++] = 0xc7;
	back[pos++] = 0xc2;
	back[pos++] = 0x01;
	back[pos++] = 0x00;
	back[pos++] = 0x00;
	back[pos++] = 0x00;
	back[pos++] = 0x0f;	//syscall
	back[pos++] = 0x05;
	back[pos++] = 0x48;	//add rsp, 0x8
	back[pos++] = 0x83;
	back[pos++] = 0xc4;
	back[pos++] = 0x08;
	back[pos++] = 0x5a;	//pop rdx
	back[pos++] = 0x5e;	//pop rsi
	back[pos++] = 0x5f;	//pop rdi
	/*ret*/
	back[pos++] = 0x59;	//pop rcx
	back[pos++] = 0x58;	//pop rax
	back[pos++] = 0xc3;	//ret
}
int jumpgot_write_resolve(uint8_t *jump, struct js_header jshdr)
{
	int pos = 0;
	uint32_t offset = jshdr.sgot_off - ( jshdr.jump_resolve_off + JUMP_RESOLVE_SIZE);
	uint32_t back_offset = jshdr.back_off -  (jshdr. jump_resolve_off  + JUMP_INSN_OFF);

	jump[pos++] = 0x50;	//push rax
	jump[pos++] = 0x50;	//push rax
	jump[pos++] = 0x51;	//push rcx
	
	/*for test length:38*/
	jump[pos++] = 0x57;	//push rdi
	jump[pos++] = 0x56;	//push rsi
	jump[pos++] = 0x52;	//push rdx
	jump[pos++] = 0x6a;	//push 0x40
	jump[pos++] = 0x26;
	jump[pos++] = 0x48;	//mov rax, 0x1
	jump[pos++] = 0xc7;
	jump[pos++] = 0xc0;
	jump[pos++] = 0x01;
	jump[pos++] = 0x00;
	jump[pos++] = 0x00;
	jump[pos++] = 0x00;
	jump[pos++] = 0x48;	//mov rdi, 0x1
	jump[pos++] = 0xc7;
	jump[pos++] = 0xc7;
	jump[pos++] = 0x01;
	jump[pos++] = 0x00;
	jump[pos++] = 0x00;
	jump[pos++] = 0x00;
	jump[pos++] = 0x48;	//mov rsi, rsp
	jump[pos++] = 0x89;
	jump[pos++] = 0xe6;
	jump[pos++] = 0x48;	//mov rdx, 0x1
	jump[pos++] = 0xc7;
	jump[pos++] = 0xc2;
	jump[pos++] = 0x01;
	jump[pos++] = 0x00;
	jump[pos++] = 0x00;
	jump[pos++] = 0x00;
	jump[pos++] = 0x0f;	//syscall
	jump[pos++] = 0x05;
	jump[pos++] = 0x48;	//add rsp, 0x8
	jump[pos++] = 0x83;
	jump[pos++] = 0xc4;
	jump[pos++] = 0x08;
	jump[pos++] = 0x5a;	//pop rdx
	jump[pos++] = 0x5e;	//pop rsi
	jump[pos++] = 0x5f;	//pop rdi
	//switch ept, length: 13
	jump[pos++] = 0xb8;	//mov eax,0
	jump[pos++] = 0x00;	
	jump[pos++] = 0x00;	
	jump[pos++] = 0x00;
	jump[pos++] = 0x00;
	jump[pos++] = 0xb9;	//mov ecx,0 
	jump[pos++] = 0x00;
	jump[pos++] = 0x00;
	jump[pos++] = 0x00;
	jump[pos++] = 0x00;
	jump[pos++] = 0x0f;	//vmfunc
	jump[pos++] = 0x01;
	jump[pos++] = 0xd4;
	// calculate ret address, length: 18
	jump[pos++] = 0x48;	//lea rax, [rip] 7 loc
	jump[pos++] = 0x8d;
	jump[pos++] = 0x05;
	jump[pos++] = 0x00;
	jump[pos++] = 0x00;
	jump[pos++] = 0x00;
	jump[pos++] = 0x00;
	jump[pos++] = 0x48; //add rax, offset of back 
	jump[pos++] = 0x05;
	jump[pos++] = ((uint32_t)back_offset>>0) & 0xff;
	jump[pos++] = ((uint32_t)back_offset>>8) & 0xff;
	jump[pos++] = ((uint32_t)back_offset>>16) & 0xff;
	jump[pos++] = ((uint32_t)back_offset>>24) & 0xff;
	jump[pos++] = 0x48;	//mov [rsp + 16], rax
	jump[pos++] = 0x89;
	jump[pos++] = 0x44;
	jump[pos++] = 0x24;
	jump[pos++] = 0x10;	
	//recover the stack length: 30
	jump[pos++] = 0x48;	//mov rax, [rsp + 16]
	jump[pos++] = 0x8b;
	jump[pos++] = 0x44;
	jump[pos++] = 0x24;
	jump[pos++] = 0x10;

	jump[pos++] = 0x48;	//mov rcx, [rsp + 24]
	jump[pos++] = 0x8b;
	jump[pos++] = 0x4c;
	jump[pos++] = 0x24;
	jump[pos++] = 0x18;

	jump[pos++] = 0x48;	//mov [rsp + 16], rcx
	jump[pos++] = 0x89;
	jump[pos++] = 0x4c;
	jump[pos++] = 0x24;
	jump[pos++] = 0x10;

	jump[pos++] = 0x48;	//mov rcx, [rsp + 32]
	jump[pos++] = 0x8b;
	jump[pos++] = 0x4c;
	jump[pos++] = 0x24;
	jump[pos++] = 0x20;

	jump[pos++] = 0x48;	//mov [rsp + 24], rcx
	jump[pos++] = 0x89;
	jump[pos++] = 0x4c;
	jump[pos++] = 0x24;
	jump[pos++] = 0x18;

	jump[pos++] = 0x48;	//mov [rsp + 32], rax
	jump[pos++] = 0x89;
	jump[pos++] = 0x44;
	jump[pos++] = 0x24;
	jump[pos++] = 0x20;

	jump[pos++] = 0x59;	//pop rcx
	jump[pos++] = 0x58; //pop rax
	//jump[pos++] = 0x58; //pop rax
	
	jump[pos++] = 0xff;	//jmp got address 6 loc
	jump[pos++] = 0x25;
	jump[pos++] = ((uint32_t)offset>>0) & 0xff;
	jump[pos++] = ((uint32_t)offset>>8) & 0xff;
	jump[pos++] = ((uint32_t)offset>>16) & 0xff;
	jump[pos++] = ((uint32_t)offset>>24) & 0xff;
}
int jumpgot_write_n(int i, uint8_t *jump, struct js_header jshdr)
{
	int pos = 0;
	uint32_t offset = (jshdr.sgot_off + SGOT_SIZE * (i + 1)) - (jshdr.jump_off + JUMP_SIZE * (i + 1));
	uint32_t back_offset = jshdr.back_off - (jshdr.jump_off + JUMP_SIZE * i + INSN_OFF);
	jump[pos++] = 0x50;	//push rax
	jump[pos++] = 0x50;	//push rax
	jump[pos++] = 0x51;	//push rcx
	
	/*for test length:38*/
	jump[pos++] = 0x57;	//push rdi
	jump[pos++] = 0x56;	//push rsi
	jump[pos++] = 0x52;	//push rdx
	jump[pos++] = 0x6a;	//push 0x40
	jump[pos++] = 0x21
	;
	jump[pos++] = 0x48;	//mov rax, 0x1
	jump[pos++] = 0xc7;
	jump[pos++] = 0xc0;
	jump[pos++] = 0x01;
	jump[pos++] = 0x00;
	jump[pos++] = 0x00;
	jump[pos++] = 0x00;
	jump[pos++] = 0x48;	//mov rdi, 0x1
	jump[pos++] = 0xc7;
	jump[pos++] = 0xc7;
	jump[pos++] = 0x01;
	jump[pos++] = 0x00;
	jump[pos++] = 0x00;
	jump[pos++] = 0x00;
	jump[pos++] = 0x48;	//mov rsi, rsp
	jump[pos++] = 0x89;
	jump[pos++] = 0xe6;
	jump[pos++] = 0x48;	//mov rdx, 0x1
	jump[pos++] = 0xc7;
	jump[pos++] = 0xc2;
	jump[pos++] = 0x01;
	jump[pos++] = 0x00;
	jump[pos++] = 0x00;
	jump[pos++] = 0x00;
	jump[pos++] = 0x0f;	//syscall
	jump[pos++] = 0x05;
	jump[pos++] = 0x48;	//add rsp, 0x8
	jump[pos++] = 0x83;
	jump[pos++] = 0xc4;
	jump[pos++] = 0x08;
	jump[pos++] = 0x5a;	//pop rdx
	jump[pos++] = 0x5e;	//pop rsi
	jump[pos++] = 0x5f;	//pop rdi

	jump[pos++] = 0xb8;	//mov eax,0
	jump[pos++] = 0x00;	
	jump[pos++] = 0x00;	
	jump[pos++] = 0x00;
	jump[pos++] = 0x00;
	jump[pos++] = 0xb9;	//mov ecx,0 
	jump[pos++] = 0x00;
	jump[pos++] = 0x00;
	jump[pos++] = 0x00;
	jump[pos++] = 0x00;
	jump[pos++] = 0x0f;	//vmfunc
	jump[pos++] = 0x01;
	jump[pos++] = 0xd4;
	

	//nop
	/*
	jump[pos++] = 0x90;
	jump[pos++] = 0x90;
	jump[pos++] = 0x90;
	jump[pos++] = 0x90;
	jump[pos++] = 0x90;
	jump[pos++] = 0x90;
	jump[pos++] = 0x90;
	jump[pos++] = 0x90;
	jump[pos++] = 0x90;
	jump[pos++] = 0x90;
	jump[pos++] = 0x90;
	jump[pos++] = 0x90;
	jump[pos++] = 0x90;
	*/
	jump[pos++] = 0x48;	//lea rax, [rip] 7 loc
	jump[pos++] = 0x8d;
	jump[pos++] = 0x05;
	jump[pos++] = 0x00;
	jump[pos++] = 0x00;
	jump[pos++] = 0x00;
	jump[pos++] = 0x00;
	jump[pos++] = 0x48; //add rax, offset of back 12 loc
	jump[pos++] = 0x05;
	jump[pos++] = ((uint32_t)back_offset>>0) & 0xff;
	jump[pos++] = ((uint32_t)back_offset>>8) & 0xff;
	jump[pos++] = ((uint32_t)back_offset>>16) & 0xff;
	jump[pos++] = ((uint32_t)back_offset>>24) & 0xff;
	jump[pos++] = 0x48;	//mov [rsp + 16], rax
	jump[pos++] = 0x89;
	jump[pos++] = 0x44;
	jump[pos++] = 0x24;
	jump[pos++] = 0x10;	
	jump[pos++] = 0x59;	//pop rcx
	jump[pos++] = 0x58; //pop rax
	//jump[pos++] = 0x58; //pop rax

	jump[pos++] = 0xff;	//jmp got address 6 loc
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
		printf("can not open file %s\n", argv[1]);
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
	int pltnum = sh_temp->sh_size/16;
	jumpgot_write(pltnum, fw);
	fclose(fp);
	fclose(fw);
	return 0;
}
