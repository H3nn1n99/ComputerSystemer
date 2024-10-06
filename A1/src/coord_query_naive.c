#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>
#include <math.h>

#include "record.h"
#include "coord_query.h"

struct naive_data {
  struct record *rs;
  int n;
};


double distance(double x1, double y1, double x2, double y2) {
    return sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
}

struct naive_data* mk_naive(struct record* rs, int n) {
  struct naive_data* data = malloc(sizeof(struct naive_data));
  if (!data) {
      fprintf(stderr, "Memory allocation failed\n");
      exit(1);
  }

  data->rs = rs; 
  data->n = n;   

  return data;
}

void free_naive(struct naive_data* data) {
  free(data); 
}

const struct record* lookup_naive(struct naive_data *data, double lon, double lat) {
  const struct record* closest_record = NULL;
  double min_distance = -1;
  for (int i = 0; i < data->n; i++) {
    double dist = distance(data->rs[i].lon, data->rs[i].lat, lon, lat); 
    if (min_distance == -1 || dist < min_distance) {
        min_distance = dist;
        closest_record = &data->rs[i];
    }
  }
  return closest_record;  
}

int main(int argc, char** argv) {
  return coord_query_loop(argc, argv,
                          (mk_index_fn)mk_naive,
                          (free_index_fn)free_naive,
                          (lookup_fn)lookup_naive);
}
