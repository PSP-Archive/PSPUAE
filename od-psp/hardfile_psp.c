/**
  * UAE - The Un*x Amiga Emulator
  *
  * Hardfile emulation for PSP
  *
  * Copyright 2003 Richard Drummond
  * Based on hardfile_win32.c
  *
  * Note: this is a rather naive implementation. Sophistication and 64-bit
  * support still to come . . .
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include "filesys.h"
#include <stdlib.h>
#include <sys/types.h>
#include <pspiofilemgr.h>

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#include "fsusage.h"


#define write_comm_pipe_int(a,b,c)
#define read_comm_pipe_int_blocking(a) 0
#define write_comm_pipe_pvoid(a,b,c)
#define read_comm_pipe_pvoid_blocking(a) 0
#define init_comm_pipe(a,b,c)
#define comm_pipe_has_data(a) 0

#define write_comm_pipe_u32(a,b,c)

#define HDF_DEBUG 1
#ifdef  HDF_DEBUG
#define DEBUG_LOG write_log( "%s: ", __func__); write_log
#else
#define DEBUG_LOG(...) do ; while(0)
#endif

/* Fill in the fields of FSP with information about space usage for
   the filesystem on which PATH resides.
   DISK is the device on which PATH is mounted, for space-getting
   methods that need to know it.
   Return 0 if successful, -1 if not.  When returning -1, ensure that
   ERRNO is either a system error value, or zero if DISK is NULL
   on a system that requires a non-NULL value.  */
int
get_fs_usage (path, disk, fsp)
     const char *path;
     const char *disk;
     struct fs_usage *fsp;
{
    DEBUG_LOG ("get_fs_usage");

  return -1;
}

int utime(const char *path, const struct utimbuf *times)
{
	return 0;	
}

int chmod (const char *filename, mode_t mode)
{
	return 0;	
}


static int hdf_seek (struct hardfiledata *hfd, uae_u64 offset)
{
    off_t ret;

    DEBUG_LOG ("called with offset=0x%llx\n", offset);

    if (hfd->offset != hfd->offset2 || hfd->size != hfd->size2) {
		write_log ("hd: memory corruption detected in seek");
		abort ();
    }

    if (offset >= hfd->size) {
		write_log ("hd: tried to seek out of bounds!");
		abort ();
    }

    offset += hfd->offset;
    if (offset & (hfd->blocksize - 1)) {
		write_log ("hd: poscheck failed, offset not aligned to blocksize!");
		abort ();
    }

    if (offset >= 0x80000000) {
		write_log ("Failed to seek passed 2GB limit");
		return -1;
    }

    ret = sceIoLseek32 ((SceUID)hfd->handle, offset, SEEK_SET);
    if (ret == -1) {
		DEBUG_LOG ("seek failed\n");
		return -1;
    }

    DEBUG_LOG ("seek okay\n");
    return 0;
}

static void poscheck (struct hardfiledata *hfd, int len)
{
		DEBUG_LOG ("poscheck");
    off_t pos;

    if (hfd->offset != hfd->offset2 || hfd->size != hfd->size2) {
		write_log ("hd: memory corruption detected in poscheck");
		abort ();
    }

    pos = sceIoLseek32 ((SceUID)hfd->handle, 0, SEEK_CUR);

    if (pos == -1 ) {
		write_log ("hd: poscheck failed. seek failure, error");
		abort ();
    }
    if (len < 0) {
		write_log ("hd: poscheck failed, negative length!");
		abort ();
    }

    if ((uae_u64)pos < hfd->offset) {
		write_log ("hd: poscheck failed, offset out of bounds!");
		abort ();
    }
    if ((uae_u64)pos >= hfd->offset + hfd->size || (uae_u64)pos >= hfd->offset + hfd->size + len) {
		write_log ("hd: poscheck failed, offset out of bounds!");
		abort ();
    }
    if (pos & (hfd->blocksize - 1)) {
		write_log ("hd: poscheck failed, offset not aligned to blocksize! (0x%llx & 0x%04.4x = 0x%04.4x\n", pos, hfd->blocksize, pos & hfd->blocksize);
		abort ();
    }
}

int hdf_read (struct hardfiledata *hfd, void *buffer, uae_u64 offset, int len)
{
    int n;

    DEBUG_LOG ("called with offset=0x%llx len=%d\n", offset, len);

    hfd->cache_valid = 0;
    hdf_seek (hfd, offset);
    poscheck (hfd, len);
    n = sceIoRead ((SceUID)hfd->handle, buffer, len);

    DEBUG_LOG ("read %d bytes\n", n);

    return n;
}

int hdf_write (struct hardfiledata *hfd, void *buffer, uae_u64 offset, int len)
{
    int n;

    DEBUG_LOG ("called with offset=0x%llx len=%d\n", offset, len);

    hfd->cache_valid = 0;
    hdf_seek (hfd, offset);
    poscheck (hfd, len);
    n = sceIoWrite ((SceUID)hfd->handle, buffer, len);

    DEBUG_LOG ("Wrote %d bytes\n", n);

    return n;
}


int hdf_open (struct hardfiledata *hfd, char *name)
{
	DEBUG_LOG("hdf_open: %s", name);
	SceUID  fd;
	if(!(fd = sceIoOpen(name, hfd->readonly ? PSP_O_RDONLY : PSP_O_RDWR, 0777))) {
        write_log("unable to open hardfile");
		return 0;
	}
		
	int i;	
	hfd->handle = (void *) fd;
	hfd->cache = 0;	
	i = strlen (name) - 1;
	while (i >= 0) {
	    if ((i > 0 && (name[i - 1] == '/' || name[i - 1] == '\\')) || i == 0) {
		strcpy (hfd->vendor_id, "UAE");
		strncpy (hfd->product_id, name + i, 15);
		strcpy (hfd->product_rev, "0.2");
		break;
	    }
	    i--;
	}
	int size = sceIoLseek32 (fd, 0, SEEK_END);
	DEBUG_LOG("hdf size: %i", size);
	hfd->size = hfd->size2 = size;
	hfd->blocksize = 512;
	sceIoLseek32 (fd, 0, SEEK_SET);


    return 1;
}

void hdf_close (struct hardfiledata *hfd)
{
    DEBUG_LOG ("called\n");

    sceIoClose ((SceUID)hfd->handle);
}

extern int hdf_dup (struct hardfiledata *hfd, void *src)
{
    DEBUG_LOG ("called\n");	
    return 0;
}

