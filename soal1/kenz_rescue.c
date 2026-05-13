#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

static char source_dir[1024];

void get_full_path(char fpath[1024], const char *path)
{
    sprintf(fpath, "%s%s", source_dir, path);
}

/* =========================
   GENERATE tujuan.txt
   ========================= */
void generate_tujuan(char *result)
{
    result[0] = '\0';

    for (int i = 1; i <= 7; i++)
    {
        char filename[1024];
        sprintf(filename, "%s/%d.txt", source_dir, i);

        FILE *fp = fopen(filename, "r");

        if (!fp)
            continue;

        char line[1024];

        while (fgets(line, sizeof(line), fp))
        {
            if (strncmp(line, "KOORD:", 6) == 0)
            {
                strcat(result, line + 6);
            }
        }

        fclose(fp);
    }
}

/* =========================
   GETATTR
   ========================= */
static int xmp_getattr(const char *path, struct stat *stbuf,
                       struct fuse_file_info *fi)
{
    (void) fi;

    int res;
    char fpath[1024];

    memset(stbuf, 0, sizeof(struct stat));

    /* FILE VIRTUAL */
    if (strcmp(path, "/tujuan.txt") == 0)
    {
        char content[4096];

        generate_tujuan(content);

        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = strlen(content);

        return 0;
    }

    get_full_path(fpath, path);

    res = lstat(fpath, stbuf);

    if (res == -1)
        return -errno;

    return 0;
}

/* =========================
   READDIR
   ========================= */
static int xmp_readdir(const char *path,
                       void *buf,
                       fuse_fill_dir_t filler,
                       off_t offset,
                       struct fuse_file_info *fi,
                       enum fuse_readdir_flags flags)
{
    (void) offset;
    (void) fi;
    (void) flags;

    DIR *dp;
    struct dirent *de;

    char fpath[1024];

    get_full_path(fpath, path);

    dp = opendir(fpath);

    if (dp == NULL)
        return -errno;

    while ((de = readdir(dp)) != NULL)
    {
        struct stat st;

        memset(&st, 0, sizeof(st));

        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;

        filler(buf, de->d_name, &st, 0, 0);
    }

    /* TAMBAHKAN FILE VIRTUAL */
    filler(buf, "tujuan.txt", NULL, 0, 0);

    closedir(dp);

    return 0;
}

/* =========================
   OPEN
   ========================= */
static int xmp_open(const char *path, struct fuse_file_info *fi)
{
    /* IZINKAN tujuan.txt DIBUKA */
    if (strcmp(path, "/tujuan.txt") == 0)
        return 0;

    int res;
    char fpath[1024];

    get_full_path(fpath, path);

    res = open(fpath, fi->flags);

    if (res == -1)
        return -errno;

    close(res);

    return 0;
}

/* =========================
   READ
   ========================= */
static int xmp_read(const char *path,
                    char *buf,
                    size_t size,
                    off_t offset,
                    struct fuse_file_info *fi)
{
    (void) fi;

    /* BACA FILE VIRTUAL */
    if (strcmp(path, "/tujuan.txt") == 0)
    {
        char content[4096];

        generate_tujuan(content);

        size_t len = strlen(content);

        if (offset < len)
        {
            if (offset + size > len)
                size = len - offset;

            memcpy(buf, content + offset, size);
        }
        else
        {
            size = 0;
        }

        return size;
    }

    int fd;
    int res;

    char fpath[1024];

    get_full_path(fpath, path);

    fd = open(fpath, O_RDONLY);

    if (fd == -1)
        return -errno;

    res = pread(fd, buf, size, offset);

    if (res == -1)
        res = -errno;

    close(fd);

    return res;
}

/* =========================
   OPERATIONS
   ========================= */
static struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .readdir = xmp_readdir,
    .open = xmp_open,
    .read = xmp_read,
};

/* =========================
   MAIN
   ========================= */
int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr,
                "Usage: %s <source_dir> <mount_point>\n",
                argv[0]);
        return 1;
    }

    realpath(argv[1], source_dir);

    /* GESER ARGUMENT */
    for (int i = 1; i < argc - 1; i++)
    {
        argv[i] = argv[i + 1];
    }

    argc--;

    return fuse_main(argc, argv, &xmp_oper, NULL);
}
