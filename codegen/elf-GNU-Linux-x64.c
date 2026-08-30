#include <elf.h>
#include <stddef.h>
#include <stdint.h>



struct elf64_hdr make_elf_header(uint32_t shoff, int shnum)
{
	return (struct elf64_hdr){
	    .e_ident = {0x7F, 'E', 'L', 'F', 2, 1, 1},
	    .e_type = 1,
	    .e_machine = 62,
	    .e_version = 1,
	    .e_entry = 0,
	    .e_phoff = 0,
	    .e_shoff = shoff,
	    .e_flags = 0,
	    .e_ehsize = 64,
	    .e_phentsize = 56,
	    .e_phnum = 0,
	    .e_shentsize = 64,
	    .e_shnum = shnum,
	    .e_shstrndx = 3,
	};
}




struct elf64_shdr make_section(uint32_t name, uint32_t type, uint64_t flags, uint64_t offset, uint64_t size)
{
	return (struct elf64_shdr){
	    .sh_name = name,
	    .sh_type = type,
	    .sh_flags = flags,
	    .sh_offset = offset,
	    .sh_size = size,
	};
}

struct elf64_shdr make_text_section(uint64_t offset, uint64_t size)
{
	return (struct elf64_shdr){
	    .sh_name = 0,
	    .sh_type = 1,
	    .sh_flags = 6,
	    .sh_offset = offset,
	    .sh_size = size,
	};
}

struct elf64_shdr make_symtab_section(uint64_t offset, uint64_t size)
{
	return (struct elf64_shdr){
	    .sh_name = 6,
	    .sh_type = 2,
	    .sh_offset = offset,
	    .sh_size = size,
	    .sh_link = 2,
	    .sh_info = 1,
	    .sh_addralign = 8,
	    .sh_entsize = 24,
	};
}

struct elf64_shdr make_strtab_section(uint32_t name, uint64_t offset, uint64_t size)
{
	return (struct elf64_shdr){
	    .sh_name = name,
	    .sh_type = 3,
	    .sh_offset = offset,
	    .sh_size = size,
	};
}

struct elf64_sym make_symbol(uint32_t strx, uint64_t value, uint64_t size)
{
	return (struct elf64_sym){
	    .st_name = strx,
	    .st_info = 0x12,
	    .st_shndx = 1,
	    .st_value = value,
	    .st_size = size,
	};
}
