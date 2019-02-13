#include "lib/stubs/externals/externals_stub.h"
#include "lib/stubs/hardware_stub.h"

#include <dlfcn.h>

#include <dirent.h>
#include <endian.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <klee/klee.h>

void orig_printf(const char * format, ...);

// Globals
static const int POS_UNOPENED = -1;
static const int POS_EOF = -2;

enum stub_file_kind {
	KIND_FILE,
	KIND_DIRECTORY,
	KIND_LINK
};

struct stub_mmap {
	// >1 -> in use
	int refcount;

	// False for read-only mmaps (we disallow access altogether)
	bool accessible;

	// 'mem' is 'actual_mem' rounded up to the page size
	// See 'mmap' for an explanation.
	void* mem;
	size_t mem_len;
	void* actual_mem;
	size_t actual_mem_len;
};

struct stub_file {
	// Folders MUST NOT have a trailing slash
	// Unix-like multi-slash simplification (e.g. /a//b == /a/b) is NOT supported
	char* path;

	// Either: (file or symlink)
	char* content;
        size_t content_len;
	// Or: (folder)
	int* children;
	int children_len;

	// In the file and folder cases, this keeps track of progress
	off_t pos;

	// Set at creation time
	enum stub_file_kind kind;

	// Support flock
	bool locked;

	// Support mmap (2 max)
	struct stub_mmap mmaps[2];
	size_t mmaps_len;
};
// Files indexed by FD
// TODO what if the code under verification branches on an FD with a hardcoded comparison?
static struct stub_file FILES[STUB_FILES_COUNT];

// Special case: the page map
static char* FILE_CONTENT_PAGEMAP = (char*) -100;

// Special case: the hugepage info file, which is truncated
static char* FILE_CONTENT_HPINFO = (char*) -200;

// Special case: Hugepage files
static char* FILE_CONTENT_HUGEPAGE = (char*) -300;


int
access(const char* pathname, int mode)
{
	if (mode == F_OK) {
		for (int n = 0; n < sizeof(FILES)/sizeof(FILES[0]); n++) {
			if (FILES[n].path != NULL && !strcmp(pathname, FILES[n].path)) {
				return 0;
			}
		}

		// Other CPUs
		const char* cpu_prefix = "/sys/devices/system/cpu/cpu";
		if (!strncmp(pathname, cpu_prefix, strlen(cpu_prefix))) {
			return -1; // TODO
		}
	}

	do {orig_printf("aborting on %s:%d\n", __LINE__, __FILE__);klee_abort();} while(0);
}

int
stat(const char* path, struct stat* buf)
{
	// Nope, we don't have modules
	if (!strcmp(path, "/sys/module")) {
		return -1;
	}

	do {orig_printf("aborting on %s:%d\n", __LINE__, __FILE__);klee_abort();} while(0);
}

int is_ktest_file(const char* file) {
    return strlen("ktest") < strlen(file) &&
        0 == strncmp(file + (strlen(file) - strlen("ktest")),
                     "ktest",
                     strlen("ktest"));
}

#ifdef VIGOR_EXECUTABLE
typedef int (*vfprintf_orig_type)(FILE *stream, const char *format, va_list ap);

void orig_printf(const char * format, ...)
{
   va_list args;

   vfprintf_orig_type orig_vfprintf;
   FILE *orig_stdout;
   orig_vfprintf = (vfprintf_orig_type)dlsym(RTLD_NEXT,"vfprintf");
   orig_stdout = *(FILE **)dlsym(RTLD_NEXT,"stdout");

   typedef int (*fflush_unlocked_orig_type)(FILE *stream);
   fflush_unlocked_orig_type orig_fflush = (fflush_unlocked_orig_type)dlsym(RTLD_NEXT,"fflush_unlocked");

   va_start (args, format);
   orig_vfprintf (orig_stdout, format, args);
   orig_fflush(orig_stdout);
   va_end (args);
}


typedef int (*orig_open_type)(const char* file, int oflag, ...);

int orig_open(const char* file, int oflag, mode_t mode) {
   orig_open_type orig_open_fptr;
   orig_open_fptr = (orig_open_type)dlsym(RTLD_NEXT,"open");

   return orig_open_fptr(file, oflag, mode);
}

typedef int (*orig_close_type)(int fd);

int orig_close(int fd) {
   orig_close_type orig_close_fptr;
   orig_close_fptr = (orig_close_type)dlsym(RTLD_NEXT,"close");

   return orig_close_fptr(fd);
}

typedef ssize_t (*orig_read_type)(int fd, void *buf, size_t count);

ssize_t orig_read(int fd, void *buf, size_t count) {
   orig_read_type orig_read_fptr;
   orig_read_fptr = (orig_read_type)dlsym(RTLD_NEXT,"read");

   return orig_read_fptr(fd, buf, count);
}

#else//VIGOR_EXECUTABLE

