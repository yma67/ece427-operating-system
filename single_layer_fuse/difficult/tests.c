#include "tests.h"

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
int test_num = 1;

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

/*
Generates garbage as text. 
Mallocs size of length + 1. Needs to be freed. 
*/

char *rand_text(int length){
    char *ret = calloc(length+1, sizeof(char));
    for(int i = 0; i < length; i++){
        ret[i] = (char)(rand() % 96 + 32);
    }
    ret[length] = '\0';
    return ret;
}

/*
Plays around with frseek and fwseek. Will shift the read and write pointer back by offset at the end if nothing fails. 
If offset is greater than write pointer, write pointer is set to zero. 
*/
int test_seek(int *file_id, int *file_size, int *write_ptr, char **write_buf, int num_file, int offset, int *err_no){
  int res;
  for(int i = 0; i < num_file; i++){
    //Just testing the shift for beyond seek boundaries before actually doing it. 
    res = sfs_frseek(file_id[i], -1);
    if(res >= 0)
      fprintf(stderr, "Warning: sfs_frseek returned positive. Negative seek location attempted. Potential frseek fail?\n");
    res = sfs_frseek(file_id[i], file_size[i] + 100);
    if(res >= 0)
      fprintf(stderr, "Warning: sfs_frseek returned positive. Seek location beyond file size attempted. Potential frseek fail?\n");
    res = sfs_fwseek(file_id[i], -1);
    if(res >= 0)
      fprintf(stderr, "Warning: sfs_frseek returned positive. Negative seek location attempted. Potential fwseek fail?\n");
    res = sfs_fwseek(file_id[i], file_size[i] + 100);
    if(res >= 0)
      fprintf(stderr, "Warning: sfs_frseek returned positive. Seek location beyond file size attempted. Potential fwseek fail?\n");
    res = sfs_frseek(file_id[i], file_size[i] - offset);
    if(res < 0)
      fprintf(stderr, "Warning: sfs_frseek returned negative. Potential frseek fail?\n");
    res = sfs_fwseek(file_id[i], file_size[i] - offset);
    if(res < 0)
      fprintf(stderr, "Warning: sfs_fwseek returned negative. Potential fwseek fail?\n");
    write_ptr[i] -= offset;
    if(write_ptr[i] < 0)
      write_ptr[i] = 0;
    //file_size[i] -= 10;
    write_buf[i][file_size[i]] = '\0';
  }
  return 0;
}

/*
Attempts to Reads all currently written text. If rseek fails, this will likely fail as well. 
*/
int test_read_all_files(int *file_id, int *file_size, char **write_buf, int num_file, int *err_no){
  int res;
  char *buf = calloc(MAX_BYTES + 1, sizeof(char));
  for(int i = 0; i < num_file; i++){
    //shift to start
    res = sfs_frseek(file_id[i], 0);
    if(res < 0)
      fprintf(stderr, "Warning: sfs_frseek returned negative. Potential frseek fail?\n");
    res = sfs_fread(file_id[i], buf, file_size[i]);
    //Just a precaution. Don't think it's actually necessary
    buf[file_size[i]] = '\0';
    if(res != file_size[i])
      fprintf(stderr, "Warning: sfs_fread should return number of bytes read. Potential read fail?\n");
    //Compare both
    if(strcmp(buf, write_buf[i]) != 0){
      fprintf(stderr, "Error: \nRead failed.\n\n");
      *err_no += 1;
      printf("%d %d %d\n", strlen(buf), strlen(write_buf[i]), file_size[i]);
    }
  }
  free(buf);
  printf("\n-------------------------------\nTest_num[%d]: Current Error Num: %d\n--------------------------------\n\n", test_num, *err_no);
  test_num++;
  return 0;
}

/*
Reads a simple sentence from the block. The sentence is test_str
*/
int test_simple_read_files(int *file_id, int *file_size, char **write_buf, int num_file, int *err_no){
  int res;
  char buf[512];
  for(int i = 0; i < num_file; i++){
    //Read at the read_ptr location
    res = sfs_fread(file_id[i], buf, strlen(test_str));
    buf[strlen(test_str)] = '\0';
    if(res != strlen(test_str))
      fprintf(stderr, "Warning: sfs_fread should return number of bytes read. Potential read fail?\n");
    if(strcmp(buf, test_str) != 0){
      fprintf(stderr, "Error: \nRead failed. Read:\n%s\nShould have Read:\n%s\n", buf, test_str);
      *err_no += 1;
    }
  }
  printf("\n-------------------------------\nTest_num[%d]: Current Error Num: %d\n--------------------------------\n\n", test_num, *err_no);
  test_num++;
  return 0;
}

