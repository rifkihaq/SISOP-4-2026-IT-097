# Praktikum Sistem Operasi - FUSE Filesystem
## Modul 4 - Kenz Rescue

### Nama : Muhammad Salman Rifki Haq
### NRP  : 5027251097

# Soal 1
---

# Deskripsi Praktikum

Pada praktikum ini dibuat sebuah filesystem virtual menggunakan **FUSE (Filesystem in Userspace)**. Filesystem ini memiliki dua fitur utama:

1. **Passthrough Filesystem**
   - Menampilkan isi folder sumber (`amba_files`) ke folder mount (`mnt`) tanpa mengubah isi file asli.

2. **Virtual File**
   - Membuat file virtual bernama `tujuan.txt`.
   - File ini tidak ada secara fisik pada folder source.
   - Isi file dibangkitkan secara otomatis dengan menggabungkan semua fragmen koordinat yang memiliki prefix `KOORD:` dari file `1.txt` hingga `7.txt`.

---

# Struktur Direktori

```bash
.
├── kenz_rescue
├── kenz_rescue.c
├── amba_files
│   ├── 1.txt
│   ├── 2.txt
│   ├── 3.txt
│   ├── 4.txt
│   ├── 5.txt
│   ├── 6.txt
│   └── 7.txt
└── mnt
```

---

# Konsep FUSE

FUSE (Filesystem in Userspace) merupakan library yang memungkinkan user membuat filesystem sendiri tanpa perlu memodifikasi kernel Linux.

Pada praktikum ini:
- `amba_files/` bertindak sebagai source directory
- `mnt/` bertindak sebagai mount point
- Semua file pada source akan ditampilkan melalui filesystem virtual

---

# Source Code

## Header dan Library

```c
#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
```

### Penjelasan

| Library | Fungsi |
|---|---|
| `fuse.h` | Library utama FUSE |
| `stdio.h` | Operasi input output |
| `string.h` | Manipulasi string |
| `errno.h` | Menampilkan kode error Linux |
| `fcntl.h` | Operasi open file |
| `dirent.h` | Operasi directory |
| `unistd.h` | Operasi sistem Linux |
| `sys/stat.h` | Informasi atribut file |

---

# Variabel Global

```c
static char source_dir[1024];
```

### Fungsi
Digunakan untuk menyimpan path absolut dari folder source (`amba_files`).

---

# Fungsi get_full_path()

```c
void get_full_path(char fpath[1024], const char *path)
{
    sprintf(fpath, "%s%s", source_dir, path);
}
```

### Fungsi
Menggabungkan path virtual dari FUSE dengan path asli source directory.

### Contoh

Jika:

```bash
path = /1.txt
```

Maka hasil:

```bash
amba_files/1.txt
```

---

# Fungsi generate_tujuan()

```c
void generate_tujuan(char *result)
```

### Fungsi
Membaca file `1.txt` hingga `7.txt`, kemudian mencari baris yang memiliki prefix:

```bash
KOORD:
```

Setelah ditemukan, isi setelah `KOORD:` akan digabungkan menjadi isi dari file virtual `tujuan.txt`.

### Alur Kerja

1. Membuka file satu per satu
2. Membaca isi file menggunakan `fgets()`
3. Mengecek apakah baris diawali `KOORD:`
4. Menggabungkan koordinat menggunakan `strcat()`

---

# Callback getattr()

```c
static int xmp_getattr(...)
```

### Fungsi
Digunakan untuk mengambil atribut file seperti:
- ukuran file
- permission
- tipe file

### Penjelasan

Jika file yang diakses adalah:

```bash
/tujuan.txt
```

Maka filesystem membuat atribut virtual secara manual:

```c
stbuf->st_mode = S_IFREG | 0444;
```

Artinya:
- `S_IFREG` → regular file
- `0444` → read only

Jika bukan file virtual, maka atribut diambil dari file asli menggunakan:

```c
lstat()
```

---

# Callback readdir()

```c
static int xmp_readdir(...)
```

### Fungsi
Digunakan saat user menjalankan:

```bash
ls mnt
```

### Alur Kerja

1. Membuka source directory menggunakan `opendir()`
2. Membaca seluruh isi directory menggunakan `readdir()`
3. Menampilkan seluruh file ke mount point
4. Menambahkan file virtual `tujuan.txt`

### Bagian Penting

```c
filler(buf, "tujuan.txt", NULL, 0, 0);
```

Digunakan untuk memunculkan file virtual pada hasil `ls`.

---

# Callback open()

```c
static int xmp_open(...)
```

### Fungsi
Digunakan ketika file dibuka menggunakan:
- `cat`
- `nano`
- `less`
- dan lainnya

### Penjelasan

Jika file yang dibuka adalah:

```bash
/tujuan.txt
```

Maka fungsi langsung mengembalikan `0` karena file virtual tidak ada secara fisik.