void orig_printf(const char * format, ...) {
   va_list args;
   va_start (args, format);
   vfprintf (stdout, format, args);
   fflush(stdout);
   va_end (args);
}

int orig_open(const char* file, int oflag, mode_t mode) {
    klee_assert(false);//Must never be called
}

int orig_close(int fd) {
    klee_assert(false);//Must never be called
}

ssize_t orig_read(int fd, void *buf, size_t count) {
    klee_assert(false);//Must never be called
}

#endif//VIGOR_EXECUTABLE

#define MAX_FILE_SIZE (128*1024*1024)

int
open(const char* file, int oflag, ...)
{
        //orig_printf("opening %s\n", file);

        if (is_ktest_file(file)) {
                orig_printf("special handling ktest file\n");
                int orig_fd = orig_open(file, oflag, O_RDONLY);
                void *buf = malloc(MAX_FILE_SIZE);
                int read_len = orig_read(orig_fd, buf, MAX_FILE_SIZE);
                int fd = stub_add_bin_file(file, buf, read_len);
                orig_close(orig_fd);
                FILES[fd].pos = 0;
                return fd;
        }

	if (!strcmp(file, "/proc/cpuinfo") && oflag == O_RDONLY) {
		return -1; // TODO
	}

	// NUMA map, unsupported for now
	if (!strcmp(file, "/proc/self/numa_maps") && oflag == O_RDONLY) {
		return -1; // TODO
	}

	// Generic
	enum stub_file_kind desired_kind = ((oflag & O_DIRECTORY) == O_DIRECTORY) ? KIND_DIRECTORY : KIND_FILE;
	for (int n = 0; n < sizeof(FILES)/sizeof(FILES[0]); n++) {
		if (FILES[n].path != NULL && !strcmp(file, FILES[n].path) && FILES[n].kind == desired_kind) {
			klee_assert(FILES[n].pos == POS_UNOPENED);

			FILES[n].pos = 0;

			return n;
		}
	}

	// Other CPUs
	const char* cpu_prefix = "/sys/devices/system/cpu/cpu";
	if (!strncmp(file, cpu_prefix, strlen(cpu_prefix)) && oflag == O_RDONLY) {
		return -1; // TODO
	}

	// Not supported!
	do {orig_printf("aborting on %s:%d\n", __LINE__, __FILE__);klee_abort();} while(0);
}


int
open64(const char* file, int oflag, ...)
{
        //orig_printf("opening %s\n", file);

        if (is_ktest_file(file)) {
                orig_printf("special handling ktest file\n");
                int orig_fd = orig_open(file, oflag, O_RDONLY);
                void *buf = malloc(MAX_FILE_SIZE);
                int read_len = orig_read(orig_fd, buf, MAX_FILE_SIZE);
                int fd = stub_add_bin_file(file, buf, read_len);
                orig_close(orig_fd);
                FILES[fd].pos = 0;
                return fd;
        }

        if (!strcmp(file, "/proc/cpuinfo") && oflag == O_RDONLY) {
                return -1; // TODO
        }

        // NUMA map, unsupported for now
        if (!strcmp(file, "/proc/self/numa_maps") && oflag == O_RDONLY) {
                return -1; // TODO
        }

        // Generic
        enum stub_file_kind desired_kind = ((oflag & O_DIRECTORY) == O_DIRECTORY) ? KIND_DIRECTORY : KIND_FILE;
        for (int n = 0; n < sizeof(FILES)/sizeof(FILES[0]); n++) {
                if (FILES[n].path != NULL && !strcmp(file, FILES[n].path) && FILES[n].kind == desired_kind) {
                        klee_assert(FILES[n].pos == POS_UNOPENED);

                        FILES[n].pos = 0;

                        return n;
                }
        }

        // Other CPUs
        const char* cpu_prefix = "/sys/devices/system/cpu/cpu";
        if (!strncmp(file, cpu_prefix, strlen(cpu_prefix)) && oflag == O_RDONLY) {
                return -1; // TODO
        }

        // Not supported!
        do {orig_printf("aborting on %s:%d\n", __LINE__, __FILE__);klee_abort();} while(0);
}

int
fcntl(int fd, int cmd, ...)
{
	klee_assert(cmd == F_SETFD);

	va_list args;
	va_start(args, cmd);

	int arg = va_arg(args, int);
	klee_assert(arg == FD_CLOEXEC);

	klee_assert(FILES[fd].children != NULL);

	return 0;
}

int
flock(int fd, int operation)
{
	klee_assert(FILES[fd].pos != POS_UNOPENED);

	// We assign similar semantics to EX and SH since we're single-threaded
	if ((operation & LOCK_EX) == LOCK_EX || (operation & LOCK_SH) == LOCK_SH) {
		// POSIX locks are re-entrant
		FILES[fd].locked = true;
		return 0;
	}

	if ((operation & LOCK_UN) == LOCK_UN) {
		klee_assert(FILES[fd].locked);
		FILES[fd].locked = false;
		return 0;
	}

	do {orig_printf("aborting on %s:%d\n", __LINE__, __FILE__);klee_abort();} while(0);
}

