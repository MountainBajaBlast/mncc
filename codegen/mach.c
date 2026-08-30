#include "../IR/ir.h"
#include "enc.h"
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


struct mach_header_64 make_mach_header(int ncmds, int sizeofcmds)
{
	return (struct mach_header_64){
	    .magic = MH_MAGIC_64,
	    .cputype = CPU_TYPE_ARM64,
	    .filetype = MH_OBJECT,
	    .ncmds = ncmds,
	    .sizeofcmds = sizeofcmds,
	};
}

struct segment_command_64 make_segment(int nsects, uint64_t vmsize)
{
	return (struct segment_command_64){
	    .cmd = LC_SEGMENT_64,
	    .cmdsize = 72 + nsects * 80,
	    .segname = "__TEXT",
	    .vmsize = vmsize,
	    .filesize = vmsize,
	    .maxprot = 7,
	    .initprot = 7,
	    .nsects = nsects,
	};
}

struct section_64 make_section(uint32_t offset, uint64_t size)
{
	return (struct section_64){
	    .sectname = "__text",
	    .segname = "__TEXT",
	    .size = size,
	    .offset = offset,
	    .align = 2,
	};
}

struct symtab_command make_symtab(uint32_t symoff, int nsyms, uint32_t stroff, uint32_t strsize)
{
	return (struct symtab_command){
	    .cmd = LC_SYMTAB,
	    .cmdsize = 24,
	    .symoff = symoff,
	    .nsyms = nsyms,
	    .stroff = stroff,
	    .strsize = strsize,
	};
}

struct nlist_64 make_nlist(uint32_t strx, uint64_t value)
{
	return (struct nlist_64){
	    .n_un.n_strx = strx,
	    .n_type = N_EXT | N_SECT,
	    .n_sect = 1,
	    .n_value = value,
	};
}

uint32_t *wrap_code(uint32_t *code, int code_size, int *out_nwords)
{
	int total = 2 + code_size + 3;
	uint32_t *text = calloc(total, sizeof(uint32_t));

	text[0] = 0xA9BF7BFD;
	text[1] = 0xAA0003FD;
	memcpy(text + 2, code, code_size * 4);
	text[code_size + 2] = 0xA8C17BFD;
	text[code_size + 3] = 0xD2800030;
	text[code_size + 4] = 0xD4001001;

	*out_nwords = total;

	return text;
}
