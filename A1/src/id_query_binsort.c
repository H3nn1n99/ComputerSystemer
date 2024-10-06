#include <stdio.h>
#include <stdlib.h>
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

int compare(const void* a, const void* b) {
    int64_t id_a = ((struct index_record*)a)->osm_id;
    int64_t id_b = ((struct index_record*)b)->osm_id;
    
    if (id_a < id_b) return -1;
    if (id_a > id_b) return 1;
    return 0;
}

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

    qsort(data->irs, n, sizeof(struct index_record), compare);
    return data;
}

void free_indexed(struct indexed_data* data) {
    free(data->irs); 
    free(data);
}

const struct record* lookup_indexed(struct indexed_data *data, int64_t needle) {
    int low = 0;
    int high = data->n - 1;

    while (low <= high) {
        int mid = (low + high) / 2;
        int64_t mid_id = data->irs[mid].osm_id;

        if (mid_id < needle) {
            low = mid + 1;
        } else if (mid_id > needle) {
            high = mid - 1;
        } else {
            return data->irs[mid].record; 
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