int
fstat(int fd, struct stat* buf)
{
	klee_assert(FILES[fd].pos != POS_UNOPENED);
	klee_assert(FILES[fd].kind == KIND_DIRECTORY);

	memset(buf, 0, sizeof(struct stat));

	return 0;
}

int
ftruncate(int fd, off_t length)
{
	klee_assert(FILES[fd].pos != POS_UNOPENED);
	klee_assert(FILES[fd].kind == KIND_FILE);

	if (FILES[fd].content == FILE_CONTENT_HPINFO) {
		FILES[fd].content = (char*) malloc(length);
		memset(FILES[fd].content, 0, length);
                FILES[fd].content_len = length;

		// On success, zero is returned. On error, -1 is returned, and errno is set appropriately.
		// -- https://linux.die.net/man/2/ftruncate
		return 0;
	}

	do {orig_printf("aborting on %s:%d\n", __LINE__, __FILE__);klee_abort();} while(0);
}
int
ftruncate64(int fd, off_t length)
{
        klee_assert(FILES[fd].pos != POS_UNOPENED);
        klee_assert(FILES[fd].kind == KIND_FILE);

        if (FILES[fd].content == FILE_CONTENT_HPINFO) {
                FILES[fd].content = (char*) malloc(length);
                memset(FILES[fd].content, 0, length);
                FILES[fd].content_len = length;

                // On success, zero is returned. On error, -1 is returned, and errno is set appropriately.
                // -- https://linux.die.net/man/2/ftruncate
                return 0;
        }

        do {orig_printf("aborting on %s:%d\n", __LINE__, __FILE__);klee_abort();} while(0);
}

off_t
lseek(int fd, off_t offset, int whence)
{
	klee_assert(FILES[fd].pos != POS_UNOPENED);
	klee_assert(FILES[fd].kind == KIND_FILE);

	if (FILES[fd].content == FILE_CONTENT_PAGEMAP) {
		klee_assert(whence == SEEK_SET);

		FILES[fd].pos = offset;

		// Upon successful completion, lseek() returns the resulting offset
		// location as measured in bytes from the beginning of the file. On
		// error, the value (off_t) -1 is returned and errno is set to indicate the error.
		// -- http://man7.org/linux/man-pages/man2/lseek.2.html
		return offset;
	} else if (whence == SEEK_CUR) {
                FILES[fd].pos += offset;
                return offset;
        }

	do {orig_printf("aborting on %s:%d\n", __LINE__, __FILE__);klee_abort();} while(0);
}
off_t
lseek64(int fd, off_t offset, int whence)
{
        klee_assert(FILES[fd].pos != POS_UNOPENED);
        klee_assert(FILES[fd].kind == KIND_FILE);

        if (FILES[fd].content == FILE_CONTENT_PAGEMAP) {
                klee_assert(whence == SEEK_SET);

                FILES[fd].pos = offset;

                // Upon successful completion, lseek() returns the resulting offset
                // location as measured in bytes from the beginning of the file. On
                // error, the value (off_t) -1 is returned and errno is set to indicate the error.
                // -- http://man7.org/linux/man-pages/man2/lseek.2.html
                return offset;
	} else if (whence == SEEK_CUR) {
                FILES[fd].pos += offset;
                return offset;
        }

        do {orig_printf("aborting on %s:%d\n", __LINE__, __FILE__);klee_abort();} while(0);
}

ssize_t
read(int fd, void *buf, size_t count)
{
	klee_assert(FILES[fd].pos != POS_UNOPENED);
	klee_assert(FILES[fd].kind == KIND_FILE);

	if (FILES[fd].content == FILE_CONTENT_PAGEMAP) {
		klee_assert(count == 8);

		// Read fake pagemap data
		// See mmap() for an explanation of the computation
		// The file's position is the virtual PFN times the size of a PFN (64 bits),
		// and we want the PFN to be equal to the VPFN since we want VAs and PAs to match
		off_t vpfn = FILES[fd].pos / sizeof(uint64_t);
		klee_assert(vpfn < (((uint64_t) 1) << 55)); // PFNs are stored on 55 bits

		// TODO I think DPDK forgets to check whether the page is marked as swapped,
		//      which changes the meaning of bits 0-54...

		memset(buf, 0, count);
#if __BYTE_ORDER == __BIG_ENDIAN
		do {orig_printf("aborting on %s:%d\n", __LINE__, __FILE__);klee_abort();} while(0); // TODO too lazy to do it right now
#else
		// Bits 0-54 are the PFN, bit 63 is "page present", rest can be 0
		// See https://www.kernel.org/doc/Documentation/vm/pagemap.txt
		*((uint64_t*) buf) = vpfn | (((uint64_t) 1) << 63);
#endif

		return count;
	}

        ssize_t total_read = 0;
        while (0 < count) {
            if (FILES[fd].pos < FILES[fd].content_len) {
                    *((char*) buf + total_read) = FILES[fd].content[FILES[fd].pos];
                    ++FILES[fd].pos;
                    ++total_read;
                    --count;
            } else {
                FILES[fd].pos = POS_EOF;
                return total_read;
            }
        }
        return total_read;
}

