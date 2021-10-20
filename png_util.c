//
// Created by Karim Alatrash on 2021-09-20.
//
  
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <limits.h>
#include "l_list.h"
#include "lab_png.h"
#include "zutil.h"
#include "crc.h"
#include "png_util.h"
U8 sig[] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};

int is_png(U8 *buf, U8 *n) {
  unsigned int ret = 0;
  for(int i = 0; i < PNG_SIG_SIZE; i++) {
    ret += buf[i]^n[i];
  }
  if(ret) {
    return 0;
  }
  return 1;
}
unsigned int read_u32(FILE *fp) {
  U32 ret;
  fread(&ret, sizeof(U32), 1, fp);
  ret = ntohl(ret);
  return ret;
}
void write_u32(FILE *fp, U32 dat) {
  dat = htonl(dat);
  fwrite(&dat, sizeof(U32), 1, fp);
}
void get_png_data_IHDR(U8 *p_data, struct data_IHDR *out) {
  U32 *p_width = (U32 *)p_data;
  U32 *p_height = (U32 *)&p_data[4];
  out->width = ntohl(* p_width);
  out->height = ntohl(* p_height);
  out->bit_depth = p_data[8];
  out->color_type = p_data[9];
  out->compression = p_data[10];
  out->filter = p_data[11];
  out->interlace = p_data[12];
}
void ihdr_to_buf (U8 *p_data, struct data_IHDR dat) {
  dat.width = htonl(dat.width);
  dat.height = htonl(dat.height);
  U8 *b_width = (U8 *)&dat.width;
  U8 *b_height = (U8 *)&dat.height;
  for(int i = 0; i < sizeof(U32); i++ ) {
    p_data[i] = b_width[i];
    p_data[i+4] = b_height[i];
  }

  p_data[8] = dat.bit_depth;
  p_data[9] = dat.color_type;
  p_data[10] = dat.compression;
  p_data[11] = dat.filter;
  p_data[12] = dat.interlace;
}
void read_chunk(struct chunk *out, FILE *fp) {
  out->length = read_u32(fp);
  U32 type;
  fread(&type, sizeof(U32), 1, fp);
  memcpy(&out->type, &type, sizeof(U32));
  out->p_data = malloc(out->length);
  fread(out->p_data, out->length, 1, fp); /*this data has not been converted to host endianness yet.*/
  out->crc = read_u32(fp);
}
void write_chunk(struct chunk *dat, FILE *fp) {
  write_u32(fp, dat->length);
  U32 *type_p = (U32 *)dat->type;
  fwrite(type_p, sizeof(U32), 1, fp);
  fwrite(dat->p_data, dat->length, 1, fp);
  write_u32(fp, dat->crc);
}

int read_png_buf(struct simple_PNG *png_out, struct data_IHDR *ihdr_dat, char* buf, U32 buf_size) {
  U32 buf_index = 0;
  U8 *u_buf = (U8 *)buf;
  if(!is_png(u_buf, sig)) {
    return 1;
  }
  buf_index += PNG_SIG_SIZE;

  png_out->p_IHDR = malloc(sizeof(struct chunk));
  png_out->p_IDAT = malloc(sizeof(struct chunk));
  png_out->p_IEND = malloc(sizeof(struct chunk));

  /*reading all the chunk data*/
  struct chunk *chunk_arr[3] = {png_out->p_IHDR, png_out->p_IDAT, png_out->p_IEND};
  U32 u32_temp;
  for (int i = 0; i < 3; i++) {
    /*read length*/
    memcpy(&u32_temp, buf+buf_index, 4);
    chunk_arr[i]->length = ntohl(u32_temp);
    buf_index += 4;

    /*read type*/
    memcpy(chunk_arr[i]->type, buf+buf_index, 4);
    buf_index += 4;

    /*read data*/
    chunk_arr[i]->p_data = malloc(chunk_arr[i]->length);
    memcpy(chunk_arr[i]->p_data, buf+buf_index, chunk_arr[i]->length);
    buf_index += chunk_arr[i]->length;

    /*read CRC*/
    memcpy(&u32_temp, buf+buf_index, 4);
    chunk_arr[i]->crc = ntohl(u32_temp);
    buf_index += 4;
  }

  /*loading IHDR data into the struct given*/
  get_png_data_IHDR(png_out->p_IHDR->p_data, ihdr_dat);

  return 0;
}