Jika bukan file virtual, maka file asli dibuka menggunakan:

```c
open()
```

---

# Callback read()

```c
static int xmp_read(...)
```

### Fungsi
Digunakan untuk membaca isi file.

### Penjelasan

Jika file yang dibaca adalah:

```bash
/tujuan.txt
```

Maka:
1. Isi file dibangkitkan menggunakan `generate_tujuan()`
2. Isi disalin ke buffer menggunakan `memcpy()`

Jika file biasa:
- isi dibaca menggunakan `pread()`

---

# Struktur fuse_operations

```c
static struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .readdir = xmp_readdir,
    .open = xmp_open,
    .read = xmp_read,
};
```

### Fungsi
Menghubungkan callback FUSE dengan operasi filesystem Linux.

| Callback | Fungsi |
|---|---|
| `getattr` | Mengambil atribut file |
| `readdir` | Membaca isi folder |
| `open` | Membuka file |
| `read` | Membaca isi file |

---

# Fungsi main()

```c
int main(int argc, char *argv[])
```

### Fungsi
Sebagai entry point program.

### Alur Kerja

1. Mengecek argument
2. Menyimpan source directory
3. Menggeser argument mount point
4. Menjalankan filesystem menggunakan:

```c
fuse_main()
```

---

# Cara Compile

```bash
gcc kenz_rescue.c -o kenz_rescue `pkg-config fuse3 --cflags --libs`
```

---

# Cara Menjalankan

```bash
mkdir mnt
./kenz_rescue amba_files mnt
```

---

# Pengujian

## Menampilkan Isi Mount

```bash
ls mnt
```

Output:

```bash
1.txt 2.txt 3.txt 4.txt 5.txt 6.txt 7.txt tujuan.txt
```

---

## Pengujian Passthrough

```bash
cat mnt/1.txt
```

Output identik dengan:

```bash
cat amba_files/1.txt
```

---

## Pengujian File Virtual

```bash
cat mnt/tujuan.txt
```

Output:
- gabungan seluruh koordinat dari file 1 sampai 7

---

# Cara Unmount

```bash
fusermount -u mnt
```

---

# Kesimpulan

Pada praktikum ini berhasil dibuat filesystem virtual menggunakan FUSE dengan fitur:
- passthrough filesystem
- virtual file generation
- pembacaan file dinamis
- manipulasi isi filesystem tanpa mengubah source asli

Praktikum ini membantu memahami bagaimana Linux filesystem bekerja melalui callback seperti:
- `getattr`
- `readdir`
- `open`
- `read`

serta memahami konsep virtual filesystem pada sistem operasi Linux.

# Soal 2

---

# Deskripsi Praktikum

Pada praktikum ini dibuat sebuah sistem database sederhana berbasis:

- FUSE (Filesystem in Userspace)
- TCP Socket Client-Server
- Docker Container

Sistem bekerja dengan alur:

```text
Client → Server → FUSE Mount → Encrypted Storage
```

Semua file database yang disimpan akan otomatis terenkripsi menggunakan metode XOR dengan key `0x76`.

Keterangan:

- `encrypted_storage/`
  Folder penyimpanan file asli yang terenkripsi.

- `fuse_mount/`
  Mount point filesystem virtual.

- `fuse.c`
  Program filesystem virtual menggunakan FUSE.

- `server.c`
  Program database server berbasis TCP socket.

- `client.c`
  Program client untuk mengirim command ke server.

- `Dockerfile`
  File konfigurasi Docker.

---

# Penjelasan FUSE

FUSE (Filesystem in Userspace) memungkinkan pembuatan filesystem virtual tanpa membuat kernel module.

Pada praktikum ini FUSE digunakan untuk:

- mengenkripsi file saat ditulis
- mendekripsi file saat dibaca
- menyembunyikan ekstensi `.enc`

Contoh:

```text
fuse_mount/test.txt
```

akan disimpan menjadi:

```text
encrypted_storage/test.txt.enc
```

---

# Penjelasan Program fuse.c

## XOR Encryption

```c
static const unsigned char XOR_KEY = 0x76;
```

Digunakan sebagai key enkripsi dan dekripsi.

---

## Fungsi xor_encrypt_decrypt()

```c
void xor_encrypt_decrypt(char *buf, size_t size)
```

Fungsi ini melakukan operasi XOR pada setiap byte data.

Karena XOR bersifat reversible:

```text
A XOR B XOR B = A
```

maka fungsi yang sama dapat digunakan untuk encrypt dan decrypt.

---

## Fungsi build_path()

Digunakan untuk membangun path file asli dan menambahkan ekstensi `.enc`.

Contoh:

```text
/test.txt
```

menjadi:

```text
encrypted_storage/test.txt.enc
```

---

## Fungsi xmp_getattr()

Digunakan untuk mengambil atribut file seperti ukuran file, permission, dan tipe file.

---