/*
Writes a single sentence into the block. 
*/
int test_simple_write_files(int *file_id, int *file_size, int *write_ptr, char **write_buf, int num_file, int *err_no){
  int res;
  for(int i = 0; i < num_file; i++){
    //Write a sentence at the write_ptr location
    memcpy(write_buf[i] + write_ptr[i], test_str, strlen(test_str));
    write_ptr[i] += strlen(test_str);
    if(write_ptr[i] > file_size[i])
      file_size[i] = write_ptr[i];
    write_buf[i][file_size[i]] = '\0'; 
    res = sfs_fwrite(file_id[i], test_str, strlen(test_str));
    if(res != strlen(test_str))
      fprintf(stderr, "Warning: sfs_fwrite should return number of bytes written. Potential write fail?\n");
  }
  return 0;
}

/*
Helper function for test_difficult_write_files
Will read the data that was written in. 
*/
int test_difficult_read_files(int *file_id, int *file_size, int *write_ptr, char **write_buf, int index, int read_length, int *err_no){
    int res;
    char *buf = calloc(read_length + 1, sizeof(char));
    char temp;
    //Just reads from the starting location and read_length long 
    res = sfs_fread(file_id[index], buf, read_length);
    buf[read_length] = '\0';
    if(res != read_length)
        fprintf(stderr, "Warning: sfs_fread should return number of bytes read. Potential read fail?\n");
    //Just a trick so we can use strcmp
    temp = write_buf[index][write_ptr[index]];
    write_buf[index][write_ptr[index]] = '\0';
    if(strcmp(buf, write_buf[index] + sizeof(char) * (write_ptr[index] - read_length)) != 0){
        fprintf(stderr, "Error: \nRead failed\n");
        *err_no += 1;
    }
    write_buf[index][write_ptr[index]] = temp;
    free(buf);
    return 0;
}

/*
Randomly reads a chunk of data. Compares it with stored values. 
*/
int test_random_read_files(int *file_id, int *file_size, int *write_ptr, char **write_buf ,int num_file, int *err_no){
  int res;
  int read_length;
  int start_index;
  char temp;
  char *buf;
  for(int i = 0; i < num_file; i++){
    //Pick a random read length. 
    read_length = rand()%MAX_WRITE_BYTE * 2;
    //Re adjust the file length. 
    if(file_size[i] < read_length)
        read_length = file_size[i];
    buf = calloc(read_length + 1, sizeof(char));

    //Get start location
    if(file_size[i] - read_length < 2)
      start_index = 0;
    else
      start_index = rand()%(file_size[i] - read_length);

    //Shift read pointer
    res = sfs_frseek(file_id[i], start_index);
    if(res < 0)
      fprintf(stderr, "Warning: sfs_frseek returned negative. Potential frseek fail?\n");
    res = sfs_fread(file_id[i], buf, read_length);
    if(res != read_length)
      fprintf(stderr, "Warning: sfs_fread should return number of bytes read. Potential read fail?\n");
    //A little trick so we can use strcmp
    temp = write_buf[i][start_index + read_length];
    write_buf[i][start_index + read_length] = '\0';
    if(strcmp(buf, write_buf[i] + sizeof(char) * (start_index)) != 0){
      fprintf(stderr, "Error: \nRead failed\n");
      *err_no += 1;
    }
    write_buf[i][start_index + read_length] = temp;
    free(buf);
  }
  printf("\n-------------------------------\nTest_num[%d]: Current Error Num: %d\n--------------------------------\n\n", test_num, *err_no);
  test_num++;
  return 0;
}

