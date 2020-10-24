/*
 *   Copyright 2016 Matt Edlefsen <matt.edlefsen@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, Inc., 53 Temple Place Ste 330,
 *   Boston MA 02111-1307, USA; either version 2 of the License, or
 *   (at your option) any later version; incorporated herein by reference.
 *
 * ----------------------------------------------------------------------- */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <ext4.h>
#include <ext4_mkfs.h>
#include <ext4_blockdev.h>
#include <ext4_fs.h>
#include <ext4_super.h>
#include <ext4_inode.h>

#include <linux/file_dev.h>

#include <libinstaller/syslinux.h>
#include <libinstaller/syslxint.h>
#include <sys/stat.h>

#include "disks.h"

static const char APPNAME[] = "loopless";
static const char VERSION[] = LOOPLESS_VERSION;
static const char COPYRIGHT[] = "Copyright 2016 Matt Edlefsen";

#ifndef EXT2_SUPER_OFFSET
#define EXT2_SUPER_OFFSET 1024
#endif

extern unsigned char gptmbr_bin[];
extern unsigned int gptmbr_bin_len;

const char vmdk_template[] = ""
  "# Disk DescriptorFile\n"
  "version=1\n"
  "CID=%08x\n"
  "parentCID=ffffffff\n"
  "createType=\"monolithicFlat\"\n"
  "# Extent description\n"
  "RW %lld FLAT \"%s\"\n\n"
  "ddb.geometry.sectors=\"63\"\n"
  "ddb.geometry.heads=\"16\"\n"
  "ddb.geometry.cylinders=\"%lld\"\n"
;


void patch_bootsect(struct ext4_blockdev* bd) {
  struct fat_boot_sector *sbs;
  sbs = (struct fat_boot_sector*)syslinux_bootsect;
  uint64_t totalsectors = bd->part_size >> SECTOR_SHIFT;

  if (totalsectors >= 65536) {
    set_16(&sbs->bsSectors, 0);
  } else {
    set_16(&sbs->bsSectors, totalsectors);
  }
  set_32(&sbs->bsHugeSectors, totalsectors);

  set_16(&sbs->bsBytesPerSec, SECTOR_SIZE);
  set_16(&sbs->bsSecPerTrack, 32);
  set_16(&sbs->bsHeads, 64);
  set_32(&sbs->bsHiddenSecs, 0);
}

int patch_syslinux(struct ext4_inode_ref* ref) {
  int num_sectors = (boot_image_len + SECTOR_SIZE - 1) >> SECTOR_SHIFT;
  num_sectors += 2;
  sector_t sectors[num_sectors];
  memset(sectors,0,sizeof(sector_t) * num_sectors);
  uint64_t file_size = ext4_inode_get_size(&ref->fs->sb,ref->inode);
  int block_size = ext4_sb_get_block_size(&ref->fs->sb);
  uint32_t num_blocks = (file_size / block_size) + ((file_size % block_size) ? 1 : 0);
  block_size >>= SECTOR_SHIFT;
  for(int bi = 0, si = 0; si < num_sectors && bi < num_blocks; ++bi) {
    ext4_fsblk_t block;
    ext4_fs_get_inode_dblk_idx(ref, bi, &block, 1);
    for(int bsi = 0; si < num_sectors && bsi < block_size; ++bsi, ++si) {
      sectors[si] = ((sector_t)block)*block_size + bsi;
    }
  }

  return syslinux_patch(sectors, num_sectors, 0, 0, "/", NULL);
}

int syslinux(struct ext4_blockdev* bd) {
  ext4_file f;
  struct ext4_inode_ref ref;
  syslinux_reset_adv(syslinux_adv);
  ext4_fremove("/ldlinux.sys");
  if(ext4_fopen(&f,"/ldlinux.sys","w") != EOK) return 101;
  if(ext4_fwrite(&f,boot_image,boot_image_len,NULL) != EOK) return 102;
  if(ext4_fwrite(&f,syslinux_adv,sizeof(syslinux_adv),NULL) != EOK) return 103;
  if(ext4_fs_get_inode_ref(bd->fs,f.inode,&ref) != EOK) return 104;
  if(ext4_fclose(&f) != EOK) return 105;
  ext4_inode_set_flag(ref.inode,EXT4_INODE_FLAG_IMMUTABLE);
  if(ext4_mode_set("/ldlinux.sys", 0444) != EOK) return 106;

  patch_bootsect(bd);
  int modbytes = patch_syslinux(&ref);

  if(ext4_fopen(&f,"/ldlinux.sys","rb+") != EOK) return 107;
  if(ext4_fwrite(&f,boot_image,modbytes,NULL) != EOK) return 108;
  if(ext4_fclose(&f) != EOK) return 109;

  if(ext4_fopen(&f,"/ldlinux.c32","w") != EOK) return 110;
  if(ext4_fwrite(&f,(char*)syslinux_ldlinuxc32,syslinux_ldlinuxc32_len,NULL) != EOK) return 111;
  if(ext4_fs_get_inode_ref(bd->fs,f.inode,&ref) != EOK) return 112;
  ext4_inode_set_flag(ref.inode,EXT4_INODE_FLAG_IMMUTABLE);
  if(ext4_fclose(&f) != EOK) return 113;
  if(ext4_mode_set("/ldlinux.c32", 0444) != EOK) return 114;

  if(ext4_block_writebytes(bd,0,syslinux_bootsect,syslinux_bootsect_len) != EOK) return 115;
  return 0;
}


