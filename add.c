//branch cy
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
int jumpgot_write(int n, FILE* fp, FILE* fo, Elf64_Shdr *shdr, Elf64_Ehdr *ehdr)
{	
	//file modify part
	//get entry offset
	struct js_header jshdr;
	Elf64_Xword old_entry = ehdr->e_entry;
	Elf64_Shdr *p_shdr = shdr;
	Elf64_Xword file_end = 0;
	Elf64_Xword entry_off = 0;
	Elf64_Xword entry_addr = 0;
	//calculate file end
	for(int i = 0; i < ehdr->e_shnum; i++)
	{
		printf("section%d\tfilesize:%lx,\t offset:%lx\n", i, (unsigned long)p_shdr->sh_size,  (unsigned long)p_shdr->sh_offset);
		if(entry_off < p_shdr->sh_offset + p_shdr->sh_size)
			entry_off =  p_shdr->sh_offset + p_shdr->sh_size;
		p_shdr++;
	}
	file_end = entry_off;
	printf("end of file:%lx\n", (unsigned long)file_end);
	//calculate new entry filled zero size
	uint32_t page_size = getpagesize();
	uint32_t zero_entry_size = page_size - entry_off%page_size;
	entry_off = entry_off+ page_size - entry_off%page_size;
	jshdr.entry_off = entry_off;
	printf("new_page is :%lx\n", entry_off);
	//calculate entry v addr
	Elf64_Phdr *phdr = malloc(sizeof(Elf64_Phdr) * ehdr->e_phnum);
	fseek(fo, ehdr->e_phoff, SEEK_SET);
	fread(phdr, sizeof(Elf64_Phdr), ehdr->e_phnum, fo);
	Elf64_Phdr *p_phdr = phdr; 
	//go through the segments
	for(int i = 0; i < ehdr->e_phnum; i++)
	{
		printf("%d\n", i);
		printf("%lx, %lx\n", p_phdr->p_vaddr, p_phdr->p_memsz);
		if(entry_addr < p_phdr->p_vaddr + p_phdr->p_memsz)
			entry_addr=  p_phdr->p_vaddr + p_phdr->p_memsz; 
		p_phdr++;
		printf("%lx\n", (unsigned long)entry_addr);
	}
	entry_addr = (entry_addr / page_size + 1) * page_size;
	jshdr.entry_addr = entry_addr;
	ehdr->e_entry = entry_addr;
	printf("entry_off is :%lx\n", entry_addr);
	//calculate the offset in instructions
	uint32_t cal_offset = ~(entry_addr + ADD_SIZE - old_entry) + 1;
	//delete rewrite entry
	//rewrite_entry(fo, file_end, ehdr, zero_entry_size, cal_offset);

	//file starts with the offsets of jump and sgot
	uint8_t *jump_resolve = malloc(JUMP_RESOLVE_SIZE * sizeof(uint8_t));
	uint8_t *jump = malloc(JUMP_SIZE * n * sizeof(uint8_t));
	uint8_t *sgot = malloc(SGOT_SIZE * (n + 2) * sizeof(uint8_t));
	uint8_t *back = malloc(BACK_SIZE * sizeof(uint8_t));
	
	jshdr.jump_resolve_size = JUMP_RESOLVE_SIZE;
	jshdr.jump_size = JUMP_SIZE * n;
	jshdr.sgot_size = SGOT_SIZE * (n + 2);
	jshdr.back_size = BACK_SIZE;
	uint32_t zero_size = page_size - (sizeof(jshdr) + jshdr.jump_resolve_size + jshdr.jump_size + jshdr.back_size)%page_size;
	uint8_t *zero = malloc(zero_size * sizeof(uint8_t));
	memset(zero , 0, zero_size);
	printf("%d\n", zero_size);
	uint32_t sgot_zero_size = page_size - jshdr.sgot_size%page_size;
	uint8_t *sgot_zero = malloc(sgot_zero_size * sizeof(uint8_t)); 
	memset(sgot_zero, 0, sgot_zero_size);
	uint8_t *stack = malloc(page_size * sizeof(uint8_t));
	memset(stack, 0, page_size);
	jshdr.zero_size = zero_size;
	//get offset of part
	jshdr.jump_resolve_off = sizeof(jshdr);
	jshdr.jump_off = sizeof(jshdr) + jshdr.jump_resolve_size;
	jshdr.sgot_off = sizeof(jshdr) + jshdr.jump_resolve_size + jshdr.jump_size + jshdr.back_size+ zero_size;
	jshdr.back_off = jshdr.jump_off + jshdr.jump_size;
	jshdr.check_off = jshdr.back_off - SGOT_SIZE;
	jshdr.stack_off = jshdr.sgot_off + jshdr.sgot_size + sgot_zero_size; 
	printf("back_off%x\n", jshdr.back_off);
	printf("jump_off%x\n", jshdr.jump_off);
	printf("sgot_off%x\n", jshdr.sgot_off);
	printf("stack_off%x\n",jshdr.stack_off);
	jumpgot_write_resolve(jump_resolve, jshdr);
	for(int i = 0; i < n; i++)
	{
		printf("handle No:%d\n", i);
		jumpgot_write_n(i, jump + i * JUMP_SIZE, jshdr);
	}
	memset(sgot, 0, SGOT_SIZE * n);
	jumpgot_write_back(back, jshdr);
	fwrite(&jshdr, sizeof(jshdr), 1, fp);
	fwrite(jump_resolve, JUMP_RESOLVE_SIZE, 1, fp);
	fwrite(jump, JUMP_SIZE * n, 1, fp);
	fwrite(back, BACK_SIZE, 1, fp);
	fwrite(zero, zero_size, 1, fp);
	fwrite(sgot, SGOT_SIZE * (n + 2), 1, fp);
	fwrite(sgot_zero, sgot_zero_size, 1, fp);
	fwrite(stack, page_size, 1, fp);

	free(zero);
}
int jumpgot_write_back(uint8_t *back, struct js_header jshdr)
{
	int pos = 0;
	uint32_t stack_offset = jshdr.stack_off - (jshdr.back_off +BACK_STACK_OFF); 
	
	back[pos++] = 0x50;	//push rax
	back[pos++] = 0x50;	//push rax
	back[pos++] = 0x53;	//push rbx
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
	// /*for test length:38*/
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
	// /*ret*/
	
	back[pos++] = 0x48;	//lea rcx, [rip + offset of stack]
	back[pos++] = 0x8d;
	back[pos++] = 0x0d;
	back[pos++] = ((uint32_t)stack_offset>>0) & 0xff;
	back[pos++] = ((uint32_t)stack_offset>>8) & 0xff;
	back[pos++] = ((uint32_t)stack_offset>>16) & 0xff;
	back[pos++] = ((uint32_t)stack_offset>>24) & 0xff;
	back[pos++] = 0x48;	//mov rax, [rcx]
	back[pos++] = 0x8b;
	back[pos++] = 0x01;
	back[pos++] = 0x48;	//add rax, rcx
	back[pos++] = 0x01;
	back[pos++] = 0xc8;
	back[pos++] = 0x48;	//mov rbx, [rax]
	back[pos++] = 0x8b;
	back[pos++] = 0x18;
	back[pos++] = 0x48;	//mov [rsp+0x18], rbx
	back[pos++] = 0x89;
	back[pos++] = 0x5c;
	back[pos++] = 0x24;
	back[pos++] = 0x18; 
	back[pos++] = 0x48;	//mov rax, 0x8
	back[pos++] = 0xc7;
	back[pos++] = 0xc0;
	back[pos++] = 0x08;
	back[pos++] = 0x00;
	back[pos++] = 0x00;
	back[pos++] = 0x00;
	back[pos++] = 0x48;	//sub [rcx], rax
	back[pos++] = 0x29;
	back[pos++] = 0x01;

	back[pos++] = 0x59;	//pop rcx
	back[pos++] = 0x5b;	//pop rbx
	back[pos++] = 0x58;	//pop rax
	back[pos++] = 0xc3;	//ret
}
int jumpgot_write_resolve(uint8_t *jump, struct js_header jshdr)
{
	int pos = 0;
	uint32_t offset = jshdr.sgot_off - ( jshdr.jump_resolve_off + JUMP_RESOLVE_SIZE);
	uint32_t back_offset = jshdr.back_off -  (jshdr. jump_resolve_off  + RESOLVE_INSN_OFF);
	uint32_t stack_offset = jshdr.stack_off - (jshdr.jump_resolve_off + RESOLVE_STACK_OFF);
	jump[pos++] = 0x50;	//push rax
	jump[pos++] = 0x53;	//push rbx
	jump[pos++] = 0x51;	//push rcx	
	/*for test length:38*/
	jump[pos++] = 0x57;	//push rdi
	jump[pos++] = 0x56;	//push rsi
	jump[pos++] = 0x52;	//push rdx
	jump[pos++] = 0x6a;	//push 0x40
	jump[pos++] = 0x23;
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
	jump[pos++] = 0xd4;	//stable:54

	jump[pos++] = 0x48;	//lea rcx, [rip + offset of stack]
	jump[pos++] = 0x8d;
	jump[pos++] = 0x0d;
	jump[pos++] = ((uint32_t)stack_offset>>0) & 0xff;
	jump[pos++] = ((uint32_t)stack_offset>>8) & 0xff;
	jump[pos++] = ((uint32_t)stack_offset>>16) & 0xff;
	jump[pos++] = ((uint32_t)stack_offset>>24) & 0xff;	
	jump[pos++] = 0x48;	//mov rax, 0x08
	jump[pos++] = 0xc7;
	jump[pos++] = 0xc0;
	jump[pos++] = 0x08;
	jump[pos++] = 0x00;
	jump[pos++] = 0x00;
	jump[pos++] = 0x00;
	jump[pos++] = 0x48;	//add [rcx], rax
	jump[pos++] = 0x01;
	jump[pos++] = 0x01;	
	jump[pos++] = 0x48;	//mov rax, [rcx]
	jump[pos++] = 0x8b;
	jump[pos++] = 0x01;
	jump[pos++] = 0x48;	//add rax, rcx
	jump[pos++] = 0x01;
	jump[pos++] = 0xc8;
	jump[pos++] = 0x48;	//mov rbx, [rsp+0x28]
	jump[pos++] = 0x8b;
	jump[pos++] = 0x5c;
	jump[pos++] = 0x24;
	jump[pos++] = 0x28;
	jump[pos++] = 0x48;	//mov [rax], rbx
	jump[pos++] = 0x89;
	jump[pos++] = 0x18;

	jump[pos++] = 0x48;	//lea rax, [rip + offset of back]
	jump[pos++] = 0x8d;
	jump[pos++] = 0x05;
	jump[pos++] = ((uint32_t)back_offset>>0) & 0xff;
	jump[pos++] = ((uint32_t)back_offset>>8) & 0xff;
	jump[pos++] = ((uint32_t)back_offset>>16) & 0xff;
	jump[pos++] = ((uint32_t)back_offset>>24) & 0xff;
	jump[pos++] = 0x48;	//mov [rsp+0x28], rax
	jump[pos++] = 0x89;
	jump[pos++] = 0x44;
	jump[pos++] = 0x24;
	jump[pos++] = 0x28;
	jump[pos++] = 0x59;	//pop rcx
	jump[pos++] = 0x5b;	//pop rbx
	jump[pos++] = 0x58; 	//pop rax

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
	uint32_t back_offset = jshdr.back_off - (jshdr.jump_off + JUMP_SIZE * i + JUMP_INSN_OFF);
	uint32_t stack_offset = jshdr.stack_off - (jshdr.jump_off + JUMP_SIZE * i + JUMP_STACK_OFF);
	
	
	jump[pos++] = 0x50;	//push rax
	jump[pos++] = 0x53;	//push rbx
	jump[pos++] = 0x51;	//push rcx	
	/*for test length:38*/
	jump[pos++] = 0x57;	//push rdi
	jump[pos++] = 0x56;	//push rsi
	jump[pos++] = 0x52;	//push rdx
	jump[pos++] = 0x6a;	//push 0x40
	jump[pos++] = 0x21;
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
	jump[pos++] = 0xd4;	//stable:54

	jump[pos++] = 0x48;	//lea rcx, [rip + offset of stack]
	jump[pos++] = 0x8d;
	jump[pos++] = 0x0d;
	jump[pos++] = ((uint32_t)stack_offset>>0) & 0xff;
	jump[pos++] = ((uint32_t)stack_offset>>8) & 0xff;
	jump[pos++] = ((uint32_t)stack_offset>>16) & 0xff;
	jump[pos++] = ((uint32_t)stack_offset>>24) & 0xff;	
	jump[pos++] = 0x48;	//mov rax, 0x08
	jump[pos++] = 0xc7;
	jump[pos++] = 0xc0;
	jump[pos++] = 0x08;
	jump[pos++] = 0x00;
	jump[pos++] = 0x00;
	jump[pos++] = 0x00;
	jump[pos++] = 0x48;	//add [rcx], rax
	jump[pos++] = 0x01;
	jump[pos++] = 0x01;	
	jump[pos++] = 0x48;	//mov rax, [rcx]
	jump[pos++] = 0x8b;
	jump[pos++] = 0x01;
	jump[pos++] = 0x48;	//add rax, rcx
	jump[pos++] = 0x01;
	jump[pos++] = 0xc8;
	jump[pos++] = 0x48;	//mov rbx, [rsp+0x18]
	jump[pos++] = 0x8b;
	jump[pos++] = 0x5c;
	jump[pos++] = 0x24;
	jump[pos++] = 0x18;
	jump[pos++] = 0x48;	//mov [rax], rbx
	jump[pos++] = 0x89;
	jump[pos++] = 0x18;

	jump[pos++] = 0x48;	//lea rax, [rip + offset of back]
	jump[pos++] = 0x8d;
	jump[pos++] = 0x05;
	jump[pos++] = ((uint32_t)back_offset>>0) & 0xff;
	jump[pos++] = ((uint32_t)back_offset>>8) & 0xff;
	jump[pos++] = ((uint32_t)back_offset>>16) & 0xff;
	jump[pos++] = ((uint32_t)back_offset>>24) & 0xff;
	jump[pos++] = 0x48;	//mov [rsp+0x18], rax
	jump[pos++] = 0x89;
	jump[pos++] = 0x44;
	jump[pos++] = 0x24;
	jump[pos++] = 0x18;
	jump[pos++] = 0x59;	//pop rcx
	jump[pos++] = 0x5b;	//pop rbx
	jump[pos++] = 0x58; 	//pop rax

	jump[pos++] = 0xff;	//jmp got address 6 loc
	jump[pos++] = 0x25;
	jump[pos++] = ((uint32_t)offset>>0) & 0xff;
	jump[pos++] = ((uint32_t)offset>>8) & 0xff;
	jump[pos++] = ((uint32_t)offset>>16) & 0xff;
	jump[pos++] = ((uint32_t)offset>>24) & 0xff;
}
int rewrite_entry(FILE *fp, Elf64_Xword file_end, Elf64_Ehdr *ehdr, uint32_t zero_size, uint32_t offset)
{
	printf("%lx\n",(unsigned long)offset);
	uint8_t *zero = malloc(zero_size * sizeof(uint8_t));
	fseek(fp, 0, SEEK_SET);
	fwrite( ehdr, sizeof(Elf64_Ehdr), 1, fp);
	fseek(fp, file_end, SEEK_SET);
	memset(zero , 0, zero_size);
	fwrite(zero, zero_size, 1, fp);
	uint8_t  *entry = malloc(ADD_SIZE * sizeof(uint8_t));
	int pos = 0;
	entry[pos++] = 0xb8;	//mov eax,0
	entry[pos++] = 0x00;	
	entry[pos++] = 0x00;	
	entry[pos++] = 0x00;	
	entry[pos++] = 0x00;	
	entry[pos++] = 0xb9;	//mov ecx,0 
	entry[pos++] = 0x00;
	entry[pos++] = 0x00;
	entry[pos++] = 0x00;
	entry[pos++] = 0x00;
	entry[pos++] = 0x0f;	//vmfunc
	entry[pos++] = 0x01;
	entry[pos++] = 0xd4;

	/*for test length:38*/
	entry[pos++] = 0x57;	//push rdi
	entry[pos++] = 0x56;	//push rsi
	entry[pos++] = 0x52;	//push rdx
	entry[pos++] = 0x6a;	//push 0x40
	entry[pos++] = 0x7e;
	entry[pos++] = 0x48;	//mov rax, 0x1
	entry[pos++] = 0xc7;
	entry[pos++] = 0xc0;
	entry[pos++] = 0x01;
	entry[pos++] = 0x00;
	entry[pos++] = 0x00;
	entry[pos++] = 0x00;
	entry[pos++] = 0x48;	//mov rdi, 0x1
	entry[pos++] = 0xc7;
	entry[pos++] = 0xc7;
	entry[pos++] = 0x01;
	entry[pos++] = 0x00;
	entry[pos++] = 0x00;
	entry[pos++] = 0x00;
	entry[pos++] = 0x48;	//mov rsi, rsp
	entry[pos++] = 0x89;
	entry[pos++] = 0xe6;
	entry[pos++] = 0x48;	//mov rdx, 0x1
	entry[pos++] = 0xc7;
	entry[pos++] = 0xc2;
	entry[pos++] = 0x01;
	entry[pos++] = 0x00;
	entry[pos++] = 0x00;
	entry[pos++] = 0x00;
	entry[pos++] = 0x0f;	//syscall
	entry[pos++] = 0x05;
	entry[pos++] = 0x48;	//add rsp, 0x8
	entry[pos++] = 0x83;
	entry[pos++] = 0xc4;
	entry[pos++] = 0x08;
	entry[pos++] = 0x5a;	//pop rdx
	entry[pos++] = 0x5e;	//pop rsi
	entry[pos++] = 0x5f;	//pop rdi


	// entry[pos++] = 0xff;
	// entry[pos++] = 0x25;
	entry[pos++] = 0xe9;				//jmp
	entry[pos++] = ((uint32_t)offset>>0) & 0xff;
	entry[pos++] = ((uint32_t)offset>>8) & 0xff;
	entry[pos++] = ((uint32_t)offset>>16) & 0xff;
	entry[pos++] = ((uint32_t)offset>>24) & 0xff;

	fwrite(entry, ADD_SIZE, 1, fp);

}
int main(int argc, char* argv[])
{
	FILE *fp, *fa, *fw;
	fp = fopen(argv[1],"rb+");
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
	jumpgot_write(pltnum, fw, fp, shdr, ehdr);
	//get program header
	// Elf64_Phdr *phdr = malloc(sizeof(Elf64_Phdr) * ehdr->e_phnum);
	// fseek(fp, ehdr->e_phoff, SEEK_SET);
	// fread(phdr, sizeof(Elf64_Phdr), ehdr->e_phnum, fp);
	// Elf64_Phdr *p_phdr = phdr;
	// for(int i = 0; i < ehdr->e_phnum; i++)
	// {
	// 	printf("segment %d\tfilesize:%lx,\t offset:%lx\n", i, (unsigned long)p_phdr->p_filesz,  (unsigned long)p_phdr->p_offset);
	// 	p_phdr++;
	// }
	// Elf64_Shdr *p_shdr = shdr;
	// Elf64_Xword file_end = 0;
	// for(int i = 0; i < ehdr->e_shnum; i++)
	// {
	// 	printf("section%d\tfilesize:%lx,\t offset:%lx\n", i, (unsigned long)p_shdr->sh_size,  (unsigned long)p_shdr->sh_offset);
	// 	if(file_end < p_shdr->sh_offset + p_shdr->sh_size)
	// 		file_end =  p_shdr->sh_offset + p_shdr->sh_size;
	// 	p_shdr++;
	// }
	// printf("end of file:%lu\n", (unsigned long)file_end);
	// rewrite_entry(fp, file_end, ehdr);
	fclose(fp);
	fclose(fw);
	return 0;
}
