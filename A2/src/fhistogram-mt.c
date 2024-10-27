// Setting _DEFAULT_SOURCE is necessary to activate visibility of
// certain header file contents on GNU/Linux systems.
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fts.h>

#include "job_queue.h"

pthread_mutex_t stdout_mutex = PTHREAD_MUTEX_INITIALIZER;
// err.h contains various nonstandard BSD extensions, but they are
// very handy.
#include <err.h>
#include "histogram.h"

// Global histogram and mutex for synchronization
int global_histogram[8] = {0};
pthread_mutex_t histogram_mutex = PTHREAD_MUTEX_INITIALIZER;

void *worker_function(void *arg) {
    struct job_queue *q = (struct job_queue *)arg;
    char *file_path;

    // Local histogram for each thread
    int local_histogram[8] = {0};

    // process files from the job queue
    while (job_queue_pop(q, (void **)&file_path) == 0) {
        FILE *file = fopen(file_path, "r");
        if (file) {
            unsigned char byte;
            while (fread(&byte, 1, 1, file) > 0) {
                update_histogram(local_histogram, byte);
            }
            fclose(file);
        }
        free(file_path);  // Free the path memory

        // histogram update
        pthread_mutex_lock(&histogram_mutex);
        merge_histogram(global_histogram, local_histogram);
        print_histogram(global_histogram);  // Print the histogram
        pthread_mutex_unlock(&histogram_mutex);
    }
    return NULL;
}

int main(int argc, char * const *argv) {
  if (argc < 2) {
    err(1, "usage: paths...");
    exit(1);
  }

  int num_threads = 1;
  char * const *paths = &argv[1];

  if (argc > 3 && strcmp(argv[1], "-n") == 0) {
    // Since atoi() simply returns zero on syntax errors, we cannot
    // distinguish between the user entering a zero, or some
    // non-numeric garbage.  In fact, we cannot even tell whether the
    // given option is suffixed by garbage, i.e. '123foo' returns
    // '123'.  A more robust solution would use strtol(), but its
    // interface is more complicated, so here we are.
    num_threads = atoi(argv[2]);

    if (num_threads < 1) {
      err(1, "invalid thread count: %s", argv[2]);
    }

    paths = &argv[3];
  } else {
    paths = &argv[1];
  }

  struct job_queue q;
  job_queue_init(&q, 64);

  pthread_t threads[num_threads];
  for (int i = 0; i < num_threads; i++) {
    pthread_create(&threads[i], NULL, worker_function, &q);
  }

  // FTS_LOGICAL = follow symbolic links
  // FTS_NOCHDIR = do not change the working directory of the process
  //
  // (These are not particularly important distinctions for our simple
  // uses.)
  int fts_options = FTS_LOGICAL | FTS_NOCHDIR;

  FTS *ftsp;
  if ((ftsp = fts_open(paths, fts_options, NULL)) == NULL) {
    err(1, "fts_open() failed");
    return -1;
  }

  FTSENT *p;
  while ((p = fts_read(ftsp)) != NULL) {
    switch (p->fts_info) {
    case FTS_D:
      break;
    case FTS_F:
      job_queue_push(&q, strdup(p->fts_path));
      break;
    default:
      break;
    }
  }

  fts_close(ftsp);

  // Shut down the job queue
  job_queue_destroy(&q);

  //all worker threads finish
  for (int i = 0; i < num_threads; i++) {
    pthread_join(threads[i], NULL);  // Join threads
  }

  move_lines(9);

  return 0;
}
