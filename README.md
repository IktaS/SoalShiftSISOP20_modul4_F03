# SoalShiftSISOP20_modul4_F03

Pada soal, diminta untuk membuat sebuah program yang dapat menangani filesystem sesuai rancangan.
Pada bagian awal program, terdapat bagian:
```
static const char *dirpath = "/home/ikta/Documents";
static const char *logpath = "/home/ikta/fs.log";
static char lastCommand[1000];
```
Bagian `*dirpath` dan `*logpath` ini perlu disesuaikan dengan direkori masing-masing pengguna agar program dapat berjalan dengan baik, yaitu dengan format:
```
"/home/[user]/Documents"
```
Dan
```
"/home/[user]/fs.log"

```
Berikut adalah isi dari fungsi main:
```
int main(int argc, char *argv[])
{
	umask(0);
	return fuse_main(argc, argv, &xmp_oper, NULL);
}
```
`umask(0)` akan memperbolehkan read, write, dan execute permission untuk semua.\
`xmp_oper` adalah `struct` yang berisi:
```
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
```
Atribut pada `struct` tersebut adalah pointer yang menuju ke fungsi.
Setelah program dicompile dengan:
```
gcc -Wall `pkg-config fuse --cflags` ssfs.c -o ssfs `pkg-config fuse --libs`

```
, akan didapatkan file bernama `ssfs`.
File `ssfs` ini kemudian dijalankan dengan
```
./ssfs [file atau direktori tujuan]
```
,dengan file tersebut terletak di lokasi yang sama dengan `ssfs`. Apabila berhasil, file tersebut kini akan tersambung ke direktori Documents.\
Pada ` struct fuse_operations xmp_oper`, terdapat banyak fungsi yang ditunjuk. Berikut adalah fungsi-fungsi yang ada pada `struct` tersebut:\
Xmp_getattr
```
static int xmp_getattr(const char *path, struct stat *stbuf)
{
	int res;
	char * encv1 = strstr(path,"encv1_");
	char * encv2 = strstr(path,"encv2_");
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

	if(encv2 == NULL){
		res = lstat(fpath,stbuf);
		if(res == -1){
			return -errno;
		}
	}else{
		sprintf(fpath,"%s%s.000",dirpath,path);
		lstat(fpath,stbuf);
		struct stat st;
		int i=0;
		int sizeCount = 0;
		while(1){
			res = stat(fpath,&st);
			if(res < 0){
				break;
			}
			i++;
			sprintf(fpath,"%s%s.%03d",dirpath,path,i);
			sizeCount += st.st_size;
		}
		stbuf->st_size = sizeCount;
	}

	return 0;
}
```
Di bagian awal, terdapat `char * encv1 = strstr(path,"encv1_");`. Bagian ini berfungsi untuk mencari apakah path mempunyai "encv1_" di dalamnya. Apabila path tidak memiliki nama tersebut, maka null akan direturn. Apabila nilai yang direturn tidak null, bagian:
```
if(encv1 != NULL && strcmp(lastCommand,"readdir") == 0){
		getDecrypted1String(encv1);
		printf("debug dec getattr path : %s\n",path);
	}
```
akan menjalankan fungsi:
```
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
```
yang mungkin memanggil:
```
void decrypt1string(char * in,int stop){
	if(strcmp(in,".") == 0 || strcmp(in,"..") == 0) return;
	for(int i=0;i<stop-1;i++){
		in[i] = getDecrypted1Char(in[i]);
	}
}
```
yang kemudian memanggil:
```
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
```
yang berfungsi melakukan dekripsi caesar cipher seperti yang diminta pada soal nomor 1.\
Bagian-bagian di atas akan dijumpai pula pada fungsi-fungsi lainnya.\
Xmp_getattr berfungsi mendapatkan atribut file.\
Xmp_access
```
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
```
Xmp_readlink
```
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
```
Xmp_readdir
```
static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
	int res;
	DIR *dp;
	struct dirent *de;

	(void) offset;
	(void) fi;

	// printf("debug init readdir path : %s\n",path);
	char * encv1 = strstr(path,"encv1_");
	char * encv2 = strstr(path,"encv2_");
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
		if(encv2 != NULL){
			if(!is_regular_file(de->d_name)){
				if(strcmp(de->d_name+(strlen(de->d_name)-4),".000") == 0){
					*(de->d_name + +(strlen(de->d_name)-4)) = '\0';
					res = (filler(buf,de->d_name,&st,0));
				}
			}
		}else{ 
			if(encv1 != NULL){
				getEncrypted1String(de->d_name);
			}
			res = (filler(buf,de->d_name,&st,0));
		}

		if (res)
			break;
	}

	closedir(dp);
	return 0;
}
```
Pada bagian ini, terdapat:
```
if(encv1 != NULL){
	getEncrypted1String(de->d_name);
}
```
yang berarti apabila lokasi mempunyai "encv1_", fungsi `getEncrypted1String(de->d_name)` akan dipanggil:
```
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
```
yang mungkin akan memanggil fungsi:
```
void encrypt1string(char * in,int stop){
	if(strcmp(in,".") == 0 || strcmp(in,"..") == 0) return;
	for(int i=0;i<stop;i++){
		in[i] = getEncrypted1Char(in[i]);
	}
}
```
yang akan memanggil:
```
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
```
untuk melakukan enkripsi caesar cipher seperti yang diminta pada soal nomor 1.\
Xmp_readdir berfungsi untuk membaca direktori.\
Xmp_mknod
```
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
```
Xmp_mkdir
```
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
```
Xmp_symlink
```
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
```
Xmp_unlink
```
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
```
Xmp_rmdir
```
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
```
Xmp_rename
```
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
```
Xmp_link
```
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
```
Xmp_chmod
```
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
```
Xmp_chown
```
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
```
Xmp_truncate
```
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
```
Xmp_utimens
```
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
```
Xmp_open
```
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
```
Xmp_read
```
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
```
Xmp_write
```
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
```
Xmp_statfs
```
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
```
Xmp_create
```
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
```
Xmp_release
```
static int xmp_release(const char *path, struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */

	(void) path;
	(void) fi;
	return 0;
}
```
Xmp_fsync
```
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
```
Xmp_setxattr
```
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
```
Xmp_getxattr
```
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
```
Xmp_listxattr
```
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
```
Xmp_removexattr
```
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
```
 \