void list_dir(char* path, int size) {
  ext4_dir d;
  const ext4_direntry* e;
  if(ext4_dir_open(&d,path) != EOK) exit(20);
  while(e = ext4_dir_entry_next(&d), e != NULL) {
    if(strcmp(".",(char*)e->name) != 0 && strcmp("..",(char*)e->name) != 0) {
      int csize = size+e->name_length;
      char* cpath = alloca(csize+2);
      memcpy(cpath,path,size);
      memcpy(cpath+size,e->name,e->name_length);
      if(e->inode_type == EXT4_DE_DIR) {
        cpath[csize] = '/';
        cpath[csize+1] = '\0';
        list_dir(cpath,csize+1);
      } else {
        printf("%.*s\n",csize,cpath);
      }
    }
  }
  if(ext4_dir_close(&d) != EOK) exit(21);
}

bool dir_is_empty(char* path) {
  ext4_dir d;
  const ext4_direntry* e;
  if(ext4_dir_open(&d,path) != EOK) exit(30);
  while(e = ext4_dir_entry_next(&d), e != NULL) {
    if(strcmp(".",(char*)e->name) != 0 && strcmp("..",(char*)e->name) != 0) {
      return false;
    }
  }
  if(ext4_dir_close(&d) != EOK) exit(31);
  return true;
}

void relative_path(char** output, char* from, char* to) {
  char* abs_from = realpath(from,NULL);
  char* abs_to = realpath(to,NULL);
  int abs_to_size = strlen(abs_to);
  char* common = abs_from + 1;
  char* p;
  while(p = strchr(common,'/'), p && strncmp(abs_from,abs_to,p+1-abs_from) == 0) {
    common = p+1;
  }
  int updirs = p?1:0;
  if(p) while(p = strchr(p+1,'/'), p) ++updirs;
  int common_size = common - abs_from;

  int relpath_size = updirs*3 + (abs_to_size - common_size);
  char* rp = malloc(relpath_size + 1);
  int i;
  for(i = 0; i < updirs*3; i+=3) {
    rp[i] = '.'; rp[i+1] = '.'; rp[i+2] = '/';
  }
  memcpy(rp+i,abs_to+common_size,abs_to_size-common_size);
  *output = rp;
  free(abs_to);
  free(abs_from);
}

typedef enum {
  IMG_WHOLE_FILE,
  IMG_PART_NUM,
  IMG_PART_RANGE
} image_path_type;

typedef struct {
  image_path_type type;
  char* path;
  union {
    int part_num;
    struct {
      uint64_t start;
      uint64_t size;
    };
  };
} image_path_info;

int parse_image_path(image_path_info* info, char* path, image_path_type type) {
  memset(info,0,sizeof(*info));
  info->type = type;
  info->path = path;

  if(type == IMG_WHOLE_FILE) {
    return 0;
  } else {
    char* part_divider = strchr(path, ':');
    if(part_divider != NULL) {
      *part_divider = '\0';

      if(type == IMG_PART_RANGE) {
        if(sscanf(part_divider+1,"%zd,%zd",&info->start,&info->size) == 2) return 0;
      }

      if(sscanf(part_divider+1,"%d",&info->part_num) == 1) {
        if(type == IMG_PART_NUM) return 0;
        int result;
        disk_info di;
        result = collect_disk_info(&di,path);
        if(result != 0) return result;
        for(int i = 0; i < di.num_partitions; ++i) {
          if(di.partitions[i].number == info->part_num) {
            info->start = di.partitions[i].start * 512;
            info->size = di.partitions[i].size * 512;
            free_disk_info(&di);
            return 0;
          }
        }
        free_disk_info(&di);
        return 3;
      } else if (type == IMG_PART_NUM) return 2;
    } else if(type == IMG_PART_NUM) return 1;
    return 0;
  }
}

