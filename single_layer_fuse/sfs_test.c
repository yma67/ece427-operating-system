/* sfs_test.c 
 * 
 * Written by Robert Vincent for Programming Assignment #1.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sfs_api.h"

/* The maximum file name length. We assume that filenames can contain
 * upper-case letters and periods ('.') characters. Feel free to
 * change this if your implementation differs.
 */
#define MAX_FNAME_LENGTH 20   /* Assume at most 20 characters (16.3) */

/* The maximum number of files to attempt to open or create.  NOTE: we
 * do not _require_ that you support this many files. This is just to
 * test the behavior of your code.
 */
#define MAX_FD 100

/* The maximum number of bytes we'll try to write to a file. If you
 * support much shorter or larger files for some reason, feel free to
 * reduce this value.
 */
#define MAX_BYTES 30000 /* Maximum file size I'll try to create */
#define MIN_BYTES 10000         /* Minimum file size */

/* Just a random test string.
 */
static char test_str[] = "The quick brown fox jumps over the lazy dog.\n";
static char rick_and_morty[] = "To b fair, you have to have a very high IQ to understand Rick and Morty.";
static char ok_boomer[] = "Ok boomer";
static char modified_pasta[] = "Ok boomer, you have to have a very high IQ to understand Rick and Morty.";

/* rand_name() - return a randomly-generated, but legal, file name.
 *
 * This function creates a filename of the form xxxxxxxx.xxx, where
 * each 'x' is a random upper-case letter (A-Z). Feel free to modify
 * this function if your implementation requires shorter filenames, or
 * supports longer or different file name conventions.
 * 
 * The return value is a pointer to the new string, which may be
 * released by a call to free() when you are done using the string.
 */
 
char *rand_name() 
{
  char fname[MAX_FNAME_LENGTH];
  int i;

  for (i = 0; i < MAX_FNAME_LENGTH; i++) {
    if (i != 16) {
      fname[i] = 'A' + (rand() % 26);
    }
    else {
      fname[i] = '.';
    }
  }
  fname[i] = '\0';
  return (strdup(fname));
}

/* The main testing program
 */