/*
Difficult Write. Will large single file writes randomly. 
This will end up shifting the read pointer to location of current write pointer. 
*/
int test_difficult_write_files(int *file_id, int *file_size, int *write_ptr, char **write_buf, int num_file, int *err_no){
  int res;
  char *text;
  int rand_offset;
  for(int i = 0; i < num_file; i++){
    //Offset is how much we decrease the write pointer
    if(file_size[i] > 1)
        rand_offset = (rand()%file_size[i]);
    else
        rand_offset = 0;
    //We want the size to mostly increase so we'll load the dice so to speak. 
    //More likely the file will increase instead of getting the write pointer shifted a lot. 
    if(rand_offset > file_size[i]/3 && (rand() % 100) > 30)
        rand_offset /= 3;
    if(write_ptr[i] < rand_offset)
        write_ptr[i] = 1;
    else
        write_ptr[i] -= rand_offset;
    
    //Generate text length of random size
    text = rand_text(rand() % MAX_WRITE_BYTE);
    //If we reach the max bytes, we will stop. 
    if(write_ptr[i] + strlen(text) >= MAX_BYTES - 1){
        free(text);
        printf("\n-------------------------------\nTest_num[%d]: Current Error Num: %d\n--------------------------------\n\n", test_num, *err_no);
        test_num++;
        return -1;
    }
    memcpy(write_buf[i] + sizeof(char) * write_ptr[i], text, strlen(text));
    //Change write location 
    res = sfs_fwseek(file_id[i], write_ptr[i]);
    if(res < 0)
      fprintf(stderr, "Warning: sfs_fwseek returned negative. Potential fwseek fail?\n");
    res = sfs_frseek(file_id[i], write_ptr[i]);
    //Shift the read location as well 
    if(res < 0)
      fprintf(stderr, "Warning: sfs_frseek returned negative. Potential frseek fail?\n");
    res = sfs_fwrite(file_id[i], text, strlen(text));
    write_ptr[i] += strlen(text);
    if(write_ptr[i] > file_size[i]){
        file_size[i] = write_ptr[i];
    }
    //Do some house keeping
    write_buf[i][file_size[i]] = '\0'; 
    if(res != strlen(text))
      fprintf(stderr, "Warning: sfs_fwrite should return number of bytes written. Potential write fail?\n");
    //Read test for if we wrote in properly
    test_difficult_read_files(file_id, file_size, write_ptr, write_buf, i, strlen(text), err_no);
    free(text);
  }
  printf("\n-------------------------------\nTest_num[%d]: Current Error Num: %d\n--------------------------------\n\n", test_num, *err_no);
  test_num++;
  return res;
}


/*
Attempts to fill up all the files to max.
This should return an error at the end. 
*/
int test_write_to_overflow(int *file_id, int *file_size, char **write_buf, int index, int *err_no){
  char * text;
  int res;
  int i = index;
  printf("Attempting to write to file cap. This will take a while\n");
      //Change the seek to the end of file.  
  res = sfs_fwseek(file_id[i], file_size[i]);
  if(res < 0)
        fprintf(stderr, "Warning: sfs_frseek returned negative. Potential frseek fail?\n");
  //loop until we hit the absolute cap or your sfs_fwrite fails. 
  for(;;){
    text = rand_text(MAX_WRITE_BYTE);
    //Reached 500 KB, we can stop now. 
    if(file_size[i] + strlen(text) > ABS_CAP_FILE_SIZE){
        printf("Reached my cap. This is OK.\nWrote %d bytes\n", file_size[i]);
        free(text);
        return 0;
    }
    file_size[i] += strlen(text);
    //When it fails, it should be OK. 
    res = sfs_fwrite(file_id[i], text, strlen(text));
    if(res <= 0){
        printf("Reached maximum file capacity. This is OK.\n Wrote %d bytes\n", file_size[i]);
        free(text);
        return 0;
    }else if(res != MAX_WRITE_BYTE){ //Else if the write isn't our file length, it's an error
        fprintf(stderr, "Error: Invalid write. \nWrote %d when was supposed to be %d\n\n", res, MAX_WRITE_BYTE);
        *err_no += 1;
    }
    free(text);
  }

  printf("\n-------------------------------\nTest_num[%d]: Current Error Num: %d\n--------------------------------\n\n", test_num, *err_no);
  test_num++;
  return 0;
}