void usage() {
  fprintf(stderr,"%s - Version %s\n",APPNAME,VERSION);
  fprintf(stderr,"%s\n\n",COPYRIGHT);
  fprintf(stderr,"Usage: %s version\n",APPNAME);
  fprintf(stderr,"Usage: %s FILE create BYTES\n",APPNAME);
  fprintf(stderr,"Usage: %s FILE create-vmdk VMDK_FILE\n",APPNAME);
  fprintf(stderr,"Usage: %s FILE create-gpt\n",APPNAME);
  fprintf(stderr,"Usage: %s FILE create-part [ PARTNUM  [ START  [ SIZE [ LABEL ] ] ] ]\n",APPNAME);
  fprintf(stderr,"Usage: %s FILE part-info\n",APPNAME);
  fprintf(stderr,"Usage: %s FILE install-mbr\n",APPNAME);
  fprintf(stderr,"Usage: %s FILE:PARTNUM set-bootable true/false\n",APPNAME);
  fprintf(stderr,"Usage: %s FILE[:PART] mkfs [ LABEL ]\n",APPNAME);
  fprintf(stderr,"Usage: %s FILE[:PART] ls\n",APPNAME);
  fprintf(stderr,"Usage: %s FILE[:PART] rm FILE\n",APPNAME);
  fprintf(stderr,"Usage: %s FILE[:PART] read FILE\n",APPNAME);
  fprintf(stderr,"Usage: %s FILE[:PART] write FILE MODE\n",APPNAME);
  fprintf(stderr,"Usage: %s FILE[:PART] syslinux\n",APPNAME);

  fprintf(stderr,"\n  PART may either be a single number indicating GPT part number or a\n");
  fprintf(stderr,"  comma separated byte range\n");
  fprintf(stderr,"\n  All partition sizes are in 512 byte sectors. All paths must be absolute.\n");

  exit(1);
}

#define PARSE_IMG(type) parse_image_path(&image,argv[1],type) == 0