int
main(int argc, char **argv)
{
  int i, j, k;
  int chunksize;
  int readsize;
  char *buffer;
  char fixedbuf[1024];
  int fds[MAX_FD];
  char *names[MAX_FD];
  int filesize[MAX_FD];
  int nopen;                    /* Number of files simultaneously open */
  int ncreate;                  /* Number of files created in directory */
  int error_count = 0;
  int tmp;

  mksfs(1);                     /* Initialize the file system. */

  /* First we open two files and attempt to write data to them.
   */
  for (i = 0; i < 2; i++) {
    names[i] = rand_name();
    fds[i] = sfs_fopen(names[i]);
    if (fds[i] < 0) {
      fprintf(stderr, "ERROR: creating first test file %s\n", names[i]);
      error_count++;
    }
    tmp = sfs_fopen(names[i]);
    if (tmp >= 0 && tmp != fds[i]) {
      fprintf(stderr, "ERROR: file %s was opened twice\n", names[i]);
      error_count++;
    }
    filesize[i] = (rand() % (MAX_BYTES-MIN_BYTES)) + MIN_BYTES;
  }

  for (i = 0; i < 2; i++) {
    for (j = i + 1; j < 2; j++) {
      if (fds[i] == fds[j]) {
        fprintf(stderr, "Warning: the file descriptors probably shouldn't be the same?\n");
      }
    }
  }

  printf("Two files created with zero length:\n");

  for (i = 0; i < 2; i++) {
    for (j = 0; j < filesize[i]; j += chunksize) {
      if ((filesize[i] - j) < 10) {
        chunksize = filesize[i] - j;
      }
      else {
        chunksize = (rand() % (filesize[i] - j)) + 1;
      }

      if ((buffer = malloc(chunksize)) == NULL) {
        fprintf(stderr, "ABORT: Out of memory!\n");
        exit(-1);
      }
      for (k = 0; k < chunksize; k++) {
        buffer[k] = (char) (j+k);
      }
      tmp = sfs_fwrite(fds[i], buffer, chunksize);
      if (tmp != chunksize) {
        fprintf(stderr, "ERROR: Tried to write %d bytes, but wrote %d\n",
                chunksize, tmp);
        error_count++;
      }
      free(buffer);
    }
  }

  if (sfs_fclose(fds[1]) != 0) {
    fprintf(stderr, "ERROR: close of handle %d failed\n", fds[1]);
    error_count++;
  }

  /* Sneaky attempt to close already closed file handle. */
  if (sfs_fclose(fds[1]) == 0) {
    fprintf(stderr, "ERROR: close of stale handle %d succeeded\n", fds[1]);
    error_count++;
  }

  printf("File %s now has length %d and %s now has length %d:\n",
         names[0], filesize[0], names[1], filesize[1]);

  /* Just to be cruel - attempt to read from a closed file handle.
   */
  if (sfs_fread(fds[1], fixedbuf, sizeof(fixedbuf)) > 0) {
    fprintf(stderr, "ERROR: read from a closed file handle?\n");
    error_count++;
  }

  fds[1] = sfs_fopen(names[1]);

  sfs_frseek(0, 0);
  sfs_frseek(1, 0);

  for (i = 0; i < 2; i++) {
    for (j = 0; j < filesize[i]; j += chunksize) {
      if ((filesize[i] - j) < 10) {
        chunksize = filesize[i] - j;
      }
      else {
        chunksize = (rand() % (filesize[i] - j)) + 1;
      }
      if ((buffer = malloc(chunksize)) == NULL) {
        fprintf(stderr, "ABORT: Out of memory!\n");
        exit(-1);
      }
      readsize = sfs_fread(fds[i], buffer, chunksize);

      if (readsize != chunksize) {
        fprintf(stderr, "ERROR: Requested %d bytes, read %d\n", chunksize, readsize);
        readsize = chunksize;
      }
      for (k = 0; k < readsize; k++) {
        if (buffer[k] != (char)(j+k)) {
          fprintf(stderr, "ERROR: data error at offset %d in file %s (%d,%d)\n",
                  j+k, names[i], buffer[k], (char)(j+k));
          error_count++;
          break;
        }
      }
      free(buffer);
    }
  }

  /*
   * Test read and write pointers
   */
  for (i = 0; i < 2; i++) {
    // first, set both read and write pointers to byte 0
    sfs_frseek(fds[i], 0);
    sfs_fwseek(fds[i], 0);

    // write content to file and verify
    tmp = sfs_fwrite(fds[i], rick_and_morty, strlen(rick_and_morty));
    if (tmp != strlen(rick_and_morty)) {
        fprintf(stderr, "ERROR: Tried to write a copypasta with %d bytes, "
                  "but only %d bytes written\n",
                  (int)strlen(rick_and_morty), tmp);
      error_count++;
    } else {
      buffer = malloc(strlen(rick_and_morty)+10);
      memset(buffer, 0, (strlen(rick_and_morty)+10)*sizeof(char));
      if ((readsize = sfs_fread(fds[i], buffer, tmp)) != tmp) {
        fprintf(stderr, "ERROR: My copypasta has %d bytes, but only %d bytes read\n",
                  (int)strlen(rick_and_morty), readsize);
        error_count++;
      } else {
        if (strcmp(buffer, rick_and_morty) != 0) {
          fprintf(stderr, "ERROR: File content is not the same, "
                    "was expecting Rick and Morty copypasta\n");
          error_count++;
        }
      }
      free(buffer);
    }

    // reset read and write pointers to byte 0
    sfs_frseek(fds[i], 0);
    sfs_fwseek(fds[i], 0);

    // overwrite content from byte 0 and verify
    tmp = sfs_fwrite(fds[i], ok_boomer, strlen(ok_boomer));
    if (tmp != strlen(ok_boomer)) {
      fprintf(stderr, "ERROR: Tried to write %d bytes, got %d bytes\n",
                (int)strlen(ok_boomer), tmp);
      error_count++;
    } else {
      tmp = (int)strlen(modified_pasta);
      buffer = malloc(tmp+10);
      memset(buffer, 0, (tmp+10)*sizeof(char));
      if ((readsize = sfs_fread(fds[i], buffer, tmp)) != tmp) {
        fprintf(stderr, "ERROR: Expected to read %d bytes, got %d instead\n",
                  tmp, readsize);
        error_count++;
      } else {
        if (strcmp(buffer, modified_pasta) != 0) {
          fprintf(stderr, "ERROR: Copypasta is not modified correctly, "
                    "was expecting 'Ok boomer' at the beginning\n");
          error_count++;
        }
      }
      free(buffer);
    }
  }

  for (i = 0; i < 2; i++) {
    if (sfs_fclose(fds[i]) != 0) {
      fprintf(stderr, "ERROR: closing file %s\n", names[i]);
      error_count++;
    }
  }

  /* Now try to close the files. Don't
   * care about the return codes, really, but just want to make sure
   * this doesn't cause a problem.
   */
  for (i = 0; i < 2; i++) {
    if (sfs_fclose(fds[i]) == 0) {
      fprintf(stderr, "Warning: closing already closed file %s\n", names[i]);
    }
    sfs_remove(names[i]);
  }

  /* Now just try to open up a bunch of files.
   */
  ncreate = 0;
  for (i = 0; i < MAX_FD; i++) {
    names[i] = rand_name();
    fds[i] = sfs_fopen(names[i]);
    if (fds[i] < 0) {
      break;
    }
    sfs_fclose(fds[i]);
    ncreate++;
  }

  printf("Created %d files in the root directory\n", ncreate);

  nopen = 0;
  for (i = 0; i < ncreate; i++) {
    fds[i] = sfs_fopen(names[i]);
    if (fds[i] < 0) {
      break;
    }
    nopen++;
  }
  printf("Simultaneously opened %d files\n", nopen);

  for (i = 0; i < nopen; i++) {
    tmp = sfs_fwrite(fds[i], test_str, strlen(test_str));
    if (tmp != strlen(test_str)) {
      fprintf(stderr, "ERROR: Tried to write %d, returned %d\n",
              (int)strlen(test_str), tmp);
      error_count++;
    }
    if (sfs_fclose(fds[i]) != 0) {
      fprintf(stderr, "ERROR: close of handle %d failed\n", fds[i]);
      error_count++;
    }
  }

  /* Re-open in reverse order */
  for (i = nopen-1; i >= 0; i--) {
    fds[i] = sfs_fopen(names[i]);
    if (fds[i] < 0) {
      fprintf(stderr, "ERROR: can't re-open file %s\n", names[i]);
    }
  }

  /* Now test the file contents.
   */
  for (i = 0; i < nopen; i++) {
      sfs_frseek(fds[i], 0);
  }

  for (j = 0; j < strlen(test_str); j++) {
    for (i = 0; i < nopen; i++) {
      char ch;

      if (sfs_fread(fds[i], &ch, 1) != 1) {
        fprintf(stderr, "ERROR: Failed to read 1 character\n");
        error_count++;
      }
      if (ch != test_str[j]) {
        fprintf(stderr, "ERROR: Read wrong byte from %s at %d (%d,%d)\n",
                names[i], j, ch, test_str[j]);
        error_count++;
        break;
      }
    }
  }

  /* Now close all of the open file handles.
   */
  for (i = 0; i < nopen; i++) {
    if (sfs_fclose(fds[i]) != 0) {
      fprintf(stderr, "ERROR: close of handle %d failed\n", fds[i]);
      error_count++;
    }
  }

  /* Now we try to re-initialize the system.
   */
  mksfs(0);

  for (i = 0; i < nopen; i++) {
    fds[i] = sfs_fopen(names[i]);
    sfs_frseek(fds[i], 0);
    if (fds[i] >= 0) {
      readsize = sfs_fread(fds[i], fixedbuf, sizeof(fixedbuf));
      if (readsize != strlen(test_str)) {
        fprintf(stderr, "ERROR: Read wrong number of bytes\n");
        error_count++;
      }

      for (j = 0; j < strlen(test_str); j++) {
        if (test_str[j] != fixedbuf[j]) {
          fprintf(stderr, "ERROR: Wrong byte in %s at %d (%d,%d)\n", 
                  names[i], j, fixedbuf[j], test_str[j]);
          printf("%d\n", fixedbuf[1]);
          error_count++;
          break;
        }
      }

      if (sfs_fclose(fds[i]) != 0) {
        fprintf(stderr, "ERROR: close of handle %d failed\n", fds[i]);
        error_count++;
      }
    }
  }

  printf("Trying to fill up the disk with repeated writes to %s.\n", names[0]);
  printf("(This may take a while).\n");

  /* Now try opening the first file, and just write a huge bunch of junk.
   * This is just to try to fill up the disk, to see what happens.
   */
  fds[0] = sfs_fopen(names[0]);
  if (fds[0] >= 0) {
    for (i = 0; i < 100000; i++) {
      int x;

      if ((i % 100) == 0) {
        fprintf(stderr, "%d\r", i);
      }

      memset(fixedbuf, (char)i, sizeof(fixedbuf));
      x = sfs_fwrite(fds[0], fixedbuf, sizeof(fixedbuf));
      if (x != sizeof(fixedbuf)) {
        /* Sooner or later, this write should fail. The only thing is that
         * it should fail gracefully, without any catastrophic errors.
         */
        printf("Write failed after %d iterations.\n", i);
        printf("If the emulated disk contains just over %d bytes, this is OK\n",
               (i * (int)sizeof(fixedbuf)));
        break;
      }
    }
    sfs_fclose(fds[0]);
  }
  else {
    fprintf(stderr, "ERROR: re-opening file %s\n", names[0]);
  }

  /* Now, having filled up the disk, try one more time to read the
   * contents of the files we created.
   */
  for (i = 0; i < nopen; i++) {
    fds[i] = sfs_fopen(names[i]);
    sfs_frseek(fds[i], 0);
    if (fds[i] >= 0) {
      readsize = sfs_fread(fds[i], fixedbuf, sizeof(fixedbuf));
      if (readsize < strlen(test_str)) {
        fprintf(stderr, "ERROR: Read wrong number of bytes\n");
        error_count++;
      }

      for (j = 0; j < strlen(test_str); j++) {
        if (test_str[j] != fixedbuf[j]) {
          fprintf(stderr, "ERROR: Wrong byte in %s at position %d (%d,%d)\n", 
                  names[i], j, fixedbuf[j], test_str[j]);
          error_count++;
          break;
        }
      }

      if (sfs_fclose(fds[i]) != 0) {
        fprintf(stderr, "ERROR: close of handle %d failed\n", fds[i]);
        error_count++;
      }
    }
  }

  fprintf(stderr, "Test program exiting with %d errors\n", error_count);
  return (error_count);
}

