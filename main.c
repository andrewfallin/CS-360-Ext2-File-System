#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <ext2fs/ext2_fs.h>

#include "type.h"
#include "util.h"
#include "util.c"
#include "ls_cd_pwd.c"
#include "get_ino.c"
#include "mkdir.c"
#include "creat.c"
#include "rmdir.c"
#include "link.c"
#include "unlink.c"
#include "touch.c"
#include "open_close.c"
#include "read.c"
#include "write.c"
#include "cat_cp.c"

MINODE *iget(int dev, int ino)
{
  //printf("iget(%d %d): ", dev, ino);
  return (MINODE *)kcwiget(dev, ino);
}

int iput(MINODE *mip)
{
  //printf("iput(%d %d)\n", mip->dev, mip->ino);
  return kcwiput(mip);
}

int getino(int dev, char *pathename)
{
  return kcwgetino(dev, pathename);
}

int init()
{
  int i, j;
  MINODE *mip;
  PROC   *p;

  printf("init()\n");

  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    mip->dev = mip->ino = 0;
    mip->refCount = 0;
    mip->mounted = 0;
    mip->mptr = 0;
  }
  for (i=0; i<NPROC; i++){
    p = &proc[i];
    p->pid = i;
    p->uid = 0;
    p->cwd = 0;
    for (j=0; j<NFD; j++)
      p->fd[j] = 0;
  }
}

// load root INODE and set root pointer to it
int mount_root()
{
  printf("mount_root()\n");
  root = iget(dev, 2);
  root->mounted = 1;
  root->mptr = &mntable;

  mntPtr = &mntable;
  mntPtr->dev = dev;
  mntPtr->ninodes = ninodes;
  mntPtr->nblocks = nblocks;
  mntPtr->bmap = bmap;
  mntPtr->imap = imap;
  mntPtr->iblk = iblk;
  mntPtr->mntDirPtr = root;
  strcpy(mntPtr->devName, "mydisk");
  strcpy(mntPtr->mntName, "/");
}

char *disk = "mydisk";

main(int argc, char *argv[ ])
{
  int ino;
  char buf[BLKSIZE];

  if (argc > 1)
    disk = argv[1];

  printf("checking EXT2 FS ....");
  if ((fd = open(disk, O_RDWR)) < 0){
    printf("open %s failed\n", disk);  exit(1);
  }
  dev = fd;

  /********** read super block at 1024 ****************/
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;

  /* verify it's an ext2 file system *****************/
  if (sp->s_magic != 0xEF53){
      printf("magic = %x is not an ext2 filesystem\n", sp->s_magic);
      exit(1);
  }
  printf("OK\n");
  ninodes = sp->s_inodes_count;
  nblocks = sp->s_blocks_count;

  get_block(dev, 2, buf);
  gp = (GD *)buf;

  bmap = gp->bg_block_bitmap;
  imap = gp->bg_inode_bitmap;
  iblk = gp->bg_inode_table;
  printf("bmp=%d imap=%d iblk = %d\n", bmap, imap, iblk);

  init();
  mount_root();
  printf("root refCount = %d\n", root->refCount);

  printf("creating P0 as running process\n");
  running = &proc[0];
  running->cwd = iget(dev, 2);
  printf("root refCount = %d\n", root->refCount);

  //printf("hit a key to continue : "); getchar();
  while(1){
    printf("**Commands**\n");
    printf("[ls|cd|pwd|mkdir|creat|rmdir|link|unlink|symlink|touch\n");
    printf("|chmod|stat|open|close|pfd|read|write|lseek|cat|cp|mv|quit]\n");
    printf("input command : ");
    fgets(line, 128, stdin);

    line[strlen(line)-1] = 0;

    if (line[0]==0)
      continue;
    pathname[0] = 0;
    link[0] = 0;

    sscanf(line, "%s %s %s", cmd, pathname, link);
    printf("cmd=%s pathname=%s link=%s\n", cmd, pathname, link);

    if(strcmp(cmd, "ls")==0)
      list_file();
    if(strcmp(cmd, "cd")==0)
      change_dir();
    if(strcmp(cmd, "pwd")==0)
      pwd(running->cwd);
    if(strcmp(cmd, "mkdir") == 0)
      make_dir();
    if(strcmp(cmd, "creat") == 0)
      creat_file();
    if(strcmp(cmd, "rmdir") == 0)
      remove_dir();
    if(strcmp(cmd, "link") == 0)
      mylink();
    if(strcmp(cmd, "unlink") == 0)
      myUnlink();
    if(strcmp(cmd, "symlink") == 0)
      mySymLink();
    if(strcmp(cmd, "touch") == 0)
      touch();
    if(strcmp(cmd, "chmod") == 0)
      my_chmod();
    if(strcmp(cmd, "stat") == 0)
      my_stat();
    if(strcmp(cmd, "rm") == 0)
      myUnlink();
    if(strcmp(cmd, "open") == 0)
      fd = open_file();
    if(strcmp(cmd, "pfd") == 0)
      pfd();
    if(strcmp(cmd, "close") == 0)
      close_file();
    if(strcmp(cmd, "read")==0)
      read_file();
    if(strcmp(cmd, "write") == 0)
      write_file();
    if(strcmp(cmd, "lseek") == 0)
      my_lseek();
    if(strcmp(cmd, "cat") == 0)
      cat();
    if(strcmp(cmd, "cp") == 0)
      my_cp();
    if(strcmp(cmd, "mv") == 0)
      mv();
    if(strcmp(cmd, "quit")==0)
      quit();
  }
}

int quit()
{
  int i;
  MINODE *mip;
  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount > 0)
      iput(mip);
  }
  exit(0);
}