int
close(int fd)
{
	klee_assert(FILES[fd].pos != POS_UNOPENED);

	FILES[fd].pos = POS_UNOPENED;
	FILES[fd].locked = false;

	// We do not remove mmapings:
	// "The mmap() function adds an extra reference to the file associated with the file descriptor fildes
	//  which is not removed by a subsequent close() on that file descriptor. This reference is removed when there are no more mappings to the file."
	// -- http://pubs.opengroup.org/onlinepubs/7908799/xsh/mmap.html

	return 0;
}

ssize_t
readlink(const char* pathname, char* buf, size_t bufsiz)
{
	for (int n = 0; n < sizeof(FILES)/sizeof(FILES[0]); n++) {
		if (FILES[n].path != NULL && !strcmp(pathname, FILES[n].path)) {
			klee_assert(FILES[n].kind == KIND_LINK);
			klee_assert(bufsiz > FILES[n].content_len);

			memcpy(buf, FILES[n].content, FILES[n].content_len);
			return FILES[n].content_len;
		}
	}

	do {orig_printf("aborting on %s:%d\n", __LINE__, __FILE__);klee_abort();} while(0);
}

// NOTE: This is a klee-uclibc internal
//       which is excluded from build because other stuff next to it causes problems with klee
//       according to a comment in libc/Makefile.in
//       The gist of it is that it reads N directory entries,
//       with a limit on the number of bytes, and returns the number of bytes actually read.
// NOTE: It seems that klee-uclibc is wrong
//       There is a comment 'Am I right?' in the source of readdir.c
//       However, r_reclen is not the record length but the length of the name;
//       thus, if the name is smaller than the size of struct dirent, things go wrong
//       As a workaround, we always set d_reclen to sizeof(struct dirent)...
ssize_t
__getdents (int fd, char* buf, size_t nbytes)
{
	size_t len = sizeof(struct dirent);
	klee_assert(nbytes >= len);

	struct dirent* de = (struct dirent*) buf;
	memset(de, 0, len);
	de->d_ino = 1; // just needs to be non-zero

	klee_assert(FILES[fd].kind == KIND_DIRECTORY);
	klee_assert(FILES[fd].pos >= 0);
	klee_assert(FILES[fd].pos <= FILES[fd].children_len);

	if (FILES[fd].pos == FILES[fd].children_len) {
		FILES[fd].pos = POS_EOF;
		return 0;
	}

	int child_fd = FILES[fd].children[FILES[fd].pos];
	char* filename = strrchr(FILES[child_fd].path, '/') + 1;
	strcpy(de->d_name, filename);
	de->d_reclen = len; // should bestrlen(de->d_name)
	de->d_type = FILES[child_fd].content == NULL ? DT_DIR : 0;

	FILES[fd].pos++;

	return len;
}

