#include "enc.h"
#include <stdio.h>
#include <stdlib.h>


void write_object_file(Instruction *insns, int count, const char *obj_path, int *function_starts,
		       char **function_names, int function_count)
{
	int max_label = 0;
	for (int i = 0; i < count; i++) {
		if (insns[i].op == OP_LABEL && insns[i].target > max_label) {
			max_label = insns[i].target;
		}
	}

	int *label_addr = calloc(max_label + 1, sizeof(int));
	uint32_t *out = calloc(count * 8 + 8, sizeof(uint32_t));
	int pos;

	first_pass(insns, count, label_addr, out, &pos);
	second_pass(insns, count, label_addr, out, &pos);

	int piece_borders[function_count + 1];
	int main_index;

	split_bytes(insns, count, function_starts, function_names, function_count, out, piece_borders, &main_index);

	free(label_addr);

	int main_start = piece_borders[main_index];
	int nwords;
	uint32_t *text = wrap_code(out + main_start, piece_borders[main_index + 1] - main_start, &nwords);
	text = append_dead_functions(text, out, piece_borders, function_count, main_index, &nwords);
	free(out);

	uint32_t code_size = nwords * 4;
	uint32_t hdrs_size = 32 + 152 + 24;
	uint32_t symoff = hdrs_size + code_size;
	uint32_t stroff = symoff + sizeof(struct nlist_64);

	struct mach_header_64 hdr = make_mach_header(2, hdrs_size - 32);
	struct segment_command_64 seg = make_segment(1, code_size);
	struct section_64 sec = make_section(hdrs_size, code_size);
	struct symtab_command st = make_symtab(symoff, 1, stroff, 8);
	struct nlist_64 nl = make_nlist(1, 0);

	FILE *f = fopen(obj_path, "wb");
	if (!f) {
		fprintf(stderr, "Fatal: Cannot open %s\n", obj_path);
		free(text);
		exit(1);
	}

	fwrite(&hdr, sizeof(hdr), 1, f);
	fwrite(&seg, sizeof(seg), 1, f);
	fwrite(&sec, sizeof(sec), 1, f);
	fwrite(&st, sizeof(st), 1, f);
	fwrite(text, 4, nwords, f);
	fwrite(&nl, sizeof(nl), 1, f);
	char strtab[8] = "\0_main\0";
	fwrite(strtab, 1, 8, f);
	fclose(f);
	free(text);
}