/*
Attempts write out of bound and read out of bounds. 
These should return error. 
*/
int test_read_write_out_of_bound(int *file_id, int *file_sizes, char **file_names, int num_file, int *err_no){
    int res;
    char *buf;
    for(int i = 0; i < num_file; i++){
        //Attempt to write with a Negative length. Should return error. 
        res = sfs_fwrite(file_id[i], "OI", -1);
        if(res > 0){
            fprintf(stderr, "Error: Invalid write length.\nWrote %d when was supposed to be %d\n\n", res, -1);
            *err_no += 1;
            break;
        }else{
          fprintf(stderr, "Returned error. This is Ok\n");
        }
        //Attempt to read with a Negative length. Should return error. 
        res = sfs_fread(file_id[i], buf, -1);
        if(res > 0){
        fprintf(stderr, "Error: sfs_fread returned positive length. Requested %d read, Read %d\n", -1, res);
        *err_no += 1;
        }else{
          fprintf(stderr, "Returned error. This is Ok\n");
        }
        //Attempt to read far larger than file. Should return error. 
        buf = calloc(file_sizes[i] + ABS_CAP_FILE_SIZE, sizeof(char));
        //Shift read pointer to 0
        res = sfs_frseek(file_id[i], 0);
        if(res < 0)
          fprintf(stderr, "Warning: sfs_frseek returned positive. Negative seek location attempted. Potential frseek fail?\n");
        res = sfs_fread(file_id[i], buf, file_sizes[i] + ABS_CAP_FILE_SIZE);
        //When i read over the file size, return the maximum read amount
        if(res != file_sizes[i]){
            fprintf(stderr, "Error: Length read should be file size\nRequested %d to read, Should be Read: %d, Read %d\n", ABS_CAP_FILE_SIZE + file_sizes[i], file_sizes[i], res);
            *err_no += 1;
        }
        free(buf);
    }
    printf("\n-------------------------------\nTest_num[%d]: Current Error Num: %d\n--------------------------------\n\n", test_num, *err_no);
    test_num++;
    return 0;
}

/*
Attempt to open maximum file descriptors possible until maximum reached. 
We will attempt this exactly twice. 
WILL REMOVE ALL FILES OPENED AT THE END OF test
Thus, no files will be open by the end of this test. 
*/
int test_overflow_open(int *file_id, int *file_sizes, int *write_ptr, char **file_names, char **write_buf, int num_file, int *err_no){
  int ret = 0;
  //We are basically generate as many new names as possible. 
  for(int i = 0; i < num_file; i++){
    file_names[i] = rand_name();
    file_id[i] = sfs_fopen(file_names[i]);
    //If we hit the cap and the file_id return is negative, we are good and stop
    if(file_id[i] < 0){
        printf("File %s failed to open\n", file_names[i]);
        printf("Maximum number of files opened. This is ok. \n");
        ret = i;
        free(file_names[i]);
        file_names[i] = NULL;
        break;
    }
    printf("File Opened %s\n", file_names[i]);
  }
  //Check if we got the same file id for different files. 
  for (int i = 0; i < ret; i++) {
    for (int j = i + 1; j < ret; j++) {
      if (file_id[i] == file_id[j]) {
        fprintf(stderr, "Warning: the file descriptors probably shouldn't be the same?\n");
      }
    }
  }
  //Remove them all and do it again. 
  test_remove_files(file_id, file_sizes, write_ptr, file_names, write_buf, ret, err_no);
  free_name_element(file_names, ret);
  //We are basically generate as many new names as possible. 
  for(int i = 0; i < num_file; i++){
    file_names[i] = rand_name();
    file_id[i] = sfs_fopen(file_names[i]);
    //If we hit the cap and the file_id return is negative, we are good and stop
    if(file_id[i] < 0){
        printf("File %s failed to open\n", file_names[i]);
        printf("Maximum number of files opened.\n");
        ret = i;
        free(file_names[i]);
        file_names[i] = NULL;
        break;
    }
    printf("File Opened %s\n", file_names[i]);
  }
  //Check if we got the same file id for different files. 
  for (int i = 0; i < ret + 1; i++) {
    for (int j = i + 1; j < ret; j++) {
      if (file_id[i] == file_id[j]) {
        fprintf(stderr, "Warning: the file descriptors probably shouldn't be the same?\n");
      }
    }
  }
  //Remove all the files. 
  test_remove_files(file_id, file_sizes, write_ptr, file_names, write_buf, ret, err_no);
  free_name_element(file_names, ret);
  printf("\n-------------------------------\nTest_num[%d]: Current Error Num: %d\n--------------------------------\n\n", test_num, *err_no);
  test_num++;

  //Returns the max number of files your file system can support or 1000 I think. 
  return ret;
}


/*
Test if file size match the stored file size. 
We assume 1 character = 1 byte.
*/
int test_get_file_size(int *file_size, char **file_names, int num_file, int *err_no){
    int size;
    for(int i = 0; i < num_file; i++){
        size = sfs_get_file_size(file_names[i]);
        if(size != file_size[i]){
            fprintf(stderr, "ERROR: Invalid file size for file %s.\nGiven: %d, Actual: %d\n", file_names[i], size, file_size[i]);
            *err_no += 1;
        }
            
    }
    printf("\n-------------------------------\nTest_num[%d]: Current Error Num: %d\n--------------------------------\n\n", test_num, *err_no);
    test_num++;
    return 0;
}