void*
mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset)
{
	// http://man7.org/linux/man-pages/man2/mmap.2.html

	// Offsets not supported
	// NOTE: if they were, they'd need to be a multiple of PAGE_SIZE
	klee_assert(offset == 0);

	klee_assert(FILES[fd].kind == KIND_FILE);

	// First off, are we trying to mmap device memory?
	for (int n = 0; n < sizeof(DEVICES)/sizeof(DEVICES[0]); n++) {
		if ((void*) FILES[fd].content == DEVICES[n].mem) {
			klee_assert(length == DEVICES[n].mem_len);

			return DEVICES[n].mem;
		}
	}

	// We don't really care about flags, since we're single-threaded

	// addr must be NULL, unless we're mmapping hugepages
	// TODO check the address somehow when it's a hugepage
	if (FILES[fd].content == FILE_CONTENT_HUGEPAGE) {
		// if it's a hugepage, length is well-defined
		klee_assert(length % (2048 * 1024) == 0);
	} else {
		klee_assert(addr == NULL);
		klee_assert(length >= FILES[fd].content_len);
	}

	// Keep the same address if we're mmapping the same length again
	// DPDK depends on this for hugepages mapping...
	for (int m = 0; m < FILES[fd].mmaps_len; m++) {
		if (FILES[fd].mmaps[m].mem_len == length) {
			if (FILES[fd].mmaps[m].refcount == 0 && FILES[fd].mmaps[m].accessible) {
				// freed mmap, need to re-allow access
				klee_allow_access(FILES[fd].mmaps[m].actual_mem, FILES[fd].mmaps[m].actual_mem_len);
			}

			FILES[fd].mmaps[m].refcount++;
			return FILES[fd].mmaps[m].mem;
		}
	}

	// don't mmap too many times
	klee_assert(FILES[fd].mmaps_len < sizeof(FILES[0].mmaps)/sizeof(FILES[0].mmaps[0]));

	// We need to align the returned value to the page size.
	// This is because we want "physical" addresses a.k.a. PAs (that we report) to be the same as virtual addresses a.k.a. VAs;
	// since PA = (PFN * PS) + (VA % PS)
	//       where PFN is the Page Frame Number, PS is the Page Size
	//   PA = VA implies PFN = (VA - (VA % PS))/PS, and since PFN must be an integer, (VA % PS) must be 0,
	//   which is only the case if the address is aligned.
	// Thus, we allocate an additional page so that we can always align the return value to a page,
	// since at most the offset we'll have to add to the "real" VA is the page size itself.
	size_t actual_length = length + PAGE_SIZE;
	void* actual_mem = malloc(actual_length);
        //klee_possibly_havoc(actual_mem, actual_length, "actual_mem");
	memset(actual_mem, 0, actual_length);

	// note that this will result in an offset of PAGE_SIZE even if it could be 0 - we don't care
	size_t real_offset = PAGE_SIZE - (((intptr_t) actual_mem) % PAGE_SIZE);
	void* mem = (actual_mem + real_offset);

	int m = FILES[fd].mmaps_len;
	FILES[fd].mmaps[m].refcount = 1;
	FILES[fd].mmaps[m].mem = mem;
	FILES[fd].mmaps[m].mem_len = length;
	FILES[fd].mmaps[m].actual_mem = actual_mem;
	FILES[fd].mmaps[m].actual_mem_len = actual_length;
	FILES[fd].mmaps_len++;

	if ((prot & PROT_WRITE) != PROT_WRITE) {
		// Read-only memory, we enforce even stronger semantics by disallowing reads with forbid_access
		klee_forbid_access(actual_mem, actual_length, "mmapped read-only memory");
		FILES[fd].mmaps[m].accessible = false;
	} else {
		FILES[fd].mmaps[m].accessible = true;
	}

	return mem;
}

void*
mmap64(void* addr, size_t length, int prot, int flags, int fd, off_t offset)
{
        // http://man7.org/linux/man-pages/man2/mmap.2.html

        // Offsets not supported
        // NOTE: if they were, they'd need to be a multiple of PAGE_SIZE
        klee_assert(offset == 0);

        klee_assert(FILES[fd].kind == KIND_FILE);

        // First off, are we trying to mmap device memory?
        for (int n = 0; n < sizeof(DEVICES)/sizeof(DEVICES[0]); n++) {
                if ((void*) FILES[fd].content == DEVICES[n].mem) {
                        klee_assert(length == DEVICES[n].mem_len);

                        return DEVICES[n].mem;
                }
        }

        // We don't really care about flags, since we're single-threaded

        // addr must be NULL, unless we're mmapping hugepages
        // TODO check the address somehow when it's a hugepage
        if (FILES[fd].content == FILE_CONTENT_HUGEPAGE) {
                // if it's a hugepage, length is well-defined
                klee_assert(length % (2048 * 1024) == 0);
        } else {
                klee_assert(addr == NULL);
                klee_assert(length >= FILES[fd].content_len);
        }

        // Keep the same address if we're mmapping the same length again
        // DPDK depends on this for hugepages mapping...
        for (int m = 0; m < FILES[fd].mmaps_len; m++) {
                if (FILES[fd].mmaps[m].mem_len == length) {
                        if (FILES[fd].mmaps[m].refcount == 0 && FILES[fd].mmaps[m].accessible) {
                                // freed mmap, need to re-allow access
                                klee_allow_access(FILES[fd].mmaps[m].actual_mem, FILES[fd].mmaps[m].actual_mem_len);
                        }

                        FILES[fd].mmaps[m].refcount++;
                        return FILES[fd].mmaps[m].mem;
                }
        }

        // don't mmap too many times
        klee_assert(FILES[fd].mmaps_len < sizeof(FILES[0].mmaps)/sizeof(FILES[0].mmaps[0]));

        // We need to align the returned value to the page size.
        // This is because we want "physical" addresses a.k.a. PAs (that we report) to be the same as virtual addresses a.k.a. VAs;
        // since PA = (PFN * PS) + (VA % PS)
        //       where PFN is the Page Frame Number, PS is the Page Size
        //   PA = VA implies PFN = (VA - (VA % PS))/PS, and since PFN must be an integer, (VA % PS) must be 0,
        //   which is only the case if the address is aligned.
        // Thus, we allocate an additional page so that we can always align the return value to a page,
        // since at most the offset we'll have to add to the "real" VA is the page size itself.
        size_t actual_length = length + PAGE_SIZE;
        void* actual_mem = malloc(actual_length);
        memset(actual_mem, 0, actual_length);

        // note that this will result in an offset of PAGE_SIZE even if it could be 0 - we don't care
        size_t real_offset = PAGE_SIZE - (((intptr_t) actual_mem) % PAGE_SIZE);
        void* mem = (actual_mem + real_offset);

        int m = FILES[fd].mmaps_len;
        FILES[fd].mmaps[m].refcount = 1;
        FILES[fd].mmaps[m].mem = mem;
        FILES[fd].mmaps[m].mem_len = length;
        FILES[fd].mmaps[m].actual_mem = actual_mem;
        FILES[fd].mmaps[m].actual_mem_len = actual_length;
        FILES[fd].mmaps_len++;

        if ((prot & PROT_WRITE) != PROT_WRITE) {
                // Read-only memory, we enforce even stronger semantics by disallowing reads with forbid_access
                klee_forbid_access(actual_mem, actual_length, "mmapped read-only memory");
                FILES[fd].mmaps[m].accessible = false;
        } else {
                FILES[fd].mmaps[m].accessible = true;
        }

        return mem;
}

