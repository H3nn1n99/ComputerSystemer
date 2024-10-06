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

// Struct for a k-d tree node
struct kd_node {
    struct record* point;  // Store the point (latitude, longitude)
    int axis;              // The axis this node splits on (0 = longitude, 1 = latitude)
    struct kd_node* left;  // Left subtree
    struct kd_node* right; // Right subtree
};

// Helper function to compare points based on axis (used for sorting)
int compare_points(const void* a, const void* b) {
    const struct record* p1 = *(const struct record**)a;
    const struct record* p2 = *(const struct record**)b;
    // Compare based on the selected axis (0 = longitude, 1 = latitude)
    return (p1->lon > p2->lon) - (p1->lon < p2->lon); // Change as needed for the axis
}

// Helper function to calculate the squared Euclidean distance between two points
double distance_squared(struct record* p1, double lon, double lat) {
    return (p1->lon - lon) * (p1->lon - lon) + (p1->lat - lat) * (p1->lat - lat);
}

// Recursively build the k-d tree
struct kd_node* build_kdtree(struct record** points, int n, int depth) {
    if (n <= 0) {
        return NULL;
    }

    int axis = depth % 2; // 0 = longitude, 1 = latitude

    // Sort points by the current axis (longitude or latitude)
    qsort(points, n, sizeof(struct record*), compare_points); // Using qsort instead of qsort_r

    // Select the median point (middle element after sorting)
    int median_index = n / 2;
    struct kd_node* node = malloc(sizeof(struct kd_node));
    node->point = points[median_index];
    node->axis = axis;

    // Recursively construct left and right subtrees
    node->left = build_kdtree(points, median_index, depth + 1);
    node->right = build_kdtree(points + median_index + 1, n - median_index - 1, depth + 1);

    return node;
}

// The mk_kdtree function, which takes the records and builds the k-d tree
struct kd_node* mk_kdtree(struct record* rs, int n) {
    printf("Creating k-d tree with %d records.\n", n);

    // Allocate an array of pointers to the records
    struct record** point_ptrs = malloc(n * sizeof(struct record*));
    for (int i = 0; i < n; i++) {
        point_ptrs[i] = &rs[i];
    }

    // Build the tree
    struct kd_node* root = build_kdtree(point_ptrs, n, 0);

    // Free the temporary array of pointers
    free(point_ptrs);
    
    return root;
}

// Recursive function to search for the nearest neighbor in the k-d tree
void lookup_kdtree_recursive(struct kd_node* node, struct record** closest, double lon, double lat, double* closest_dist_squared) {
    if (node == NULL) {
        return;
    }

    // Compute the distance between the current node's point and the query point
    double dist_squared = distance_squared(node->point, lon, lat);

    // If the current node is closer, update the closest point
    if (dist_squared < *closest_dist_squared) {
        *closest = node->point;
        *closest_dist_squared = dist_squared;
    }

    // Determine which subtree to search first
    int axis = node->axis;
    double diff = (axis == 0) ? lon - node->point->lon : lat - node->point->lat;

    // Recursively search the closer subtree
    if (diff < 0) {
        lookup_kdtree_recursive(node->left, closest, lon, lat, closest_dist_squared);
    } else {
        lookup_kdtree_recursive(node->right, closest, lon, lat, closest_dist_squared);
    }

    // If the hypersphere intersects the splitting plane, search the other subtree
    if (*closest_dist_squared > diff * diff) {
        if (diff < 0) {
            lookup_kdtree_recursive(node->right, closest, lon, lat, closest_dist_squared);
        } else {
            lookup_kdtree_recursive(node->left, closest, lon, lat, closest_dist_squared);
        }
    }
}

// The lookup_kdtree function, which finds the nearest neighbor in the k-d tree
const struct record* lookup_kdtree(struct kd_node* root, double lon, double lat) {
    const struct record* closest = NULL;
    double closest_dist_squared = INFINITY;
    lookup_kdtree_recursive(root, (struct record**)&closest, lon, lat, &closest_dist_squared);
    return closest;
}

// Recursively free the k-d tree
void free_kdtree(struct kd_node* node) {
    if (node == NULL) {
        return;
    }
    free_kdtree(node->left);
    free_kdtree(node->right);
    free(node);
}

// Main function
int main(int argc, char** argv) {
    return coord_query_loop(argc, argv,
                            (mk_index_fn)mk_kdtree,
                            (free_index_fn)free_kdtree,
                            (lookup_fn)lookup_kdtree);
}