/*
Test if all the files names exists in our file name list. Doesn't have to be in order inserted
but the order given should always be the same when looped around.
This means that I can have 
file1
file2
file3

get_file_name x 3 returns

file2
file3
file1 
That is OK.

But on the next get_file_name x 3,
I should get 

file2
file3
file1 
That is what is meant by order. 
*/
int test_get_file_name(char **file_names, int num_file, int *err_no){
    char **name_list = calloc(MAX_FD, sizeof(char**));
    char *name = NULL;
    int res;
    int found =0;
    //Print a list of current file names
    for(int i = 0; i < num_file; i++){
      printf("File_name list: %s\n", file_names[i]);
    }

    //Collect list of file names
    for(int i = 0; i < num_file; i++){
        name_list[i] = calloc(MAX_FNAME_LENGTH*2, sizeof(char));
        res = sfs_get_next_file_name(name_list[i]);
        if (res < 0) {
            fprintf(stderr, "Warning: the sfs_get_next_file_name returned negative values\n");
        }
    }

    //Try to find the word in the list. If not exist, then errors
    for(int j = 0; j < num_file; j++){
      for(int i = 0; i < num_file; i++){
          if(strcmp(name_list[i], file_names[j]) == 0){
              found++;
              break;
          }
      }
      if(!found){
          fprintf(stderr, "ERROR: Invalid file name.\nGiven: %s\n", name);
          *err_no += 1;
      }
    }

    name = calloc(MAX_FNAME_LENGTH*2, sizeof(char));
    res = sfs_get_next_file_name(name);
    if(res != 0){
        fprintf(stderr, "ERROR: End of namelist. Should return 0.\n");
        *err_no += 1;
    }
    free(name);


    for(int i = 0; i < num_file; i++){
      printf("Collected list: %s\n", name_list[i]);
    }
    //Now check the get_file_name order is consistent. 
    for(int i = 0; i < num_file; i++){
        name = calloc(MAX_FNAME_LENGTH*2, sizeof(char));
        res = sfs_get_next_file_name(name);
        if(strcmp(name, name_list[i]) != 0){
          fprintf(stderr, "ERROR: File name not in order.\nGiven: %s\n", name);
          *err_no += 1;
        }
        free(name_list[i]);
        free(name);
    }
    name = calloc(MAX_FNAME_LENGTH*2, sizeof(char));
    res = sfs_get_next_file_name(name);
    if(res != 0){
        fprintf(stderr, "ERROR: End of namelist. Should return 0.\n");
        *err_no += 1;
    }
    free(name);
    printf("\n-------------------------------\nTest_num[%d]: Current Error Num: %d\n--------------------------------\n\n", test_num, *err_no);
    test_num++;
    free(name_list);
    return 0;
}


/*
Attempts to remove files(AKA delete the file from the system)
Will only give warnings. 
*/
int test_remove_files(int *file_id, int *file_size, int *write_ptr, char **file_names, char **write_buf, int num_file, int *err_no){
  int res;
  for(int i = 0; i < num_file && i < ABS_CAP_FD; i++){
    printf("File Removed %s\n", file_names[i]);
    res = sfs_remove(file_names[i]);
    if(res < 0)
      fprintf(stderr, "Warning: sfs_fclose returned negative value. Potential fclose fail?\n");
    file_size[i] = 0;
    write_ptr[i] = 0;
    free(file_names[i]);
    file_names[i] = NULL;
  }
  printf("\n-------------------------------\nTest_num[%d]: Current Error Num: %d\n--------------------------------\n\n", test_num, *err_no);
  test_num++;
  return 0;
}