int main(int argc, char** argv) {
  image_path_info image;
  if(argc == 2 && strcmp("version",argv[1]) == 0) {
    printf("%s\n",VERSION);
    return 0;
  }
  if(!(argc > 2 && (
          (strcmp("create",argv[2]) == 0 && argc == 4 && PARSE_IMG(IMG_WHOLE_FILE)) ||
          (strcmp("create-gpt",argv[2]) == 0 && argc == 3 && PARSE_IMG(IMG_WHOLE_FILE)) ||
          (strcmp("install-mbr",argv[2]) == 0 && argc == 3 && PARSE_IMG(IMG_WHOLE_FILE)) ||
          (strcmp("create-vmdk",argv[2]) == 0 && argc == 4 && PARSE_IMG(IMG_WHOLE_FILE)) ||
          (strcmp("create-part",argv[2]) == 0 && (argc >= 3 && argc <= 7) && PARSE_IMG(IMG_WHOLE_FILE)) ||
          (strcmp("set-bootable",argv[2]) == 0 && argc == 4 && PARSE_IMG(IMG_PART_NUM)) ||
          (strcmp("part-info",argv[2]) == 0 && argc == 3 && PARSE_IMG(IMG_WHOLE_FILE)) ||
          (strcmp("syslinux",argv[2]) == 0 && argc == 3 && PARSE_IMG(IMG_PART_RANGE)) ||
          (strcmp("mkfs",argv[2]) == 0 && (argc == 3 || argc == 4) && PARSE_IMG(IMG_PART_RANGE)) ||
          (strcmp("ls",argv[2]) == 0 && argc == 3 && PARSE_IMG(IMG_PART_RANGE)) ||
          (strcmp("rm",argv[2]) == 0 && argc == 4 && PARSE_IMG(IMG_PART_RANGE)) ||
          (strcmp("read",argv[2]) == 0 && argc == 4 && PARSE_IMG(IMG_PART_RANGE)) ||
          (strcmp("write",argv[2]) == 0 && argc == 5 && PARSE_IMG(IMG_PART_RANGE))
    ))) {
    usage();
  }
  int buf_size = 20000000;
  char* buf = NULL;

  if(strcmp("create",argv[2]) == 0) {
    long long size = atoll(argv[3]);
    if(size < 1) return 1;
    if(size % 512 > 0) size+= (512-size%512);
    int fd = open(argv[1],O_TRUNC | O_CREAT | O_WRONLY, 0666);
    if(fd < 0) return 2;
    if(ftruncate(fd,size) < 0) return 3;
    close(fd);
    return 0;
  } else if(strcmp("create-vmdk",argv[2]) == 0) {
    FILE* vmdk = fopen(argv[3],"w");
    if(!vmdk) return 2;
    uint32_t cid = rand();
    struct stat st;
    if(stat(argv[1],&st) < 0) return 3;
    char* path;
    relative_path(&path,argv[3],argv[1]);
    fprintf(vmdk,vmdk_template,cid,st.st_size/512,path,st.st_size/(512*63*16));
    free(path);
    fclose(vmdk);
    return 0;
  } else if(strcmp("create-gpt",argv[2]) == 0) {
    return create_gpt_table(image.path);
  } else if (strcmp("create-part",argv[2]) == 0) {
    int part_num = 0;
    uint64_t start = 0;
    uint64_t size = 0;
    char* label = NULL;
    switch(argc) {
      case 7: label = argv[6];
      case 6: size = atoll(argv[5]);
      case 5: start = atoll(argv[4]);
      case 4: part_num = atoll(argv[3]);
    }
    return create_partition(image.path,part_num,start,size,label);
  } else if (strcmp("set-bootable",argv[2]) == 0) {
    bool value;
    if (strcmp("true",argv[3]) == 0) {
      value = true;
    } else if (strcmp("false",argv[3]) == 0) {
      value = false;
    } else {
      usage();
    }
    return set_partition_bootable(image.path,image.part_num,value);
  } else if (strcmp("part-info",argv[2]) == 0) {
    int result;
    disk_info di;
    result = collect_disk_info(&di,image.path);
    if(result != 0) return result;
    print_disk_info(&di);
    free_disk_info(&di);
    return 0;
  } else if (strcmp("install-mbr",argv[2]) == 0) {
    int fd = open(argv[1],O_WRONLY);
    if(fd < 0) return 1;
    if(write(fd,gptmbr_bin,gptmbr_bin_len) < gptmbr_bin_len) return 2;
    close(fd);
    return 0;
  }

  file_dev_name_set(image.path);
  struct ext4_blockdev* bd = file_dev_get();
  struct ext4_blockdev part = {0};
  if(image.size > 0) {
    part.bdif = bd->bdif;
    part.part_offset = image.start;
    part.part_size = image.size;
    bd = &part;
  }

  if(strcmp("mkfs",argv[2]) == 0)
  {
    struct ext4_fs fs = {0};
    struct ext4_mkfs_info info = {0};
    if(argc > 3) {
      info.label = argv[3];
    }
    if(image.size > 0) {
      info.len = part.part_size;
    }
    return ext4_mkfs(&fs, bd, &info, F_SET_EXT4);
  }

  if(ext4_device_register(bd,"image")) return 2;
  if(ext4_mount("image","/",false) != EOK) return 3;

  if(strcmp("syslinux",argv[2]) == 0) {
    int result = syslinux(bd);
    if(result != 0) return result;
  } else if(strcmp("ls",argv[2]) == 0) {
    list_dir("/",1);
  } else if(strcmp("rm",argv[2]) == 0) {
    ext4_fremove(argv[3]);
    char* end;
    while(end = strrchr(argv[3]+1,'/'), end != NULL) {
      *end = '\0';
      if(!dir_is_empty(argv[3])) break;
      ext4_dir_rm(argv[3]);
    }
  } else {
    ext4_file f;
    buf = malloc(buf_size);
    if(strcmp("read",argv[2]) == 0) {
      if(ext4_fopen(&f,argv[3],"r") != EOK) return 4;
      size_t written;
      if(ext4_fread(&f,buf,buf_size,&written) != EOK) return 5;
      while(written > 0) {
        if(fwrite(buf,1,written,stdout) == 0) return 7;
        if(ext4_fread(&f,buf,buf_size,&written) != EOK) return 6;
      }
    } else if(strcmp("write",argv[2]) == 0){
      uint32_t mode;
      if(sscanf(argv[4],"%o",&mode) != 1) return 10;
      uint32_t dir_mode = mode;
      char* end = argv[3];
      if(mode & 0600) dir_mode |= 0100;
      if(mode & 0060) dir_mode |= 0010;
      if(mode & 0006) dir_mode |= 0001;
      while(end = strchr(end+1,'/'), end != NULL) {
        *end = '\0';
        ext4_dir_mk(argv[3]);
        ext4_mode_set(argv[3],dir_mode);
        *end = '/';
      }
      if(ext4_fopen(&f,argv[3],"w") != EOK) return 8;
      int written;
      while((written = fread(buf,1,buf_size,stdin)) > 0) {
        if(ext4_fwrite(&f,buf,written,NULL) != EOK) return 9;
      }
      ext4_mode_set(argv[3],mode);
    }
    if(ext4_fclose(&f)) return 11;
  }
  if(ext4_umount("/") != EOK) return 12;
  free(buf);
}
