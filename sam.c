#include <string.h>
#include "sam.h"

#define TYPE_BAM  1
#define TYPE_READ 2

bam_header_t *bam_header_dup(const bam_header_t *h0)
{
	bam_header_t *h;
	int i;
	h = bam_header_init();
	*h = *h0;
	h->hash = 0;
	h->text = (char*)calloc(h->l_text + 1, 1);
	memcpy(h->text, h0->text, h->l_text);
	h->target_len = (uint32_t*)calloc(h->n_targets, 4);
	h->target_name = (char**)calloc(h->n_targets, sizeof(void*));
	for (i = 0; i < h->n_targets; ++i) {
		h->target_len[i] = h0->target_len[i];
		h->target_name[i] = strdup(h0->target_name[i]);
	}
	return h;
}

samfile_t *samopen(const char *fn, const char *mode, const void *aux)
{
	samfile_t *fp;
	fp = (samfile_t*)calloc(1, sizeof(samfile_t));
	if (mode[0] == 'r') { // read
		fp->type |= TYPE_READ;
		if (mode[1] == 'b') { // binary
			fp->type |= TYPE_BAM;
			fp->x.bam = strcmp(fn, "-")? bam_open(fn, "r") : bam_dopen(fileno(stdin), "r");
			if (fp->x.bam == 0) goto open_err_ret;
			fp->header = bam_header_read(fp->x.bam);
		} else { // text
			fp->x.tamr = sam_open(fn);
			if (fp->x.tamr == 0) goto open_err_ret;
			fp->header = sam_header_read(fp->x.tamr);
			if (fp->header->n_targets == 0) { // no @SQ fields
				if (aux) { // check if aux is present
					bam_header_destroy(fp->header);
					fp->header = sam_header_read2((const char*)aux);
				}
				if (fp->header->n_targets == 0)
					fprintf(stderr, "[samopen] no @SQ lines in the header.\n");
			} else fprintf(stderr, "[samopen] SAM header is present: %d sequences.\n", fp->header->n_targets);
		}
	} else if (mode[0] == 'w') { // write
		fp->header = bam_header_dup((const bam_header_t*)aux);
		if (mode[1] == 'b') { // binary
			fp->type |= TYPE_BAM;
			fp->x.bam = strcmp(fn, "-")? bam_open(fn, "w") : bam_dopen(fileno(stdout), "w");
			if (fp->x.bam == 0) goto open_err_ret;
			bam_header_write(fp->x.bam, fp->header);
		} else { // text
			// open file
			fp->x.tamw = strcmp(fn, "-")? fopen(fn, "w") : stdout;
			if (fp->x.tamr == 0) goto open_err_ret;
			// write header
			if (strstr(mode, "h")) {
				int i;
				bam_header_t *alt;
				// parse the header text 
				alt = bam_header_init();
				alt->l_text = fp->header->l_text; alt->text = fp->header->text;
				sam_header_parse(alt);
				alt->l_text = 0; alt->text = 0;
				// check if there are @SQ lines in the header
				fwrite(fp->header->text, 1, fp->header->l_text, fp->x.tamw);
				if (alt->n_targets) { // then write the header text without dumping ->target_{name,len}
					if (alt->n_targets != fp->header->n_targets)
						fprintf(stderr, "[samopen] inconsistent number of target sequences.\n");
				} else { // then dump ->target_{name,len}
					for (i = 0; i < fp->header->n_targets; ++i)
						fprintf(fp->x.tamw, "@SQ\tSN:%s\tLN:%d\n", fp->header->target_name[i], fp->header->target_len[i]);
				}
				bam_header_destroy(alt);
			}
		}
	}
	return fp;

open_err_ret:
	free(fp);
	return 0;
}

void samclose(samfile_t *fp)
{
	if (fp == 0) return;
	if (fp->header) bam_header_destroy(fp->header);
	if (fp->type & TYPE_BAM) bam_close(fp->x.bam);
	else if (fp->type & TYPE_READ) sam_close(fp->x.tamr);
	else fclose(fp->x.tamw);
	free(fp);
}

int samread(samfile_t *fp, bam1_t *b)
{
	if (fp == 0 || !(fp->type & TYPE_READ)) return -1; // not open for reading
	if (fp->type & TYPE_BAM) return bam_read1(fp->x.bam, b);
	else return sam_read1(fp->x.tamr, fp->header, b);
}

int samwrite(samfile_t *fp, const bam1_t *b)
{
	if (fp == 0 || (fp->type & TYPE_READ)) return -1; // not open for writing
	if (fp->type & TYPE_BAM) return bam_write1(fp->x.bam, b);
	else {
		char *s = bam_format1(fp->header, b);
		int l = strlen(s);
		fputs(s, fp->x.tamw); fputc('\n', fp->x.tamw);
		free(s);
		return l + 1;
	}
}

int sampileup(samfile_t *fp, int mask, bam_pileup_f func, void *func_data)
{
	bam_plbuf_t *buf;
	int ret;
	bam1_t *b;
	b = bam_init1();
	buf = bam_plbuf_init(func, func_data);
	bam_plbuf_set_mask(buf, mask);
	while ((ret = samread(fp, b)) >= 0)
		bam_plbuf_push(b, buf);
	bam_plbuf_push(0, buf);
	bam_plbuf_destroy(buf);
	bam_destroy1(b);
	return 0;
}
