#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>

#include "record.h"
#include "id_query.h"

struct index_record {
    int64_t osm_id;
    const struct record *record;
};

struct indexed_data {
    struct index_record *irs;
    int n;
};

struct indexed_data* mk_indexed(struct record* rs, int n) {
struct indexed_data* data = malloc(sizeof(struct indexed_data));
if (!data) {
      fprintf(stderr, "Memory allocation failed\n");
      exit(1);
  }

data->irs = malloc(n * sizeof(struct index_record));
    if (!data->irs) {
        fprintf(stderr, "Memory allocation failed\n");
        free(data);
        exit(1);
    }

    data->n = n;
    for (int i = 0; i < n; i++) {
        data->irs[i].osm_id = rs[i].osm_id;  
        data->irs[i].record = &rs[i];
    }
    
    return data;
}

void free_indexed(struct indexed_data* data) {
    free(data->irs); 
    free(data);
}

const struct record* lookup_indexed(struct indexed_data *data,
int64_t desiredID) {
    for (int i = 0; i < data->n; i++) {
        if (data->irs[i].osm_id == desiredID) {
            return data->irs[i].record; 
        }
    }
    return NULL;
}

int main(int argc, char** argv) {
    return id_query_loop(argc, argv,
                        (mk_index_fn)mk_indexed,
                        (free_index_fn)free_indexed,
                        (lookup_fn)lookup_indexed);
}
