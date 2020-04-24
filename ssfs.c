/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  Minor modifications and note by Andy Sayler (2012) <www.andysayler.com>

  Source: fuse-2.8.7.tar.gz examples directory
  http://sourceforge.net/projects/fuse/files/fuse-2.X/

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall `pkg-config fuse --cflags` fusexmp.c -o fusexmp `pkg-config fuse --libs`

  Note: This implementation is largely stateless and does not maintain
        open file handels between open and release calls (fi->fh).
        Instead, files are opened and closed as necessary inside read(), write(),
        etc calls. As such, the functions that rely on maintaining file handles are
        not implmented (fgetattr(), etc). Those seeking a more efficient and
        more complete implementation may wish to add fi->fh support to minimize
        open() and close() calls and support fh dependent functions.

*/

#define FUSE_USE_VERSION 28
#define HAVE_SETXATTR

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
/* For pread()/pwrite() */
#define _XOPEN_SOURCE 500
#endif

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif
#include <limits.h>
#include <time.h>
#include <stdlib.h>

static const char *dirpath = "/home/ikta/Documents";
static const char *logpath = "/home/ikta/fs.log";
static char lastCommand[1000];

int is_regular_file( char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}

char getEncrypted1Char(char in){
	char keystring[] = "9(ku@AW1[Lmvgax6q`5Y2Ry?+sF!^HKQiBXCUSe&0M.b%rI'7d)o4~VfZ*{#:}ETt$3J-zpc]lnh8,GwP_ND|jO";
	int key = 10;
	for(int i = 0;i<strlen(keystring);i++){
		if(in == keystring[i]){
			return keystring[(i+key)%strlen(keystring)];
		}
	}
    return in;
}

char getDecrypted1Char(char in){
	char keystring[] = "9(ku@AW1[Lmvgax6q`5Y2Ry?+sF!^HKQiBXCUSe&0M.b%rI'7d)o4~VfZ*{#:}ETt$3J-zpc]lnh8,GwP_ND|jO";
	int key = strlen(keystring) - 10;
	for(int i = 0;i<strlen(keystring);i++){
		if(in == keystring[i]){
			return keystring[(i+key)%strlen(keystring)];
		}
	}
    return in;
}

void encrypt1string(char * in,int stop){
	if(strcmp(in,".") == 0 || strcmp(in,"..") == 0) return;
	for(int i=0;i<stop;i++){
		in[i] = getEncrypted1Char(in[i]);
	}
}

void decrypt1string(char * in,int stop){
	if(strcmp(in,".") == 0 || strcmp(in,"..") == 0) return;
	for(int i=0;i<stop-1;i++){
		in[i] = getDecrypted1Char(in[i]);
	}
}

void getEncrypted1String(char * string){
	if(strcmp(string,".") == 0 || strcmp(string,"..") == 0) return;
	int stop = strlen(string);
	for(int i=stop;i>=0;i--){
		if(string[i] == '/') break;
		if(string[i] == '.'){
			stop = i;
			break;
		}
	}
	encrypt1string(string,stop);
}

void getDecrypted1String(char * string){
	if(strcmp(string,".") == 0 || strcmp(string,"..") == 0) return;
	char * slash = strstr(string,"/");
	if(slash != NULL) {
		int stop = strlen(slash);
		for(int i=stop;i>=0;i--){
			if(slash[i] == '/') break;
			if(slash[i] == '.'){
				stop = i;
				break;
			}
		}
		decrypt1string(slash+1,stop);
	}
}

void getTime(char * dest){
    char buffer[10000];
    memset(buffer,0,sizeof(buffer));
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    strftime(buffer,sizeof(buffer),"%y%m%d-%X",&tm);
    strcpy(dest,buffer);
}

void printlog(char * args){
	FILE* log;
	log = fopen(logpath,"a+");
	fprintf(log,"%s\n",args);
	fclose(log);
}

void printInfo(char * args){
	char message[10000],timestamp[40];
	getTime(timestamp);
	sprintf(message,"%s::%s::%s","INFO",timestamp,args);
	printlog(message);
}

