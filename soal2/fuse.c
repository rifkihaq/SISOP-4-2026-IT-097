#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <stdlib.h>
#include <limits.h>

static const char *storage_dir = "encrypted_storage";
static const unsigned char XOR_KEY = 0x76;

void xor_encrypt_decrypt(char *buf, size_t size) {
    for (size_t i = 0; i < size; i++) {
        buf[i] ^= XOR_KEY;
    }
}

void build_path(char fpath[PATH_MAX], const char *path) {
    strcpy(fpath, storage_dir);

    if (strcmp(path, "/") == 0) {
        return;
    }

    char temp[PATH_MAX];
    strcpy(temp, path);

    char *dot = strrchr(temp, '.');

    if (dot && strcmp(dot, ".enc") == 0) {
        strcat(fpath, temp);
    } else {
        strcat(temp, ".enc");
        strcat(fpath, temp);
    }
}

static int xmp_getattr(const char *path, struct stat *stbuf) {
    int res;
    char fpath[PATH_MAX];

    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0) {
        return lstat(storage_dir, stbuf);
    }

    build_path(fpath, path);

    res = lstat(fpath, stbuf);

    if (res == -1)
        return -errno;

    return 0;
}

static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi) {

    DIR *dp;
    struct dirent *de;
    char fpath[PATH_MAX];

    (void) offset;
    (void) fi;

    if (strcmp(path, "/") == 0) {
        strcpy(fpath, storage_dir);
    } else {
        build_path(fpath, path);
    }

    dp = opendir(fpath);

    if (dp == NULL)
        return -errno;

    while ((de = readdir(dp)) != NULL) {
        struct stat st;

        memset(&st, 0, sizeof(st));

        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;

        char name[NAME_MAX];
        strcpy(name, de->d_name);

        char *dot = strstr(name, ".enc");

        if (dot)
            *dot = '\0';

        filler(buf, name, &st, 0);
    }

    closedir(dp);

    return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi) {
    int res;
    char fpath[PATH_MAX];

    build_path(fpath, path);

    res = open(fpath, fi->flags);

    if (res == -1)
        return -errno;

    close(res);

    return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi) {

    int fd;
    int res;
    char fpath[PATH_MAX];

    (void) fi;

    build_path(fpath, path);

    fd = open(fpath, O_RDONLY);

    if (fd == -1)
        return -errno;

    char *temp = malloc(size);

    res = pread(fd, temp, size, offset);

    if (res == -1) {
        res = -errno;
    } else {
        xor_encrypt_decrypt(temp, res);
        memcpy(buf, temp, res);
    }

    free(temp);

    close(fd);

    return res;
}

static int xmp_write(const char *path, const char *buf, size_t size,
                     off_t offset, struct fuse_file_info *fi) {

    int fd;
    int res;
    char fpath[PATH_MAX];

    (void) fi;

    build_path(fpath, path);

    fd = open(fpath, O_WRONLY);

    if (fd == -1)
        return -errno;

    char *temp = malloc(size);

    memcpy(temp, buf, size);

    xor_encrypt_decrypt(temp, size);

    res = pwrite(fd, temp, size, offset);

    if (res == -1)
        res = -errno;

    free(temp);

    close(fd);

    return res;
}

static int xmp_create(const char *path, mode_t mode,
                      struct fuse_file_info *fi) {

    char fpath[PATH_MAX];

    build_path(fpath, path);

    int fd = creat(fpath, mode);

    if (fd == -1)
        return -errno;

    close(fd);

    return 0;
}

static int xmp_truncate(const char *path, off_t size) {
    char fpath[PATH_MAX];

    build_path(fpath, path);

    int res = truncate(fpath, size);

    if (res == -1)
        return -errno;

    return 0;
}

static int xmp_unlink(const char *path) {
    char fpath[PATH_MAX];

    build_path(fpath, path);

    int res = unlink(fpath);

    if (res == -1)
        return -errno;

    return 0;
}

static int xmp_mkdir(const char *path, mode_t mode) {
    char fpath[PATH_MAX];

    snprintf(fpath, PATH_MAX, "%s%s", storage_dir, path);

    int res = mkdir(fpath, mode);

    if (res == -1)
        return -errno;

    return 0;
}

static int xmp_rmdir(const char *path) {
    char fpath[PATH_MAX];

    snprintf(fpath, PATH_MAX, "%s%s", storage_dir, path);

    int res = rmdir(fpath);

    if (res == -1)
        return -errno;

    return 0;
}

static int xmp_access(const char *path, int mask) {
    char fpath[PATH_MAX];

    build_path(fpath, path);

    int res = access(fpath, mask);

    if (res == -1)
        return -errno;

    return 0;
}

static int xmp_utimens(const char *path, const struct timespec ts[2]) {
    char fpath[PATH_MAX];

    build_path(fpath, path);

    int res = utimensat(0, fpath, ts, AT_SYMLINK_NOFOLLOW);

    if (res == -1)
        return -errno;

    return 0;
}

static struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .readdir = xmp_readdir,
    .open = xmp_open,
    .read = xmp_read,
    .write = xmp_write,
    .create = xmp_create,
    .truncate = xmp_truncate,
    .unlink = xmp_unlink,
    .mkdir = xmp_mkdir,
    .rmdir = xmp_rmdir,
    .access = xmp_access,
    .utimens = xmp_utimens,
};

int main(int argc, char *argv[]) {
    umask(0);
    return fuse_main(argc, argv, &xmp_oper, NULL);
}
