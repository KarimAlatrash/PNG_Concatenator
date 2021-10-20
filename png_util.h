
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <limits.h>
#include "lab_png.h"
#include "zutil.h"
#include "crc.h"

int is_png(U8 *buf, U8 *n);
unsigned int read_u32(FILE *fp);
void write_u32(FILE *fp, U32 dat);
void get_png_data_IHDR(U8 *p_data, struct data_IHDR *out);
void ihdr_to_buf (U8 *p_data, struct data_IHDR dat);
void read_chunk(struct chunk *out, FILE *fp);
void write_chunk(struct chunk *dat, FILE *fp);
int read_png_buf(struct simple_PNG *png_out, struct data_IHDR *ihdr_dat, char *buf, U32 buf_size);
int read_png_file(struct simple_PNG *png_out, struct data_IHDR *ihdr_dat, FILE *fp, int mode);
void write_png (struct simple_PNG *dat, FILE *fp);
void free_png (struct simple_PNG *png);
U8* inf_png_dat(struct simple_PNG **png_arr, U8 *new_ihdr_dat, int argc, U64 *inf_len);
U8 * def_png_dat(U8 *inf_dat, U64 *len_def, U64 len_inf);
void calc_crc(struct chunk *c);
void cat_png(struct simple_PNG **png_arr, int argc);