Apabila diperhatikan, banyak fungsi di atas yang memanggil fungsi:
```
void printInfo(char * args){
	char message[10000],timestamp[40];
	getTime(timestamp);
	sprintf(message,"%s::%s::%s","INFO",timestamp,args);
	printlog(message);
}
```
atau
```
void printWarning(char * args){
	char message[10000],timestamp[40];
	getTime(timestamp);
	sprintf(message,"%s::%s::%s","WARNING",timestamp,args);
	printlog(message);
}
```
Ke-2 fungsi `printInfo` dan `printWarning` memiliki tujuan yang serupa, yaitu untuk mencetak aktivitas dalam log sesuai dengan soal nomor 4. Ke-2 fungsi tersebut akan memanggil:
```
void printlog(char * args){
	FILE* log;
	log = fopen(logpath,"a+");
	fprintf(log,"%s\n",args);
	fclose(log);
}
```
untuk mencatat aktivitas pada file `fs.log` dengan bantuan fungsi:
```
void getTime(char * dest){
    char buffer[10000];
    memset(buffer,0,sizeof(buffer));
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    strftime(buffer,sizeof(buffer),"%y%m%d-%X",&tm);
    strcpy(dest,buffer);
}
```
Untuk mendapatkan waktu saat fungsi dijalankan.\
Sesuai perintah soal, `printWarning` dipanggil pada fungsi syscall rmdir(xmp_rmdir) dan unlink(xmp_unlink), dan `printInfo` pada fungsi lainnya.


# Permasalahan yang dihadapi
-Kurangnya dokumentasi mengenai Fuse serta kesulitan pencarian penjelasan Fuse yang jelas dan lengkap