int
munmap(void* addr, size_t length)
{
	// First off, are we trying to unmap device memory?
	for (int n = 0; n < sizeof(DEVICES)/sizeof(DEVICES[0]); n++) {
		if (addr == DEVICES[n].mem) {
			klee_assert(length == DEVICES[n].mem_len);

			// Do nothing - the memory still exists!
			return 0;
		}
	}

	// Upon successful completion, munmap() shall return 0; otherwise, it shall return -1 and set errno to indicate the error.
	// -- https://linux.die.net/man/3/munmap

	for (int n = 0; n < sizeof(FILES)/sizeof(FILES[0]); n++) {
		for (int m = 0; m < FILES[n].mmaps_len; m++) {
			if (FILES[n].mmaps[m].mem == addr) {
				klee_assert(FILES[n].mmaps[m].mem_len == length);

				// We never free the mappings or decrease mmaps_len, since we keep old mappings alive
				// But we do ensure freed mmaps are not accessed
				FILES[n].mmaps[m].refcount--;
				if (FILES[n].mmaps[m].refcount == 0 && FILES[n].mmaps[m].accessible) {
					klee_forbid_access(FILES[n].mmaps[m].actual_mem, FILES[n].mmaps[m].actual_mem_len, "freed mmap");
				}

				return 0;
			}
		}
	}

	do {orig_printf("aborting on %s:%d\n", __LINE__, __FILE__);klee_abort();} while(0);
}

int
munmap64(void* addr, size_t length)
{
        // First off, are we trying to unmap device memory?
        for (int n = 0; n < sizeof(DEVICES)/sizeof(DEVICES[0]); n++) {
                if (addr == DEVICES[n].mem) {
                        klee_assert(length == DEVICES[n].mem_len);

                        // Do nothing - the memory still exists!
                        return 0;
                }
        }

        // Upon successful completion, munmap() shall return 0; otherwise, it shall return -1 and set errno to indicate the error.
        // -- https://linux.die.net/man/3/munmap

        for (int n = 0; n < sizeof(FILES)/sizeof(FILES[0]); n++) {
                for (int m = 0; m < FILES[n].mmaps_len; m++) {
                        if (FILES[n].mmaps[m].mem == addr) {
                                klee_assert(FILES[n].mmaps[m].mem_len == length);

                                // We never free the mappings or decrease mmaps_len, since we keep old mappings alive
                                // But we do ensure freed mmaps are not accessed
                                FILES[n].mmaps[m].refcount--;
                                if (FILES[n].mmaps[m].refcount == 0 && FILES[n].mmaps[m].accessible) {
                                        klee_forbid_access(FILES[n].mmaps[m].actual_mem, FILES[n].mmaps[m].actual_mem_len, "freed mmap");
                                }

                                return 0;
                        }
                }
        }

        do {orig_printf("aborting on %s:%d\n", __LINE__, __FILE__);klee_abort();} while(0);
}