/*
Attempt to close the files. 
Will test if the file is open by
Attempting to close the same file
Attemping to close using a negative file id
Attemping to close using an ID that's beyond the currently assigned IDs by 1. This should return -1. 
Attempting to read the closed file
*/
int test_close_files(char **file_names, int *file_id, int num_file, int *err_no){
  int res;
  int max = 0;
  char buf[64];
  for(int i = 0; i < num_file; i++)
    if(file_id[i] > max)
      max = file_id[i];
  for(int i = 0; i < num_file; i++){
    res = sfs_fclose(file_id[i]);
    if(res < 0){
      fprintf(stderr, "ERROR: sfs_fclose failed %s\n", file_names[i]);
      *err_no += 1;
    }
    //Try closing the file again
    res = sfs_fclose(file_id[i]);
    if(res >= 0){
      fprintf(stderr, "Error: sfs_fclose returned positive. Did it actually close the file %s?\n", file_names[i]);
      *err_no += 1;
    }else{
      fprintf(stderr, "Failed to close. This is OK.\n");
    }
    //Now I'll just try to close a file with negative file_id
    res = sfs_fclose(-MAX_BYTES);
    if(res >= 0){
      fprintf(stderr, "Error: sfs_fclose returned positive. Attempted to close a file %s with file_id %d\n", file_names[i], -MAX_BYTES);
      *err_no += 1;
    }else{
      fprintf(stderr, "Failed to close. This is OK.\n");
    }
    //Finally, we end this with an attempt to open beyond the current file limit. 
    res = sfs_fclose(max + 1);
    if(res >= 0){
      fprintf(stderr, "Error: sfs_fclose returned positive. Attempted to close a file %s with file_id %d\n", file_names[i], 1 + max);
      *err_no += 1;
    }else{
      fprintf(stderr, "Failed to close. This is OK.\n");
    }
    //Attempts to write closed file. This should fail. 
    res = sfs_fread(file_id[i], buf, strlen(test_str));
    if(res > 0){
      fprintf(stderr, "Error: sfs_fread returned positive length. File %s should be closed\n", file_names[i]);
      *err_no += 1;
    }else{
      fprintf(stderr, "Failed to read. This is OK.\n");
    }
  }
  printf("\n-------------------------------\nTest_num[%d]: Current Error Num: %d\n--------------------------------\n\n", test_num, *err_no);
  test_num++;
  return 0;
}

/*
Will attempt to create new files
Will give error if different files has same file descriptors. 
*/
int test_open_new_files(char **file_names, int *file_id, int num_file, int *err_no){
  if(num_file < 1)
    return 0;
  //We always keep this one close to heart
  file_names[0] = strdup("FINAL_ANSWERS.pdf");
  file_id[0] = sfs_fopen(file_names[0]);
  printf("File Opened %s\n", file_names[0]);
  if (file_id[0] < 0) {
    fprintf(stderr, "ERROR: Cannot open file %s\n", file_names[0]);
    *err_no += 1;
  }
  //Now do the same for the rest
  for(int i = 1; i < num_file; i++){
    file_names[i] = rand_name();
    printf("File Opened %s\n", file_names[i]);
    file_id[i] = sfs_fopen(file_names[i]);
    if (file_id[i] < 0) {
      fprintf(stderr, "ERROR: Cannot open file %s\n", file_names[i]);
      *err_no += 1;
    }
  }
  //Test if the file ids given are all different
  for (int i = 0; i < num_file; i++) {
    for (int j = i + 1; j < num_file; j++) {
      if (file_id[i] == file_id[j]) {
        fprintf(stderr, "Warning: the file descriptors probably shouldn't be the same?\n");
      }
    }
  }
  printf("\n-------------------------------\nTest_num[%d]: Current Error Num: %d\n--------------------------------\n\n", test_num, *err_no);
  test_num++;
  return 0;
}

/*
Reopen existing files. 
Hence, it will not generate new names, but reutilize names in the file_names array
Will give error if different files has same file descriptors. 
*/
int test_open_old_files(char **file_names, int *file_id, int num_file, int *err_no){
  if(num_file < 1)
    return 0;
  //We don't generate new file names but same as above
  file_id[0] = sfs_fopen(file_names[0]);
  printf("File Opened %s\n", file_names[0]);
  if (file_id[0] < 0) {
    fprintf(stderr, "ERROR: Cannot open file %s\n", file_names[0]);
    *err_no += 1;
  }
  for(int i = 1; i < num_file; i++){
    printf("File Opened %s\n", file_names[i]);
    file_id[i] = sfs_fopen(file_names[i]);
    if (file_id[i] < 0) {
      fprintf(stderr, "ERROR: Cannot open file %s\n", file_names[i]);
      *err_no += 1;
    }
  }
  for (int i = 0; i < num_file; i++) {
    for (int j = i + 1; j < num_file; j++) {
      if (file_id[i] == file_id[j]) {
        fprintf(stderr, "Warning: the file descriptors probably shouldn't be the same?\n");
      }
    }
  }
  printf("\n-------------------------------\nTest_num[%d]: Current Error Num: %d\n--------------------------------\n\n", test_num, *err_no);
  test_num++;
  return 0;
}

int free_name_element(char **name_list, int num_file){
  for(int i = 0; i < num_file; i++)
    free(name_list[i]);
  return 0;
}