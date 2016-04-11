#include <stdint.h>
#define JUMP_SIZE 184//135//106//86
#define JUMP_RESOLVE_SIZE 63//106
#define RESOLVE_STACKGOT_OFF1 109
#define RESOLVE_STACKGOT_OFF2 132
#define RESOLVE_STACKGOT_OFF3 142
#define RESOLVE_INSN_OFF 92//61/*23*///+
#define SGOT_SIZE 8
#define BACK_8PARAM_SIZE 112
#define BACK_SIZE 108//90//56/* 18*/
#define JUMP_INSN_OFF 121//92//67//61 /*23*/
#define RESOLVE_STACK_OFF 61
#define ADD_SIZE 56
#define JUMP_CHECK_OFF  24//62//60
#define JUMP_STACK_OFF 90//61
#define BACK_FLAG_OFF 107
#define BACK_STACK_OFF 62
#define BACK_STACKGOT_OFF 96
#define BACK_8PARAM_FLAG_OFF 111
#define BACK_8PARAM_STACK_OFF 62
#define BACK_8PARAM_STACKGOT_OFF 100
#define JUMP_STACKGOT_OFF1 90
#define JUMP_STACKGOT_OFF2 135
#define JUMP_STACKGOT_OFF3 178
#define JUMP_FLAG_OFF1 144
#define JUMP_FLAG_OFF2 171
#define EI_NIDENT (16)

#define PT_NULL   0   /* Program header table entry unused */
#define PT_LOAD   1   /* Loadable program segment */
#define PT_DYNAMIC  2   /* Dynamic linking information */
#define PT_INTERP 3   /* Program interpreter */
#define PT_NOTE   4   /* Auxiliary information */
#define PT_SHLIB  5   /* Reserved */
#define PT_PHDR   6   /* Entry for header table itself */
#define PT_TLS    7   /* Thread-local storage segment */
#define PT_NUM    8   /* Number of defined types */

#define ALIGN(n, m) (n + m - 1) & ~(m - 1)

typedef uint64_t Elf64_Addr;
typedef uint64_t Elf64_Off;
typedef uint16_t Elf64_Half;
typedef uint32_t Elf64_Word;
typedef uint64_t Elf64_Xword;
typedef struct
{
  unsigned char	e_ident[EI_NIDENT];	/* Magic number and other info */
  Elf64_Half	e_type;			/* Object file type */
  Elf64_Half	e_machine;		/* Architecture */
  Elf64_Word	e_version;		/* Object file version */
  Elf64_Addr	e_entry;		/* Entry point virtual address */
  Elf64_Off	e_phoff;		/* Program header table file offset */
  Elf64_Off	e_shoff;		/* Section header table file offset */
  Elf64_Word	e_flags;		/* Processor-specific flags */
  Elf64_Half	e_ehsize;		/* ELF header size in bytes */
  Elf64_Half	e_phentsize;		/* Program header table entry size */
  Elf64_Half	e_phnum;		/* Program header table entry count */
  Elf64_Half	e_shentsize;		/* Section header table entry size */
  Elf64_Half	e_shnum;		/* Section header table entry count */
  Elf64_Half	e_shstrndx;		/* Section header string table index */
} Elf64_Ehdr;

typedef struct
{
  Elf64_Word	sh_name;		/* Section name (string tbl index) */
  Elf64_Word	sh_type;		/* Section type */
  Elf64_Xword	sh_flags;		/* Section flags */
  Elf64_Addr	sh_addr;		/* Section virtual addr at execution */
  Elf64_Off	sh_offset;		/* Section file offset */
  Elf64_Xword	sh_size;		/* Section size in bytes */
  Elf64_Word	sh_link;		/* Link to another section */
  Elf64_Word	sh_info;		/* Additional section information */
  Elf64_Xword	sh_addralign;		/* Section alignment */
  Elf64_Xword	sh_entsize;		/* Entry size if section holds table */
} Elf64_Shdr;//section header table entries

typedef struct
{
  Elf64_Word	p_type;			/* Segment type */
  Elf64_Word	p_flags;		/* Segment flags */
  Elf64_Off	p_offset;		/* Segment file offset */
  Elf64_Addr	p_vaddr;		/* Segment virtual address */
  Elf64_Addr	p_paddr;		/* Segment physical address */
  Elf64_Xword	p_filesz;		/* Segment size in file */
  Elf64_Xword	p_memsz;		/* Segment size in memory */
  Elf64_Xword	p_align;		/* Segment alignment */
} Elf64_Phdr;//program header table entries

typedef struct js_header
{
  uint32_t jump_resolve_size;
  uint32_t jump_size;
  uint32_t sgot_size;
  uint32_t zero_size;
  uint32_t back8_size;
  uint32_t back_size;
  uint32_t jump_resolve_off;
  uint32_t jump_off;
  uint32_t sgot_off;
  uint32_t back8_off;
  uint32_t back_off;
  uint64_t entry_off;
  uint64_t entry_addr;
  uint32_t flag_off;
  uint32_t check_off;
  uint32_t stack_off; //stack to recover  
  uint32_t stack_got; //shared stack got
}js_header;