__attribute__((constructor(150))) // High prio, must execute after other stuff since it relies on hardware stubs
static void
stub_stdio_files_init(void)
{
	// Helper methods declarations
	char* stub_pci_file(const char* device_name, const char* file_name);
	char* stub_pci_folder(const char* device_name);
	char* stub_pci_addr(size_t addr);
	int stub_add_file(char* path, char* content);
	int stub_add_bin_file(char* path, char* content, size_t content_len);
	int stub_add_link(char* path, char* content);
	int stub_add_folder_array(char* path, int children_len, int* children);
	int stub_add_folder(char* path, int children_len, ...);


	// Files initialization
	int f = 0;
	memset(FILES, 0, sizeof(FILES));

	// PCI-related files
	int* dev_folders = (int*) malloc(STUB_HARDWARE_DEVICES_COUNT * sizeof(int));
	for (int n = 0; n < STUB_HARDWARE_DEVICES_COUNT; n++) {
		char* dev = DEVICES[n].name;

		// Basic files
		int vendor_fd = stub_add_file(stub_pci_file(dev, "vendor"), "32902\n"); // Intel (0x8086)
		int device_fd = stub_add_file(stub_pci_file(dev, "device"), "4347\n"); // Intel 82599ES (0x10fb)
		int subvendor_fd = stub_add_file(stub_pci_file(dev, "subsystem_vendor"), "0\n"); // any
		int subdevice_fd = stub_add_file(stub_pci_file(dev, "subsystem_device"), "0\n"); // any
		int class_fd = stub_add_file(stub_pci_file(dev, "class"), "131072\n"); // Intel 82599ES (0x020000)
		int maxvfs_fd = stub_add_file(stub_pci_file(dev, "max_vfs"), "0\n"); // no virtual functions
		int numanode_fd = stub_add_file(stub_pci_file(dev, "numa_node"), "0\n"); // NUMA node 0

		// Driver symlink
		int driver_fd = stub_add_link(stub_pci_file(dev, "driver"), "/drivers/igb_uio");

		// 'uio' folder, itself containing an empty folder 'uioN' (where N is the device number)
		char uio_name[1024];
		snprintf(uio_name, sizeof(uio_name), "uio/uio%d", n);
		int uio_entry_fd = stub_add_folder(stub_pci_file(dev, strdup(uio_name)), 0);
		int uio_fd = stub_add_folder(stub_pci_file(dev, "uio"), 1, uio_entry_fd);

		// Resources file
		// Multiple lines; each line has the format <start addr> <end addr> <flags>
		// all of which are 64-bit numbers in hexadecimal notation (starting with 0x)
		// Flag 0x200 is "memory", i.e. the addresses designate a DMA location
		// DPDK interprets 6 lines max (PCI_MAX_RESOURCE)

		// One single resource, rest are empty
		char* resource_format =
			"%s %s 0x0000000000000200\n"
			"0x0000000000000000 0x0000000000000000 0x0000000000000000\n"
			"0x0000000000000000 0x0000000000000000 0x0000000000000000\n"
			"0x0000000000000000 0x0000000000000000 0x0000000000000000\n"
			"0x0000000000000000 0x0000000000000000 0x0000000000000000\n"
			"0x0000000000000000 0x0000000000000000 0x0000000000000000\n";
		char resource_content[1024];
		snprintf(resource_content, sizeof(resource_content), resource_format,
				stub_pci_addr((size_t) DEVICES[n].mem), stub_pci_addr((size_t) DEVICES[n].mem + DEVICES[n].mem_len - 1));

		int resource_fd = stub_add_file(stub_pci_file(dev, "resource"), strdup(resource_content));

		// Since we say we have one resource, we need to create it...
		int resource0_fd = stub_add_bin_file(stub_pci_file(dev, "resource0"), (char*) DEVICES[n].mem, 0);

		dev_folders[n] = stub_add_folder(stub_pci_folder(dev), 11,
					vendor_fd, device_fd, subvendor_fd, subdevice_fd,
					class_fd, maxvfs_fd, numanode_fd, driver_fd,
					uio_fd, resource_fd, resource0_fd);

		// UIO stuff
		char sys_uio_path[1024];
		snprintf(sys_uio_path, sizeof(sys_uio_path), "/sys/class/uio/uio%d/device/config", n);
		stub_add_file(strdup(sys_uio_path), "");

		// Interrupts
		char interrupts_file_path[1024];
		snprintf(interrupts_file_path, sizeof(interrupts_file_path), "/dev/uio%d", n);
		DEVICES[n].interrupts_fd = stub_add_file(strdup(interrupts_file_path), "");
	}

	stub_add_folder_array("/sys/bus/pci/devices", STUB_HARDWARE_DEVICES_COUNT, dev_folders);

	// Hugepages properties
	char huge_free_value[1024];
	snprintf(huge_free_value, sizeof(huge_free_value), "%d\n", STUB_HUGEPAGES_COUNT);
	int huge_res_fd = stub_add_file("/sys/kernel/mm/hugepages/hugepages-2048kB/resv_hugepages", "0\n"); // number of reserved hugepages
	int huge_free_fd = stub_add_file("/sys/kernel/mm/hugepages/hugepages-2048kB/free_hugepages", strdup(huge_free_value)); // number of free hugepages
	int huge_2048_fd = stub_add_folder("/sys/kernel/mm/hugepages/hugepages-2048kB", 2, huge_res_fd, huge_free_fd);
	stub_add_folder("/sys/kernel/mm/hugepages", 1, huge_2048_fd);

	// /sys stuff
	// TODO: We pretend CPU 0 / NUMA node 0 exist, but what about others?
	stub_add_file("/sys/devices/system/cpu/cpu0/topology/core_id", "0\n"); // CPU 0 is core ID 0
	stub_add_folder("/sys/devices/system/node/node0/cpu0", 0);

	// /proc stuff
	stub_add_file("/proc/mounts", "hugetlbfs /dev/hugepages hugetlbfs rw,relatime 0 0\n"); // only hugepages, what DPDK cares about
	stub_add_file("/proc/meminfo", "Hugepagesize:       2048 kB\n"); // only hugepages, what DPDK cares about
	stub_add_file("/proc/self/pagemap", FILE_CONTENT_PAGEMAP);

	// Hugepages folder (empty)
	int dot_fd = stub_add_file("./.", ""); // need a / in the name, see remark in stub_file
	stub_add_folder("/dev/hugepages", 1, dot_fd); // DPDK relies on there being at least 1 file in there

	// HACK this folder is opened as a file for locking and as a folder for enumerating...
	stub_add_file("/dev/hugepages", "");

	// HACK those files exist but are not bound to their folder (defined above)...
	stub_add_file("/dev/hugepages/rte", FILE_CONTENT_HUGEPAGE);
	for (int n = 0; n < STUB_HUGEPAGES_COUNT; n++) {
		char hugepage_file_name[1024];
		snprintf(hugepage_file_name, sizeof(hugepage_file_name), "/dev/hugepages/rtemap_%d", n);
		stub_add_file(strdup(hugepage_file_name), FILE_CONTENT_HUGEPAGE);
	}

	// /var stuff
	stub_add_file("/var/run/.rte_hugepage_info", FILE_CONTENT_HPINFO);

	// Other devices
	stub_add_file("/dev/zero", ""); // HACK as long as it's not read, we can pretend it doesn't contain anything
}