void printWarning(char * args){
	char message[10000],timestamp[40];
	getTime(timestamp);
	sprintf(message,"%s::%s::%s","WARNING",timestamp,args);
	printlog(message);
}

static int xmp_getattr(const char *path, struct stat *stbuf)
{
	int res;
	char * encv1 = strstr(path,"encv1_");
	if(encv1 != NULL && strcmp(lastCommand,"readdir") == 0){
		getDecrypted1String(encv1);
		printf("debug dec getattr path : %s\n",path);
	}

	char fpath[PATH_MAX];
	if(strcmp(path,"/") == 0){
        path=dirpath;
        sprintf(fpath,"%s",path);
    }else{
        sprintf(fpath, "%s%s",dirpath,path);
    }
	char logbuffer[1000];
	sprintf(logbuffer,"%s::%s","GETATTR",path);
	printInfo(logbuffer);

	res = lstat(fpath, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_access(const char *path, int mask)
{
	int res;
	char * encv1 = strstr(path,"encv1_");
	if(encv1 != NULL && strcmp(lastCommand,"readdir") == 0){
		getDecrypted1String(encv1);
		printf("debug dec getattr path : %s\n",path);
	}
	char fpath[PATH_MAX];
	if(strcmp(path,"/") == 0){
        path=dirpath;
        sprintf(fpath,"%s",path);
    }else{
        sprintf(fpath, "%s%s",dirpath,path);
    }
	char logbuffer[1000];
	sprintf(logbuffer,"%s::%s","ACCESS",path);
	printInfo(logbuffer);
	strcpy(lastCommand,"access");

	res = access(fpath, mask);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_readlink(const char *path, char *buf, size_t size)
{
	int res;
	char * encv1 = strstr(path,"encv1_");
	if(encv1 != NULL && strcmp(lastCommand,"readdir") == 0){
		getDecrypted1String(encv1);
		printf("debug dec getattr path : %s\n",path);
	}
	char fpath[PATH_MAX];
	if(strcmp(path,"/") == 0){
        path=dirpath;
        sprintf(fpath,"%s",path);
    }else{
        sprintf(fpath, "%s%s",dirpath,path);
    }
	char logbuffer[1000];
	sprintf(logbuffer,"%s::%s","READLINK",path);
	printInfo(logbuffer);
	strcpy(lastCommand,"readlink");

	res = readlink(fpath, buf, size - 1);
	if (res == -1)
		return -errno;

	buf[res] = '\0';
	return 0;
}


static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
	DIR *dp;
	struct dirent *de;

	(void) offset;
	(void) fi;

	// printf("debug init readdir path : %s\n",path);
	char * encv1 = strstr(path,"encv1_");
	if(encv1 != NULL && strcmp(lastCommand,"readdir") == 0){
		getDecrypted1String(encv1);
		printf("debug dec getattr path : %s\n",path);
	}	// printf("debug enc readdir path : %s\n",path);


	char fpath[PATH_MAX];
	if(strcmp(path,"/") == 0){
        path=dirpath;
        sprintf(fpath,"%s",path);
    }else{
        sprintf(fpath, "%s%s",dirpath,path);
    }
	char logbuffer[1000];
	sprintf(logbuffer,"%s::%s","READDIR",path);
	printInfo(logbuffer);
	memset(lastCommand,0,sizeof(lastCommand));
	strcpy(lastCommand,"readdir");

	dp = opendir(fpath);
	if (dp == NULL)
		return -errno;

	while ((de = readdir(dp)) != NULL) {
		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = de->d_ino;
		st.st_mode = de->d_type << 12;

		// printf("debug dename init : %s\n",de->d_name);
		if(encv1 != NULL){
			getEncrypted1String(de->d_name);
		}
		// printf("debug dename enc : %s\n",de->d_name);

		if (filler(buf, de->d_name, &st, 0))
			break;
	}

	closedir(dp);
	return 0;
}

static int xmp_mknod(const char *path, mode_t mode, dev_t rdev)
{
	int res;

	char fpath[PATH_MAX];
	if(strcmp(path,"/") == 0){
        path=dirpath;
        sprintf(fpath,"%s",path);
    }else{
        sprintf(fpath, "%s%s",dirpath,path);
    }
	char logbuffer[1000];
	sprintf(logbuffer,"%s::%s","MKNOD",path);
	printInfo(logbuffer);
	memset(lastCommand,0,sizeof(lastCommand));
	strcpy(lastCommand,"mknod");

	/* On Linux this could just be 'mknod(path, mode, rdev)' but this
	   is more portable */
	if (S_ISREG(mode)) {
		res = open(fpath, O_CREAT | O_EXCL | O_WRONLY, mode);
		if (res >= 0)
			res = close(res);
	} else if (S_ISFIFO(mode))
		res = mkfifo(fpath, mode);
	else
		res = mknod(fpath, mode, rdev);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_mkdir(const char *path, mode_t mode)
{
	int res;

	char fpath[PATH_MAX];
	if(strcmp(path,"/") == 0){
        path=dirpath;
        sprintf(fpath,"%s",path);
    }else{
        sprintf(fpath, "%s%s",dirpath,path);
    }
	char logbuffer[1000];
	sprintf(logbuffer,"%s::%s","MKDIR",path);
	printInfo(logbuffer);
	memset(lastCommand,0,sizeof(lastCommand));
	strcpy(lastCommand,"mkdir");

	res = mkdir(fpath, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_unlink(const char *path)
{
	int res;
	char * encv1 = strstr(path,"encv1_");
	if(encv1 != NULL && strcmp(lastCommand,"readdir") == 0){
		getDecrypted1String(encv1);
		printf("debug dec getattr path : %s\n",path);
	}
	char fpath[PATH_MAX];
	if(strcmp(path,"/") == 0){
        path=dirpath;
        sprintf(fpath,"%s",path);
    }else{
        sprintf(fpath, "%s%s",dirpath,path);
    }
	char logbuffer[1000];
	sprintf(logbuffer,"%s::%s","UNLINK",path);
	printWarning(logbuffer);
	memset(lastCommand,0,sizeof(lastCommand));
	strcpy(lastCommand,"unlink");


	res = unlink(fpath);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_rmdir(const char *path)
{
	int res;
	char * encv1 = strstr(path,"encv1_");
	if(encv1 != NULL && strcmp(lastCommand,"readdir") == 0){
		getDecrypted1String(encv1);
		printf("debug dec getattr path : %s\n",path);
	}
	char fpath[PATH_MAX];
	if(strcmp(path,"/") == 0){
        path=dirpath;
        sprintf(fpath,"%s",path);
    }else{
        sprintf(fpath, "%s%s",dirpath,path);
    }
	char logbuffer[1000];
	sprintf(logbuffer,"%s::%s","RMDIR",path);
	printWarning(logbuffer);
	memset(lastCommand,0,sizeof(lastCommand));
	strcpy(lastCommand,"rmdir");


	res = rmdir(fpath);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_symlink(const char *from, const char *to)
{
	int res;

	char logbuffer[1000];
	sprintf(logbuffer,"%s::%s::%s","SYMLINK",from,to);
	printInfo(logbuffer);
	memset(lastCommand,0,sizeof(lastCommand));
	strcpy(lastCommand,"symlink");

	res = symlink(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_rename(const char *from, const char *to)
{
	int res;

	char logbuffer[1000];
	sprintf(logbuffer,"%s::%s::%s","RENAME",from,to);
	printInfo(logbuffer);
	memset(lastCommand,0,sizeof(lastCommand));
	strcpy(lastCommand,"rename");

	res = rename(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_link(const char *from, const char *to)
{
	int res;

	char logbuffer[1000];
	sprintf(logbuffer,"%s::%s::%s","LINK",from,to);
	printInfo(logbuffer);
	memset(lastCommand,0,sizeof(lastCommand));
	strcpy(lastCommand,"link");

	res = link(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_chmod(const char *path, mode_t mode)
{
	int res;
	char * encv1 = strstr(path,"encv1_");
	if(encv1 != NULL && strcmp(lastCommand,"readdir") == 0){
		getDecrypted1String(encv1);
		printf("debug dec getattr path : %s\n",path);
	}
	char fpath[PATH_MAX];
	if(strcmp(path,"/") == 0){
        path=dirpath;
        sprintf(fpath,"%s",path);
    }else{
        sprintf(fpath, "%s%s",dirpath,path);
    }
	char logbuffer[1000];
	sprintf(logbuffer,"%s::%s","CHMOD",path);
	printInfo(logbuffer);
	memset(lastCommand,0,sizeof(lastCommand));
	strcpy(lastCommand,"chmod");


	res = chmod(fpath, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_chown(const char *path, uid_t uid, gid_t gid)
{
	int res;
	char * encv1 = strstr(path,"encv1_");
	if(encv1 != NULL && strcmp(lastCommand,"readdir") == 0){
		getDecrypted1String(encv1);
		printf("debug dec getattr path : %s\n",path);
	}
	char fpath[PATH_MAX];
	if(strcmp(path,"/") == 0){
        path=dirpath;
        sprintf(fpath,"%s",path);
    }else{
        sprintf(fpath, "%s%s",dirpath,path);
    }
	char logbuffer[1000];
	sprintf(logbuffer,"%s::%s","CHOWN",path);
	printInfo(logbuffer);
	memset(lastCommand,0,sizeof(lastCommand));
	strcpy(lastCommand,"chown");


	res = lchown(fpath, uid, gid);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_truncate(const char *path, off_t size)
{
	int res;
	// char * encv1 = strstr(path,"encv1_");
	// if(encv1 != NULL){
	// 	getDecrypted1String(encv1);
	// }

	char fpath[PATH_MAX];
	if(strcmp(path,"/") == 0){
        path=dirpath;
        sprintf(fpath,"%s",path);
    }else{
        sprintf(fpath, "%s%s",dirpath,path);
    }
	char logbuffer[1000];
	sprintf(logbuffer,"%s::%s","TRUNCATE",path);
	printInfo(logbuffer);
	memset(lastCommand,0,sizeof(lastCommand));
	strcpy(lastCommand,"truncate");


	res = truncate(fpath, size);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_utimens(const char *path, const struct timespec ts[2])
{
	int res;
	// char * encv1 = strstr(path,"encv1_");
	// if(encv1 != NULL){
	// 	getDecrypted1String(encv1);
	// }

	char fpath[PATH_MAX];
	if(strcmp(path,"/") == 0){
        path=dirpath;
        sprintf(fpath,"%s",path);
    }else{
        sprintf(fpath, "%s%s",dirpath,path);
    }
	char logbuffer[1000];
	sprintf(logbuffer,"%s::%s","UTIMENS",path);
	printInfo(logbuffer);
	memset(lastCommand,0,sizeof(lastCommand));
	strcpy(lastCommand,"utimens");

	struct timeval tv[2];

	tv[0].tv_sec = ts[0].tv_sec;
	tv[0].tv_usec = ts[0].tv_nsec / 1000;
	tv[1].tv_sec = ts[1].tv_sec;
	tv[1].tv_usec = ts[1].tv_nsec / 1000;

	res = utimes(fpath, tv);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi)
{
	int res;
	char * encv1 = strstr(path,"encv1_");
	if(encv1 != NULL && strcmp(lastCommand,"readdir") == 0){
		getDecrypted1String(encv1);
		printf("debug dec getattr path : %s\n",path);
	}
	char fpath[PATH_MAX];
	if(strcmp(path,"/") == 0){
        path=dirpath;
        sprintf(fpath,"%s",path);
    }else{
        sprintf(fpath, "%s%s",dirpath,path);
    }
	char logbuffer[1000];
	sprintf(logbuffer,"%s::%s","OPEN",path);
	printInfo(logbuffer);
	memset(lastCommand,0,sizeof(lastCommand));
	strcpy(lastCommand,"open");


	res = open(fpath, fi->flags);
	if (res == -1)
		return -errno;

	close(res);
	return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	int fd;
	int res;
	char * encv1 = strstr(path,"encv1_");
	if(encv1 != NULL && strcmp(lastCommand,"readdir") == 0){
		getDecrypted1String(encv1);
		printf("debug dec getattr path : %s\n",path);
	}
	char fpath[PATH_MAX];
	if(strcmp(path,"/") == 0){
        path=dirpath;
        sprintf(fpath,"%s",path);
    }else{
        sprintf(fpath, "%s%s",dirpath,path);
    }
	char logbuffer[1000];
	sprintf(logbuffer,"%s::%s","READ",path);
	printInfo(logbuffer);
	memset(lastCommand,0,sizeof(lastCommand));
	strcpy(lastCommand,"read");


	(void) fi;
	fd = open(fpath, O_RDONLY);
	if (fd == -1)
		return -errno;

	res = pread(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	close(fd);
	return res;
}

static int xmp_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	int fd;
	int res;
	char * encv1 = strstr(path,"encv1_");
	if(encv1 != NULL && strcmp(lastCommand,"readdir") == 0){
		getDecrypted1String(encv1);
		printf("debug dec getattr path : %s\n",path);
	}
	char fpath[PATH_MAX];
	if(strcmp(path,"/") == 0){
        path=dirpath;
        sprintf(fpath,"%s",path);
    }else{
        sprintf(fpath, "%s%s",dirpath,path);
    }
	char logbuffer[1000];
	sprintf(logbuffer,"%s::%s","WRITE",path);
	printInfo(logbuffer);
	memset(lastCommand,0,sizeof(lastCommand));
	strcpy(lastCommand,"write");


	(void) fi;
	fd = open(fpath, O_WRONLY);
	if (fd == -1)
		return -errno;

	res = pwrite(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	close(fd);
	return res;
}

static int xmp_statfs(const char *path, struct statvfs *stbuf)
{
	int res;
	char * encv1 = strstr(path,"encv1_");
	if(encv1 != NULL && strcmp(lastCommand,"readdir") == 0){
		getDecrypted1String(encv1);
		printf("debug dec getattr path : %s\n",path);
	}
	char fpath[PATH_MAX];
	if(strcmp(path,"/") == 0){
        path=dirpath;
        sprintf(fpath,"%s",path);
    }else{
        sprintf(fpath, "%s%s",dirpath,path);
    }
	char logbuffer[1000];
	sprintf(logbuffer,"%s::%s","STATFS",path);
	printInfo(logbuffer);
	memset(lastCommand,0,sizeof(lastCommand));
	strcpy(lastCommand,"statfs");


	res = statvfs(fpath, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_create(const char* path, mode_t mode, struct fuse_file_info* fi) {

    (void) fi;
	// char * encv1 = strstr(path,"encv1_");
	// if(encv1 != NULL){
	// 	getDecrypted1String(encv1);
	// }

	char fpath[PATH_MAX];
	if(strcmp(path,"/") == 0){
        path=dirpath;
        sprintf(fpath,"%s",path);
    }else{
        sprintf(fpath, "%s%s",dirpath,path);
    }
	char logbuffer[1000];
	sprintf(logbuffer,"%s::%s","CREATE",path);
	printInfo(logbuffer);
	memset(lastCommand,0,sizeof(lastCommand));
	strcpy(lastCommand,"create");


    int res;
    res = creat(fpath, mode);
    if(res == -1)
	return -errno;

    close(res);

    return 0;
}


static int xmp_release(const char *path, struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */

	(void) path;
	(void) fi;
	return 0;
}

static int xmp_fsync(const char *path, int isdatasync,
		     struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */

	(void) path;
	(void) isdatasync;
	(void) fi;
	return 0;
}

#ifdef HAVE_SETXATTR
static int xmp_setxattr(const char *path, const char *name, const char *value,
			size_t size, int flags)
{
	char * encv1 = strstr(path,"encv1_");
	if(encv1 != NULL && strcmp(lastCommand,"readdir") == 0){
		getDecrypted1String(encv1);
		printf("debug dec getattr path : %s\n",path);
	}
	char fpath[PATH_MAX];
	if(strcmp(path,"/") == 0){
        path=dirpath;
        sprintf(fpath,"%s",path);
    }else{
        sprintf(fpath, "%s%s",dirpath,path);
    }
	char logbuffer[1000];
	sprintf(logbuffer,"%s::%s","SETXATTR",path);
	printInfo(logbuffer);
	memset(lastCommand,0,sizeof(lastCommand));
	strcpy(lastCommand,"SETXATTR");

	int res = lsetxattr(fpath, name, value, size, flags);
	if (res == -1)
		return -errno;
	return 0;
}

static int xmp_getxattr(const char *path, const char *name, char *value,
			size_t size)
{
	char * encv1 = strstr(path,"encv1_");
	if(encv1 != NULL && strcmp(lastCommand,"readdir") == 0){
		getDecrypted1String(encv1);
		printf("debug dec getattr path : %s\n",path);
	}
	char fpath[PATH_MAX];
	if(strcmp(path,"/") == 0){
        path=dirpath;
        sprintf(fpath,"%s",path);
    }else{
        sprintf(fpath, "%s%s",dirpath,path);
    }
	char logbuffer[1000];
	sprintf(logbuffer,"%s::%s","GETXATTR",path);
	printInfo(logbuffer);
	memset(lastCommand,0,sizeof(lastCommand));
	strcpy(lastCommand,"GETXATTR");

	int res = lgetxattr(fpath, name, value, size);
	if (res == -1)
		return -errno;
	return res;
}

static int xmp_listxattr(const char *path, char *list, size_t size)
{
	char * encv1 = strstr(path,"encv1_");
	if(encv1 != NULL && strcmp(lastCommand,"readdir") == 0){
		getDecrypted1String(encv1);
		printf("debug dec getattr path : %s\n",path);
	}
	char fpath[PATH_MAX];
	if(strcmp(path,"/") == 0){
        path=dirpath;
        sprintf(fpath,"%s",path);
    }else{
        sprintf(fpath, "%s%s",dirpath,path);
    }
	char logbuffer[1000];
	sprintf(logbuffer,"%s::%s","LISTXATTR",path);
	printInfo(logbuffer);
	memset(lastCommand,0,sizeof(lastCommand));
	strcpy(lastCommand,"LISTXATTR");

	int res = llistxattr(fpath, list, size);
	if (res == -1)
		return -errno;
	return res;
}

static int xmp_removexattr(const char *path, const char *name)
{
	char * encv1 = strstr(path,"encv1_");
	if(encv1 != NULL && strcmp(lastCommand,"readdir") == 0){
		getDecrypted1String(encv1);
		printf("debug dec getattr path : %s\n",path);
	}
	char fpath[PATH_MAX];
	if(strcmp(path,"/") == 0){
        path=dirpath;
        sprintf(fpath,"%s",path);
    }else{
        sprintf(fpath, "%s%s",dirpath,path);
    }
	char logbuffer[1000];
	sprintf(logbuffer,"%s::%s","REMOVEXATTR",path);
	printInfo(logbuffer);
	memset(lastCommand,0,sizeof(lastCommand));
	strcpy(lastCommand,"REMOVEXATTR");

	int res = lremovexattr(fpath, name);
	if (res == -1)
		return -errno;
	return 0;
}
#endif /* HAVE_SETXATTR */

static struct fuse_operations xmp_oper = {
	.getattr	= xmp_getattr,
	.access		= xmp_access,
	.readlink	= xmp_readlink,
	.readdir	= xmp_readdir,
	.mknod		= xmp_mknod,
	.mkdir		= xmp_mkdir,
	.symlink	= xmp_symlink,
	.unlink		= xmp_unlink,
	.rmdir		= xmp_rmdir,
	.rename		= xmp_rename,
	.link		= xmp_link,
	.chmod		= xmp_chmod,
	.chown		= xmp_chown,
	.truncate	= xmp_truncate,
	.utimens	= xmp_utimens,
	.open		= xmp_open,
	.read		= xmp_read,
	.write		= xmp_write,
	.statfs		= xmp_statfs,
	.create     = xmp_create,
	.release	= xmp_release,
	.fsync		= xmp_fsync,
#ifdef HAVE_SETXATTR
	.setxattr	= xmp_setxattr,
	.getxattr	= xmp_getxattr,
	.listxattr	= xmp_listxattr,
	.removexattr	= xmp_removexattr,
#endif
};

int main(int argc, char *argv[])
{
	umask(0);
	return fuse_main(argc, argv, &xmp_oper, NULL);
}