int read_png_file(struct simple_PNG *png_out, struct data_IHDR *ihdr_dat, FILE *fp, int mode) {
  U8 png_sig[PNG_SIG_SIZE]; /*allocates bytes for png signature header*/
  
  /*read header*/
  fread(&png_sig, PNG_SIG_SIZE, 1, fp);
  if(!is_png (png_sig, sig)) {
    return 0;
  }
  if(mode == 1) {
    png_out->p_IHDR = malloc(sizeof(struct chunk));
    read_chunk(png_out->p_IHDR, fp);
    get_png_data_IHDR(png_out->p_IHDR->p_data, ihdr_dat);
    /*you can print the fetched width from ihdr_dat*/
  }
  else if (mode == 2) {

    png_out->p_IHDR = malloc(sizeof(struct chunk));
    png_out->p_IDAT = malloc(sizeof(struct chunk));
    png_out->p_IEND = malloc(sizeof(struct chunk));
    /*chunks must be read in order or must seek appropriate distance*/
    read_chunk(png_out->p_IHDR, fp);
    read_chunk(png_out->p_IDAT, fp);
    read_chunk(png_out->p_IEND, fp);
    get_png_data_IHDR(png_out->p_IHDR->p_data, ihdr_dat);
  }
  
  return 1;
}
void write_png (struct simple_PNG *dat, FILE *fp) {
  /*write header*/
  fwrite(sig, PNG_SIG_SIZE, 1, fp);

  write_chunk(dat->p_IHDR, fp);
  write_chunk(dat->p_IDAT, fp);
  write_chunk(dat->p_IEND, fp);
}
void free_png (struct simple_PNG *png) {
  if(png->p_IHDR != NULL) {
    free(png->p_IHDR->p_data);
    free(png->p_IHDR);
  }
  if(png->p_IDAT != NULL) {
    free(png->p_IDAT->p_data);
    free(png->p_IDAT);
  }
  if(png->p_IEND != NULL) {
    free(png->p_IEND->p_data);
    free(png->p_IEND); 
  } 
}

U8* inf_png_dat(struct simple_PNG **png_arr, U8 *new_ihdr_dat, int argc, U64 *inf_len) {
  U64 dest_len = 0;
  //FILE *fp;
  U8 *dest_buf = malloc(1);
  U32 total_height = 0;
  U32 width = UINT_MAX;

  struct data_IHDR curr_ihdr_dat;

  for(int i = 1; i < argc; i++) {

    get_png_data_IHDR(png_arr[i-1]->p_IHDR->p_data, &curr_ihdr_dat);

    /*This block disallows any images not of the same width as image 1*/
    if(width == UINT_MAX) {
      width = curr_ihdr_dat.width;
    } else if (width != curr_ihdr_dat.width) {
      free_png(png_arr[i-1]);
      //fclose(fp);
      continue;
    }
    total_height += curr_ihdr_dat.height;
    //fclose(fp);

    /*ALLOCATE BUFFER SPACE*/
    dest_len += curr_ihdr_dat.height * (curr_ihdr_dat.width*4 + 1);
    U8 * new_buf = realloc(dest_buf, dest_len);
    if(new_buf != NULL) {
      dest_buf = new_buf;
    } else {
      fprintf(stderr,"!!!!! memory was failed to be allocated !!!!!\n");
      exit(-1);
    }

    /*INFLATE AND CONCAT DATA*/
    U64 filled_space = dest_len - curr_ihdr_dat.height * (curr_ihdr_dat.width*4 + 1);
    U64 fuller_space = 0;
    mem_inf(&dest_buf[filled_space], &fuller_space, png_arr[i-1]->p_IDAT->p_data, png_arr[i-1]->p_IDAT->length);
    
  }

  /*writing ihdr data to png_arr[0]*/
  curr_ihdr_dat.height = total_height;
  ihdr_to_buf(new_ihdr_dat, curr_ihdr_dat);

  *inf_len = dest_len;
  return dest_buf;
}

/*deflate the decompressed idat value*/
U8 * def_png_dat(U8 *inf_dat, U64 *len_def, U64 len_inf) {
  int ret = mem_def(inf_dat, len_def, inf_dat, len_inf, Z_DEFAULT_COMPRESSION);
  if (ret != 0) { /* success */
    fprintf(stderr,"mem_def failed\n");
    exit(-1);
  }

  /*reallocate the correct size memory*/
  U8 * new_buf = realloc(inf_dat, *len_def);/*malloc(*len_def);*/
  if(new_buf != NULL) {
    return new_buf;/*memcpy(new_buf, inf_dat, *len_def);*/
  } else {
    fprintf(stderr,"!!!!! memory was failed to be allocated !!!!!\n");
    exit(-1);
  }

}

void calc_crc(struct chunk *c) {
  /*CRC check on ihdr chunk*/
  U8 *crc_buf = malloc(4 + c->length);
  memcpy(crc_buf, c->type, 4);
  memcpy(&crc_buf[4], c->p_data, c->length);
  c->crc = crc(crc_buf, c->length+4);
  free(crc_buf);
}

void cat_png(struct simple_PNG **png_arr, int argc){

  U64 inf_len = 0;
  U64 def_len = 0;
  
  U8 *new_ihdr_dat = malloc(13);
  //struct simple_PNG **png_arr = malloc(sizeof(struct simple_PNG *) * argc);

  U8* inf_dat = inf_png_dat(png_arr, new_ihdr_dat, argc, &inf_len);
  U8* def_dat = def_png_dat(inf_dat, &def_len, inf_len);
  inf_dat = NULL;

  free(png_arr[0]->p_IDAT->p_data);
  free(png_arr[0]->p_IHDR->p_data);
  png_arr[0]->p_IDAT->p_data = def_dat;
  png_arr[0]->p_IDAT->length = def_len;
  png_arr[0]->p_IHDR->p_data = new_ihdr_dat;
  calc_crc(png_arr[0]->p_IDAT);
  calc_crc(png_arr[0]->p_IHDR);

  FILE *fp = fopen("all.png", "wb");

  write_png(png_arr[0], fp);

  fclose(fp);

}