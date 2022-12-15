#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
  // Check number of arguments
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <config_file>\n", argv[0]);
    return 1;
  }

  // Open config file
  FILE *fp = fopen(argv[1], "r");
  if (fp == NULL) {
    fprintf(stderr, "Error: Could not open config file '%s'\n", argv[1]);
    return 1;
  }

  // Read locations from config file
  char folder_loc[FILENAME_MAX], input_loc[FILENAME_MAX], output_loc[FILENAME_MAX];
  if (fscanf(fp, "%s\n%s\n%s", folder_loc, input_loc, output_loc) != 3) {
    fprintf(stderr, "Error: Invalid config file format\n");
    return 1;
  }
  fclose(fp);

  // Open folder containing subfolders
  DIR *dir = opendir(folder_loc);
  if (dir == NULL) {
    fprintf(stderr, "Error: Could not open folder '%s'\n", folder_loc);
    return 1;
  }

  // Open results file
  FILE *results_fp = fopen("results.csv", "w");
  if (results_fp == NULL) {
    fprintf(stderr, "Error: Could not open results file\n");
    return 1;
  }

  // Loop through subfolders
  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    // Skip current and parent directories
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    // Concatenate subfolder location with C file location
    char c_file_loc[FILENAME_MAX];
    snprintf(c_file_loc, FILENAME_MAX, "%s/%s/main.c", folder_loc, entry->d_name);

    // Fork process
    pid_t pid = fork();
    if (pid == 0) {
      // Compile C file in child process
      char compile_cmd[FILENAME_MAX];
      snprintf(compile_cmd, FILENAME_MAX, "gcc %s -o %s/%s/main.out", c_file_loc, folder_loc, entry->d_name);
      if (system(compile_cmd) != 0) {
        fprintf(stderr, "Error: Could not compile '%s'\n", c_file_loc);
        return 1;
      }

      // Run compiled file with input and redirect output
      char run_cmd[FILENAME_MAX];
        snprintf(run_cmd, FILENAME_MAX, "%s/%s/main.out < %s > %s/%s/output.txt", folder_loc, entry->d_name, input_loc, folder_loc, entry->d_name);
        if (system(run_cmd) != 0) {
        fprintf(stderr, "Error: Could not run '%s'\n", c_file_loc);
        return 1;
        }
        // Compare output with correct output
        char compare_cmd[FILENAME_MAX];
        snprintf(compare_cmd, FILENAME_MAX, "comp.out %s %s/%s/output.txt", output_loc, folder_loc, entry->d_name);
        int score = system(compare_cmd);

        // Write username and score to results file
        fprintf(results_fp, "%s,%d\n", entry->d_name, score);

        // Exit child process
        return 0;
        }
    }

    // Wait for child processes to finish
    while (wait(NULL) > 0);

    // Close directory and results file
    closedir(dir);
    fclose(results_fp);

    return 0;
}