#include <stddef.h>
#include <stdbool.h>

#define MAX_N 20
#define MAX_CYCLE_VERTS (2 * MAX_N + 4)
#define MAX_STACKS 8

typedef struct
{
    int u, v;
} Edge;

// Helper functions
void swap(int *a, int *b);

void compute_inverse(const int pi[], int pi_inv[], int n);

void print_array(int arr[], int n);

void save_D_to_file(const char *filename, int *D, long long size);

void d_array_path(char *buf, size_t bufsize, int n);
int read_D_from_file(const char *filename, long long r);
int *load_D_from_file(const char *filename, long long size);
int actual_distance_from_identity(const int *pi, int n);

void initialize_identity_permutation(int *pid, int n);

void transposition(const int *src, int *dst, int n, int i, int j, int k);

int *ComputeTDistanceFromIdentity(int n);

int rank1(int n, int pi[], int pi_inv[]);

int rank_safe(int n, const int src[], int *inv_buf);

// Original recursive unrank1: Builds a permutation from a given rank
void unrank1(int n, int r, int pi[]);

// Breakpoint graph (black/gray edges, cycles)
void extend_permutation(const int *src, int *dst, int n);
void build_edges(const int *ext, int n, Edge *black, Edge *gray);
void print_edges(const char *label, const Edge *edges, int count);
int find_cycles(const Edge *black, const Edge *gray, int n, int *cycle_len,
                 int cycle_vertices[][MAX_CYCLE_VERTS], int *cycle_vertex_count);
void print_cycles(const int *cycle_len, int cycle_vertices[][MAX_CYCLE_VERTS],
                   const int *cycle_vertex_count, int n_cycles);
int count_odd_cycles(const int *cycle_len, int n_cycles);
int lower_bound(int n, int c_odd);
int lower_bound_from_permutation(const int *pi, int n);
int find_max_gap_permutation(int n);

// Stacks in series (Input -> S1 -> ... -> S{num_stacks} -> Output)
bool sort_with_stacks(const int *pi, int n, int num_stacks, bool verbose);

// West-style repeated single-pass greedy stack-sort heuristic (fast, incomplete)
bool west_k_stack_sortable(const int *pi, int n, int k, bool verbose);

// Patience-sorting-style pyramid-pile heuristic (fast, tests LIS <= num_stacks)
bool pyramid_sortable(const int *pi, int n, int num_stacks, bool verbose);

// Exhaustive brute force: sortable by `passes` sequential free-choice single-stack passes?
bool free_pass_stack_sortable(const int *pi, int n, int passes);
