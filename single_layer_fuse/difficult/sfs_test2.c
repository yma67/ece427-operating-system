#include "tests.h"

//A more difficult test. Will attempt to overload your file system. 
//For all tests, -1 is considered error and 0 is considered success. 
int difficult_test(){
  char **write_buf;
  int file_id[ABS_CAP_FD];
  int *write_ptr = calloc(ABS_CAP_FD, sizeof(int));
  char *file_names[ABS_CAP_FD];
  int *file_size = calloc(ABS_CAP_FD, sizeof(int));
  int iterations = 10;
  int err_no = 0;
  int res;
  int num_file = MAX_FD;
  printf("\n-------------------------------\nInitializing Difficult test.\n--------------------------------\n\n");
  write_buf = calloc(MAX_FD, sizeof(char *));

  for(int i = 0; i < MAX_FD; i++)
    write_buf[i] = calloc(MAX_BYTES + 1, sizeof(char));

  mksfs(1);                     /* Initialize the file system. */
  //Attemping to crash the system with overflowing fopens
  //This function will remove all files after it's done.
  test_overflow_open(file_id, file_size, write_ptr, file_names, write_buf, ABS_CAP_FD, &err_no);
  //So we have to open new files. 
  test_open_new_files(file_names, file_id, num_file, &err_no);
  test_get_file_name(file_names, num_file, &err_no);
  //Heavy write into the file system
  for(int i = 0; i < iterations; i++){
    res = test_difficult_write_files(file_id, file_size, write_ptr, write_buf, num_file, &err_no);
    if(res < 0)
      break;
    //File sizes should match
    test_get_file_size(file_size, file_names, num_file, &err_no);
  }
  // //More heavy random access memory reads
  for(int i = 0; i < iterations; i++){
    test_random_read_files(file_id, file_size, write_ptr, write_buf, num_file, &err_no);
  }
  //Test out a few errors you should catch  
  test_read_write_out_of_bound(file_id, file_size, write_buf, num_file, &err_no);
  //Read All
  test_read_all_files(file_id, file_size, write_buf, num_file, &err_no);
  //Crank the file system to max for two files. Assumes we have two files at least. 
  test_write_to_overflow(file_id, file_size, write_buf, 0, &err_no);
  test_write_to_overflow(file_id, file_size, write_buf, 1, &err_no);

  // //Remove all files
  test_close_files(file_names, file_id, num_file, &err_no);
  test_remove_files(file_id, file_size, write_ptr, file_names, write_buf, num_file, &err_no);
  //Free old files names
  free_name_element(file_names, num_file);
  //Open up new files with new names. 
  test_open_new_files(file_names, file_id, num_file, &err_no);
  //Test if the new file names match. 
  test_get_file_name(file_names, num_file, &err_no);
  for(int i = 0; i < iterations; i++){
    res = test_difficult_write_files(file_id, file_size, write_ptr, write_buf, num_file, &err_no);
    if(res < 0)
      break;
    //At each step, we test the file size to make it sure it matches 
    test_get_file_size(file_size, file_names, num_file, &err_no);
    //We also play around with the seek by offsetting. 
    test_seek(file_id, file_size, write_ptr, write_buf, num_file, 10, &err_no);
  }
  //More heavy random access memory reads
  for(int i = 0; i < iterations; i++){
    test_random_read_files(file_id, file_size, write_ptr, write_buf, num_file, &err_no);
  }  

  test_read_all_files(file_id, file_size, write_buf, num_file, &err_no);
  //Recreate the file system but stale. 
  mksfs(0);
  test_open_old_files(file_names, file_id, num_file, &err_no);
  //More heavy random access memory reads
  for(int i = 0; i < iterations; i++){
    test_random_read_files(file_id, file_size, write_ptr, write_buf, num_file, &err_no);
  }  
  //Final round
  test_read_all_files(file_id, file_size, write_buf, num_file, &err_no);
  //Remove all files
  test_close_files(file_names, file_id, num_file, &err_no);
  test_remove_files(file_id, file_size, write_ptr, file_names, write_buf, num_file, &err_no);

  printf("\n-------------------------------\nDifficult test Finished.\nCurrent Error Num: %d\n--------------------------------\n\n", err_no);
  free_name_element(file_names, num_file);
  free(file_size);
  for(int i = 0; i < MAX_FD; i++){
    free(write_buf[i]);
  }
  free(write_buf);
  free(write_ptr);
  return 0;
}

/* The main testing program
 */
int main(int argc, char **argv){
  difficult_test();
}