## Fungsi xmp_readdir()

Digunakan untuk membaca isi direktori dan menyembunyikan ekstensi `.enc` dari user.

---

## Fungsi xmp_read()

Digunakan untuk:

1. membaca file terenkripsi
2. melakukan dekripsi XOR
3. menampilkan hasil dekripsi ke user

---

## Fungsi xmp_write()

Digunakan untuk:

1. menerima data dari user
2. mengenkripsi data menggunakan XOR
3. menyimpan data ke file asli

---

## Fungsi xmp_create()

Digunakan untuk membuat file baru dengan ekstensi `.enc`.

---

## Fungsi main()

Menjalankan filesystem FUSE menggunakan `fuse_main()`.

---

# Penjelasan Program server.c

Program `server.c` berfungsi sebagai database server sederhana berbasis TCP socket.

Server berjalan pada port:

```text
9000
```

---

## Fungsi Socket

```c
socket(AF_INET, SOCK_STREAM, 0)
```

Digunakan untuk membuat TCP socket.

---

## Fungsi bind()

```c
bind(server_fd, ...)
```

Digunakan untuk menghubungkan server ke port 9000.

---

## Fungsi listen()

```c
listen(server_fd, 5)
```

Digunakan agar server dapat menerima koneksi client.

---

## Fungsi accept()

```c
accept(server_fd, ...)
```

Digunakan untuk menerima koneksi dari client.

---

## Fungsi create_database()

Digunakan untuk membuat folder database.

Contoh:

```text
CREATE DATABASE kampus
```

akan membuat:

```text
db/kampus/
```

---

## Fungsi create_table()

Digunakan untuk membuat file CSV sebagai tabel.

Contoh:

```text
CREATE TABLE kampus mahasiswa
```

akan membuat:

```text
db/kampus/mahasiswa.csv
```

---

## Fungsi list_database()

Digunakan untuk menampilkan daftar database.

---

## Fungsi list_table()

Digunakan untuk menampilkan daftar tabel dalam database.

---

# Penjelasan Program client.c

Program `client.c` berfungsi sebagai client untuk mengirim command ke server.

Client terhubung ke:

```text
127.0.0.1:9000
```

---

## Fungsi connect()

```c
connect(sock, ...)
```

Digunakan untuk terhubung ke server.

---

## Fungsi send()

```c
send(sock, buffer, strlen(buffer), 0)
```

Digunakan untuk mengirim command ke server.

---

## Fungsi recv()

```c
recv(sock, response, BUFFER_SIZE, 0)
```

Digunakan untuk menerima response dari server.

---

# Penjelasan Docker

Docker digunakan untuk menjalankan database server di dalam container.

Keuntungan Docker:

- environment konsisten
- mudah dipindahkan
- isolasi aplikasi

---

# Penjelasan Dockerfile

## Base Image

```dockerfile
FROM ubuntu:latest
```

Menggunakan Ubuntu sebagai base image.

---

## Working Directory

```dockerfile
WORKDIR /app
```

Menentukan folder kerja di dalam container.

---

## Copy File

```dockerfile
COPY . /app
```

Menyalin seluruh project ke container.

---

## Install GCC

```dockerfile
RUN apt update && apt install -y gcc make
```

Menginstall compiler.

---

## Expose Port

```dockerfile
EXPOSE 9000
```

Membuka port 9000.

---

## CMD

```dockerfile
CMD ["./server"]
```

Menjalankan program server saat container dijalankan.

---

# Cara Menjalankan Program

## Compile FUSE

```bash
gcc fuse.c -o fuse `pkg-config fuse --cflags --libs`
```

## Compile Server

```bash
gcc server.c -o server
```

## Compile Client

```bash
gcc client.c -o client
```

## Jalankan FUSE

```bash
./fuse -f fuse_mount
```

## Jalankan Server

```bash
./server
```

## Jalankan Client

```bash
./client
```

---

# Contoh Command

## Membuat Database

```text
CREATE DATABASE kampus
```

## Melihat Database

```text
LIST DATABASE
```

## Membuat Table

```text
CREATE TABLE kampus mahasiswa
```

## Melihat Table

```text
LIST TABLE kampus
```

---

# Hasil Enkripsi

File asli tersimpan pada:

```text
encrypted_storage/
```

Contoh:

```text
mahasiswa.csv.enc
```

Isi file akan berupa karakter acak karena telah dienkripsi menggunakan XOR.

---

# Kesimpulan

Praktikum ini berhasil mengimplementasikan:

1. Filesystem virtual menggunakan FUSE
2. Enkripsi otomatis menggunakan XOR
3. Database sederhana berbasis file CSV
4. Komunikasi client-server menggunakan TCP socket
5. Containerisasi menggunakan Docker

Sistem berhasil melakukan:

- pembuatan database
- pembuatan tabel
- penyimpanan data terenkripsi
- komunikasi jaringan melalui socket