// Helper methods - not part of the external stubs

char*
stub_pci_file(const char* device_name, const char* file_name) {
	char buffer[1024];
	snprintf(buffer, sizeof(buffer), "/sys/bus/pci/devices/%s/%s", device_name, file_name);
	return strdup(buffer);
}

char*
stub_pci_folder(const char* device_name) {
	char buffer[1024];
	snprintf(buffer, sizeof(buffer), "/sys/bus/pci/devices/%s", device_name);
	return strdup(buffer);
}

char*
stub_pci_addr(size_t addr)
{
	char buffer[] = "0x0000000000000000";
	int pos = strlen(buffer) - 1;
	while (addr > 0) {
		size_t digit = addr % 16;
		addr /= 16;

		if (digit < 10) {
			buffer[pos] = '0' + digit;
		} else {
			buffer[pos] = 'A' + (digit - 10);
		}

		pos--;
	}

	return strdup(buffer);
}

static int file_counter;
int
stub_add_file(char* path, char* content)
{
	struct stub_file file;
	memset(&file, 0, sizeof(struct stub_file));

	file.path = path;
	file.content = content;
        if (content == FILE_CONTENT_PAGEMAP ||
            content == FILE_CONTENT_HPINFO ||
            content == FILE_CONTENT_HUGEPAGE ) {

            file.content_len = 0;
        } else {
            file.content_len = strlen(content);
        }
	file.pos = POS_UNOPENED;
	file.kind = KIND_FILE;

	int fd = file_counter;
	file_counter++;

	FILES[fd] = file;

	return fd;
}

int
stub_add_bin_file(char* path, char* content, size_t content_len)
{
	struct stub_file file;
	memset(&file, 0, sizeof(struct stub_file));

	file.path = path;
	file.content = content;
        file.content_len = content_len;
	file.pos = POS_UNOPENED;
	file.kind = KIND_FILE;

	int fd = file_counter;
	file_counter++;

	FILES[fd] = file;

	return fd;
}

int
stub_add_link(char* path, char* content)
{
	int fd = stub_add_file(path, content);
	FILES[fd].kind = KIND_LINK;
	return fd;
}

int
stub_add_folder_array(char* path, int children_len, int* children)
{
	struct stub_file file;
	memset(&file, 0, sizeof(struct stub_file));

	file.path = path;
	file.children = children;
	file.children_len = children_len;
	file.pos = POS_UNOPENED;
	file.kind = KIND_DIRECTORY;

	int fd = file_counter;
	file_counter++;

	FILES[fd] = file;

	return fd;
}

int
stub_add_folder(char* path, int children_len, ...)
{
	va_list args;
	va_start(args, children_len);

	int* children = (int*) malloc(children_len * sizeof(int));
	for (int n = 0; n < children_len; n++) {
		children[n] = va_arg(args, int);
	}

	return stub_add_folder_array(path, children_len, children);
}
