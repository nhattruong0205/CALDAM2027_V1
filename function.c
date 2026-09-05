#include "function.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Helper functions
void swap(int *a, int *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

void compute_inverse(const int pi[], int pi_inv[], int n)
{
    for (int i = 0; i < n; i++)
    {
        pi_inv[pi[i]] = i;
    }
}

void print_array(int arr[], int n)
{
    printf("[");
    for (int i = 0; i < n; i++)
    {
        if (i > 0)
        {
            printf(" ");
        }
        printf("%d", arr[i] + 1);
    }
    printf("] \n");
}

// =============== Saving files function ======================
void save_D_to_file(const char *filename, int *D, long long size)
{
    FILE *fp = fopen(filename, "w");
    if (!fp)
    {
        printf("Error: cannot open %s for writing.\n", filename);
        return;
    }
    for (long long i = 0; i < size; i++)
    {
        fprintf(fp, "%d\n", D[i]);
    }
    fclose(fp);
    printf("Saved D array to %s (%lld elements)\n", filename, size);
}

// Path of the saved distance file for a given n, matching the filename
// ComputeTDistanceFromIdentity writes to.
void d_array_path(char *buf, size_t bufsize, int n)
{
    snprintf(buf, bufsize,
             "/Users/nhattruong/Documents/Summer 2026/CALDAM2027/dArray/distances_n%d.txt", n);
}

// Reads a single value D[r] from a previously saved distance file (one int
// per line, as written by save_D_to_file), without loading the whole array
// or recomputing the BFS.
int read_D_from_file(const char *filename, long long r)
{
    FILE *fp = fopen(filename, "r");
    if (!fp)
    {
        printf("Error: cannot open %s for reading.\n", filename);
        return -1;
    }

    int value = -1;
    for (long long i = 0; i <= r; i++)
        fscanf(fp, "%d", &value);

    fclose(fp);
    return value;
}

// Loads a full saved distance array (one int per line, as written by
// save_D_to_file) into memory in a single pass. Returns NULL if the file
// cannot be opened; caller owns the returned buffer (free it).
int *load_D_from_file(const char *filename, long long size)
{
    FILE *fp = fopen(filename, "r");
    if (!fp)
    {
        printf("Error: cannot open %s for reading.\n", filename);
        return NULL;
    }

    int *D = malloc(size * sizeof(int));
    for (long long i = 0; i < size; i++)
        fscanf(fp, "%d", &D[i]);

    fclose(fp);
    return D;
}

// Looks up the actual transposition distance of a 0-indexed permutation pi
// (size n) from identity, by ranking pi and reading D[rank] out of the
// saved dArray/distances_n{n}.txt file (must already exist).
int actual_distance_from_identity(const int *pi, int n)
{
    int pi_inv[MAX_N];
    compute_inverse(pi, pi_inv, n);
    int r = rank_safe(n, pi, pi_inv);

    char filename[512];
    d_array_path(filename, sizeof(filename), n);
    return read_D_from_file(filename, r);
}

void initialize_identity_permutation(int *pid, int n)
{
    for (int i = 0; i < n; i++)
    {
        pid[i] = i;
    }
}

void transposition(const int *src, int *dst, int n, int i, int j, int k)
{
    int idx = 0;

    // 1. Prefix: [0..i-1]
    for (int x = 0; x < i; ++x)
        dst[idx++] = src[x];

    // 2. Block: [j..k]
    for (int x = j; x <= k; ++x)
        dst[idx++] = src[x];

    // 3. Middle: [i..j-1]
    for (int x = i; x < j; ++x)
        dst[idx++] = src[x];

    // 4. Suffix: [k+1..n-1]
    for (int x = k + 1; x < n; ++x)
        dst[idx++] = src[x];
}

// =============== Breakpoint graph (black/gray edges, cycles) ===============

// Extends a 0-indexed permutation of size n into the "hat" permutation of
// size n+2 by prepending 0 and appending n+1, using 1-indexed values for
// the original entries (e.g. internal [1 3 2 4 0] -> [0 2 4 3 5 1 6]).
void extend_permutation(const int *src, int *dst, int n)
{
    dst[0] = 0;
    for (int i = 0; i < n; i++)
        dst[i + 1] = src[i] + 1;
    dst[n + 1] = n + 1;
}

// Builds the black edges (ext[i], ext[i-1]) and the gray edges (i-1, i) of
// the breakpoint graph for the extended permutation ext (length n+2).
// Both black and gray have n+1 edges.
void build_edges(const int *ext, int n, Edge *black, Edge *gray)
{
    for (int i = 1; i <= n + 1; i++)
    {
        black[i - 1].u = ext[i];
        black[i - 1].v = ext[i - 1];
        gray[i - 1].u = i - 1;
        gray[i - 1].v = i;
    }
}

void print_edges(const char *label, const Edge *edges, int count)
{
    printf("%s: ", label);
    for (int i = 0; i < count; i++)
        printf("(%d %d) ", edges[i].u, edges[i].v);
    printf("\n");
}

// Decomposes the black/gray graph into alternating cycles. Edges are
// directed (gray: i-1 -> i; black: pi_i -> pi_{i-1}), so every vertex has
// exactly one outgoing edge per color and the decomposition is unique
// (equivalent to the cycles of the permutation black_next(gray_next(.))).
// Returns the number of cycles found and fills cycle_len[c] with the number
// of black edges (== gray edges) in cycle c, cycle_vertices[c] with the
// sequence of vertices visited (starting vertex repeated at the end), and
// cycle_vertex_count[c] with how many entries that is. cycle_len and
// cycle_vertex_count must hold at least n+1 ints.
int find_cycles(const Edge *black, const Edge *gray, int n, int *cycle_len,
                int cycle_vertices[][MAX_CYCLE_VERTS], int *cycle_vertex_count)
{
    int m = n + 1;
    bool gray_used[MAX_N + 2] = {false};
    int black_out[MAX_N + 2], gray_out[MAX_N + 2];

    for (int e = 0; e < m; e++)
    {
        black_out[black[e].u] = e;
        gray_out[gray[e].u] = e;
    }

    int n_cycles = 0;
    for (int start_e = 0; start_e < m; start_e++)
    {
        if (gray_used[start_e])
            continue;

        int start_v = gray[start_e].u;
        int cur_v = start_v;
        int len = 0;
        int need_black = 0;
        int vcount = 0;
        cycle_vertices[n_cycles][vcount++] = cur_v;

        do
        {
            if (need_black)
            {
                int e = black_out[cur_v];
                cur_v = black[e].v;
                len++;
            }
            else
            {
                int e = gray_out[cur_v];
                gray_used[e] = true;
                cur_v = gray[e].v;
            }
            cycle_vertices[n_cycles][vcount++] = cur_v;
            need_black = !need_black;
        } while (cur_v != start_v);

        cycle_len[n_cycles] = len;
        cycle_vertex_count[n_cycles] = vcount;
        n_cycles++;
    }

    return n_cycles;
}

void print_cycles(const int *cycle_len, int cycle_vertices[][MAX_CYCLE_VERTS],
                  const int *cycle_vertex_count, int n_cycles)
{
    for (int c = 0; c < n_cycles; c++)
    {
        printf("Cycle %d (length %d): ", c, cycle_len[c]);
        for (int i = 0; i < cycle_vertex_count[c]; i++)
        {
            if (i > 0)
                printf(" %s ", (i % 2 == 1) ? "-g->" : "-b->");
            printf("%d", cycle_vertices[c][i]);
        }
        printf("\n");
    }
    printf("Odd cycles: %d / %d\n", count_odd_cycles(cycle_len, n_cycles), n_cycles);
}

// Number of odd-length cycles (length = number of black edges in the
// cycle) among cycle_len[0..n_cycles-1].
int count_odd_cycles(const int *cycle_len, int n_cycles)
{
    int odd = 0;
    for (int c = 0; c < n_cycles; c++)
        if (cycle_len[c] % 2 == 1)
            odd++;
    return odd;
}

// Lower bound on the transposition distance: floor(((n+1) - c_odd) / 2).
int lower_bound(int n, int c_odd)
{
    return (int)floor((n + 1 - c_odd) / 2.0);
}

// Runs the full pipeline (extend -> breakpoint graph -> cycles -> odd count
// -> lower_bound) directly from a 0-indexed permutation pi of size n.
int lower_bound_from_permutation(const int *pi, int n)
{
    int ext[MAX_N + 2];
    extend_permutation(pi, ext, n);

    Edge black[MAX_N + 1], gray[MAX_N + 1];
    build_edges(ext, n, black, gray);

    int cycle_len[MAX_N + 1];
    int cycle_vertices[MAX_N + 1][MAX_CYCLE_VERTS];
    int cycle_vertex_count[MAX_N + 1];
    int n_cycles = find_cycles(black, gray, n, cycle_len, cycle_vertices, cycle_vertex_count);

    return lower_bound(n, count_odd_cycles(cycle_len, n_cycles));
}

// Searches all n! permutations of size n for the largest gap between the
// actual transposition distance (from BFS) and the breakpoint-graph lower
// bound, prints every permutation achieving that gap, and returns the gap.
int find_max_gap_permutation(int n)
{
    char filename[512];
    d_array_path(filename, sizeof(filename), n);

    long long fact = 1;
    for (int x = 2; x <= n; x++)
        fact *= x;

    int *D = load_D_from_file(filename, fact);
    if (!D)
        return -1;

    int pi[MAX_N];
    int best_gap = -1;

    for (long long r = 0; r < fact; r++)
    {
        initialize_identity_permutation(pi, n);
        unrank1(n, (int)r, pi);
        int gap = D[r] - lower_bound_from_permutation(pi, n);
        if (gap > best_gap)
            best_gap = gap;
    }

    int count = 0;
    for (long long r = 0; r < fact; r++)
    {
        initialize_identity_permutation(pi, n);
        unrank1(n, (int)r, pi);
        if (D[r] - lower_bound_from_permutation(pi, n) == best_gap)
        {
            count++;
            printf("  ");
            print_array(pi, n);
        }
    }
    printf("Max gap: %d, achieved by %d permutation(s)\n", best_gap, count);

    free(D);
    return best_gap;
}

// =============== Sorting with stacks in series ===============
// Models `num_stacks` plain LIFO stacks chained Input -> S1 -> S2 -> ... ->
// Sk -> Output: any value may be pushed onto any stack (no ordering
// constraint). Every column -- S1, S2, S3, ... and Output -- is printed
// left-is-top: the most recently arrived value is shown left-most. Because
// of that, a value can only legally be popped to Output when it is the
// largest value not yet output: each new arrival is the left-most (newest)
// entry, so arrivals have to count down (n, n-1, ..., 1) for Output to read
// as the ascending sorted sequence once printed left to right (anything
// popped out of that order can never be fixed later, since Output only ever
// gains new left-most entries). Which move to take when several are legal
// actually matters -- e.g. shifting a stack's top onward as soon as possible
// can bury a value under another with no way to dig it back out -- so this
// runs a backtracking search over the legal moves (preferring the
// right-most move first, since that is usually the winning choice and keeps
// the search fast) rather than a single fixed-priority pass.

typedef struct
{
    int items[MAX_N];
    int top; // index of top element, -1 if empty
} SeriesStack;

static void sstack_init(SeriesStack *s) { s->top = -1; }
static bool sstack_empty(const SeriesStack *s) { return s->top < 0; }
static int sstack_top(const SeriesStack *s) { return s->items[s->top]; }
static void sstack_push(SeriesStack *s, int v) { s->items[++s->top] = v; }
static int sstack_pop(SeriesStack *s) { return s->items[s->top--]; }

// Formats vals[0..len-1] as "v0+1 v1+1 ..." (1-indexed, matching print_array,
// left-to-right in storage order), or "-" when len is 0. Used for Input,
// where the front of the remaining queue is naturally left-most already.
static void format_values(char *buf, size_t bufsize, const int *vals, int len)
{
    if (len == 0)
    {
        snprintf(buf, bufsize, "-");
        return;
    }
    size_t pos = 0;
    for (int i = 0; i < len; i++)
    {
        int written = snprintf(buf + pos, bufsize - pos, i == 0 ? "%d" : " %d", vals[i] + 1);
        if (written < 0 || (size_t)written >= bufsize - pos)
            break;
        pos += (size_t)written;
    }
}

// Formats vals[0..len-1] left-to-right in reverse storage order (index
// len-1 first), or "-" when len is 0. Used for S1..Sk and Output, where
// index len-1 is the most recently arrived value and left-is-top.
static void format_values_reversed(char *buf, size_t bufsize, const int *vals, int len)
{
    if (len == 0)
    {
        snprintf(buf, bufsize, "-");
        return;
    }
    size_t pos = 0;
    for (int i = len - 1; i >= 0; i--)
    {
        int written = snprintf(buf + pos, bufsize - pos, i == len - 1 ? "%d" : " %d", vals[i] + 1);
        if (written < 0 || (size_t)written >= bufsize - pos)
            break;
        pos += (size_t)written;
    }
}

static void print_stack_row(const int *pi, int n, int in_pos, SeriesStack *stacks,
                             int num_stacks, const int *output, int out_len)
{
    char buf[128];

    format_values(buf, sizeof(buf), pi + in_pos, n - in_pos);
    printf("%-10s| ", buf);

    for (int i = 0; i < num_stacks; i++)
    {
        format_values_reversed(buf, sizeof(buf), stacks[i].items, stacks[i].top + 1);
        printf("%-6s| ", buf);
    }

    format_values_reversed(buf, sizeof(buf), output, out_len);
    printf("%s\n", buf);
}

#define MAX_MOVES (MAX_N * (MAX_STACKS + 1))

// Move codes recorded in StackSortSearch.path:
//   0 .. num_stacks-2  -> shift stacks[code] onto stacks[code+1]
//   num_stacks - 1     -> pop the last stack to output
//   num_stacks         -> push the next input value onto stacks[0]
typedef struct
{
    SeriesStack stacks[MAX_STACKS];
    int num_stacks;
    const int *pi;
    int n;
    int in_pos;
    int next_needed;
    int out_len;
    int path[MAX_MOVES];
    int path_len;
} StackSortSearch;

// Backtracking search: tries every legal move from the current state
// (right-most move first), recursing and undoing on failure. Records the
// winning sequence of moves in st->path. Returns true iff the output can be
// fully sorted from this state.
static bool dfs_sort(StackSortSearch *st)
{
    if (st->out_len == st->n)
        return true;

    int k = st->num_stacks;
    SeriesStack *last = &st->stacks[k - 1];

    if (!sstack_empty(last) && sstack_top(last) == st->next_needed)
    {
        int v = sstack_pop(last);
        st->next_needed--;
        st->out_len++;
        st->path[st->path_len++] = k - 1;

        if (dfs_sort(st))
            return true;

        st->path_len--;
        st->out_len--;
        st->next_needed++;
        sstack_push(last, v);
    }

    for (int i = k - 2; i >= 0; i--)
    {
        SeriesStack *src = &st->stacks[i];
        SeriesStack *dst = &st->stacks[i + 1];
        if (!sstack_empty(src))
        {
            int v = sstack_pop(src);
            sstack_push(dst, v);
            st->path[st->path_len++] = i;

            if (dfs_sort(st))
                return true;

            st->path_len--;
            sstack_pop(dst);
            sstack_push(src, v);
        }
    }

    if (st->in_pos < st->n)
    {
        SeriesStack *first = &st->stacks[0];
        sstack_push(first, st->pi[st->in_pos]);
        st->in_pos++;
        st->path[st->path_len++] = k;

        if (dfs_sort(st))
            return true;

        st->path_len--;
        st->in_pos--;
        sstack_pop(first);
    }

    return false;
}

// Sorts pi (0-indexed permutation of size n) using `num_stacks` stacks
// chained in series (Input -> S1 -> ... -> S{num_stacks} -> Output), via
// backtracking search over the legal moves. Returns true iff some legal
// sequence sorts pi; if verbose, replays and prints the winning sequence as
// a step-by-step trace table (or reports that no sequence exists).
bool sort_with_stacks(const int *pi, int n, int num_stacks, bool verbose)
{
    StackSortSearch st;
    for (int i = 0; i < num_stacks; i++)
        sstack_init(&st.stacks[i]);
    st.num_stacks = num_stacks;
    st.pi = pi;
    st.n = n;
    st.in_pos = 0;
    st.next_needed = n - 1; // arrivals at Output must count down n-1, n-2, ..., 0
    st.out_len = 0;
    st.path_len = 0;

    if (!dfs_sort(&st))
    {
        if (verbose)
            printf("Not sortable with %d stack(s) in series: no legal move "
                   "sequence sorts this permutation.\n",
                   num_stacks);
        return false;
    }

    if (verbose)
    {
        SeriesStack stacks[MAX_STACKS];
        for (int i = 0; i < num_stacks; i++)
            sstack_init(&stacks[i]);
        int output[MAX_N];
        int out_len = 0;
        int in_pos = 0;

        printf("%-10s| ", "Input");
        for (int i = 0; i < num_stacks; i++)
        {
            char label[8];
            snprintf(label, sizeof(label), "S%d", i + 1);
            printf("%-6s| ", label);
        }
        printf("Output\n");
        print_stack_row(pi, n, in_pos, stacks, num_stacks, output, out_len);

        for (int m = 0; m < st.path_len; m++)
        {
            int code = st.path[m];
            if (code == num_stacks)
                sstack_push(&stacks[0], pi[in_pos++]);
            else if (code == num_stacks - 1)
                output[out_len++] = sstack_pop(&stacks[num_stacks - 1]);
            else
                sstack_push(&stacks[code + 1], sstack_pop(&stacks[code]));

            print_stack_row(pi, n, in_pos, stacks, num_stacks, output, out_len);
        }
    }

    return true;
}

// =============== West-style stack-sortable heuristic ===============
// The classical, deterministic single-pass greedy stack-sort (Knuth/West):
// scan the input left to right, popping the stack to output whenever its top
// is less than the next input value, otherwise pushing the next input value.
// One pass sorts exactly the permutations avoiding the pattern 231. West
// defined a permutation as "2-stack-sortable" when applying this same pass
// twice in a row yields the identity. This generalizes that to k passes.
static void west_stack_sort_pass(const int *in, int *out, int n)
{
    int stack[MAX_N];
    int top = -1;
    int out_len = 0;

    for (int i = 0; i < n; i++)
    {
        int x = in[i];
        while (top >= 0 && stack[top] < x)
            out[out_len++] = stack[top--];
        stack[++top] = x;
    }
    while (top >= 0)
        out[out_len++] = stack[top--];
}

// Heuristic (not exhaustive) test generalizing West's "2-stack-sortable"
// definition to k passes: applies the single-pass greedy stack-sort above k
// times in a row and checks whether the result is the identity. Pass k=3 for
// the "West-3" heuristic. This runs in O(n*k) time -- far faster than
// sort_with_stacks' backtracking search -- but it is a strictly weaker,
// incomplete test: it repeats k *separate* full left-to-right passes with no
// backtracking, rather than exploring the live k-stacks-in-series pipeline
// sort_with_stacks searches. A "true" here means pi is sortable (proof by
// construction); a "false" only means this specific greedy heuristic didn't
// find a way -- it does not prove pi is unsortable by k stacks in series.
bool west_k_stack_sortable(const int *pi, int n, int k, bool verbose)
{
    int cur[MAX_N], next[MAX_N];
    memcpy(cur, pi, n * sizeof(int));

    if (verbose)
    {
        printf("start:        ");
        print_array(cur, n);
    }

    for (int pass = 0; pass < k; pass++)
    {
        west_stack_sort_pass(cur, next, n);
        memcpy(cur, next, n * sizeof(int));

        if (verbose)
        {
            printf("after pass %d: ", pass + 1);
            print_array(cur, n);
        }
    }

    for (int i = 0; i < n; i++)
        if (cur[i] != i)
            return false;
    return true;
}

// =============== Pyramid-pile heuristic ===============
// A third, distinct fast heuristic (unrelated to West's repeated-pass idea).
// Maintains `num_stacks` piles ("pyramids"), each required to stay strictly
// decreasing from top to bottom: a value may only be pushed onto a pile if
// it is smaller than the pile's current top (or the pile is empty), so
// reading a pile bottom to top the values shrink towards the top -- a
// pyramid shape.
//
// Phase 1 (placing): scanning the permutation left to right, each value is
// placed on the tightest-fitting eligible pile: among the piles whose top it
// is smaller than, the one with the smallest such top (an empty pile is only
// used when no non-empty pile can take the value, since starting a fresh
// pile is the least "tight" option). If no pile -- empty or otherwise -- can
// legally take the next value, the scan stops there and this returns false.
//
// This placement rule is the classical patience-sorting rule, and it is
// provably optimal for a specific, narrower question: the minimum number of
// strictly-decreasing piles needed to hold an entire sequence equals the
// length of its longest increasing subsequence (LIS). So with num_stacks
// piles, phase 1 places every value (never gets stuck) if and only if
// LIS(pi) <= num_stacks.
//
// Phase 2 (merging): once every value has been placed, repeatedly pop
// whichever pile currently has the smallest top and append it to the
// output. Each pile, popped from the top down, yields an increasing run (the
// pile was built decreasing bottom-to-top, so the values only grow as you
// dig past the top), so this is exactly a k-way merge of num_stacks
// increasing runs -- which always reconstructs the full sorted sequence.
// Phase 2 therefore can never fail on its own; it only runs once phase 1 has
// already succeeded.
//
// Note this answers a different question from "is pi sortable by num_stacks
// stacks in series" (sort_with_stacks) or the West-k heuristic
// (west_k_stack_sortable) -- passing this test does not by itself prove
// either of those.
bool pyramid_sortable(const int *pi, int n, int num_stacks, bool verbose)
{
    SeriesStack piles[MAX_STACKS];
    for (int i = 0; i < num_stacks; i++)
        sstack_init(&piles[i]);

    if (verbose)
    {
        printf("%-10s| ", "Input");
        for (int i = 0; i < num_stacks; i++)
        {
            char label[8];
            snprintf(label, sizeof(label), "P%d", i + 1);
            printf("%-6s| ", label);
        }
        printf("Output\n");
    }

    // Phase 1: place every input value onto the tightest-fitting pile.
    for (int idx = 0; idx < n; idx++)
    {
        int x = pi[idx];

        int best = -1;
        for (int i = 0; i < num_stacks; i++)
        {
            bool eligible = sstack_empty(&piles[i]) || sstack_top(&piles[i]) > x;
            if (!eligible)
                continue;

            if (best == -1)
            {
                best = i;
                continue;
            }

            bool best_empty = sstack_empty(&piles[best]);
            bool cur_empty = sstack_empty(&piles[i]);

            if (best_empty && !cur_empty)
                best = i; // a real top is always a tighter fit than a fresh pile
            else if (!best_empty && !cur_empty && sstack_top(&piles[i]) < sstack_top(&piles[best]))
                best = i; // tighter fit among two real tops
        }

        if (best == -1)
        {
            if (verbose)
                printf("Stuck: %d fits under none of the %d pyramids after placing %d of %d values.\n",
                       x + 1, num_stacks, idx, n);
            return false;
        }

        sstack_push(&piles[best], x);

        if (verbose)
        {
            char buf[128];
            format_values(buf, sizeof(buf), pi + idx + 1, n - idx - 1);
            printf("%-10s| ", buf);
            for (int i = 0; i < num_stacks; i++)
            {
                format_values_reversed(buf, sizeof(buf), piles[i].items, piles[i].top + 1);
                printf("%-6s| ", buf);
            }
            printf("-      (placed %d on pyramid %d)\n", x + 1, best + 1);
        }
    }

    // Phase 2: input is exhausted -- repeatedly pop the smallest available
    // top across all piles until every pile is empty (k-way merge).
    int output[MAX_N];
    int out_len = 0;

    while (out_len < n)
    {
        int best = -1;
        for (int i = 0; i < num_stacks; i++)
        {
            if (sstack_empty(&piles[i]))
                continue;
            if (best == -1 || sstack_top(&piles[i]) < sstack_top(&piles[best]))
                best = i;
        }

        int v = sstack_pop(&piles[best]);
        output[out_len++] = v;

        if (verbose)
        {
            char buf[128];
            printf("%-10s| ", "-");
            for (int i = 0; i < num_stacks; i++)
            {
                format_values_reversed(buf, sizeof(buf), piles[i].items, piles[i].top + 1);
                printf("%-6s| ", buf);
            }
            format_values(buf, sizeof(buf), output, out_len);
            printf("%-6s (popped %d from pyramid %d)\n", buf, v + 1, best + 1);
        }
    }

    if (verbose)
        printf("All %d values placed and merged back out in sorted order.\n", n);
    return true;
}

// =============== Free-choice multi-pass single-stack sortability ===============
// Brute-force decision procedure for a specific model: pi is fed through ONE
// stack, `passes` times in a row. Each pass allows full freedom -- at every
// step you may push the next remaining input value onto the stack, or pop
// the stack's top to that pass's output, in any order the LIFO discipline
// allows -- rather than West's fixed deterministic greedy rule. Passes never
// interleave: pass i+1 only begins once pass i's entire schedule has fully
// completed (its input exhausted and its stack emptied), unlike
// sort_with_stacks' freely-interleaved stacks-in-series pipeline. This
// exhaustively searches every schedule of every pass and returns true the
// instant any full sequence of schedules reaches the identity.
//
// For passes == 1 this reduces to plain single-stack sortability, which is
// exactly 231-avoidance (Catalan-counted) -- freedom of choice cannot beat
// the deterministic greedy for a single pass, since if any schedule sorts
// pi, so does always-pop-when-possible. But once a second pass is allowed to
// act on whichever intermediate result pass 1 happened to leave behind, the
// combined two-pass class can be strictly larger than West's forced-greedy
// S(S(pi)) test (west_k_stack_sortable with k=2), because free choice in
// pass 1 can aim for a different (still valid) intermediate result that
// happens to be easier for pass 2 to finish off.
// Takes stack/mid by const pointer and copies into a fresh local array
// before mutating, rather than mutating a shared array in place. A shared,
// mutated-in-place array is not safe here: once some sub-exploration pops
// back past a given index and pushes something new into that same slot, any
// other, unrelated branch that later reuses the same index has no way to
// tell that the value sitting there belongs to an already-abandoned branch
// rather than the one it's actually exploring. Copying avoids that
// aliasing entirely; the arrays are only ever O(n) long, so the extra
// copying is negligible.
static bool dfs_free_pass(const int *input, int n, int in_pos, const int *stack, int top,
                           const int *mid, int mid_len, int passes_left)
{
    if (in_pos == n && top < 0)
    {
        if (passes_left == 1)
        {
            for (int i = 0; i < n; i++)
                if (mid[i] != i)
                    return false;
            return true;
        }

        int next_stack[MAX_N], next_mid[MAX_N];
        return dfs_free_pass(mid, n, 0, next_stack, -1, next_mid, 0, passes_left - 1);
    }

    if (in_pos < n)
    {
        int new_stack[MAX_N];
        for (int i = 0; i <= top; i++)
            new_stack[i] = stack[i];
        new_stack[top + 1] = input[in_pos];
        if (dfs_free_pass(input, n, in_pos + 1, new_stack, top + 1, mid, mid_len, passes_left))
            return true;
    }

    if (top >= 0)
    {
        int new_mid[MAX_N];
        for (int i = 0; i < mid_len; i++)
            new_mid[i] = mid[i];
        new_mid[mid_len] = stack[top];
        if (dfs_free_pass(input, n, in_pos, stack, top - 1, new_mid, mid_len + 1, passes_left))
            return true;
    }

    return false;
}

bool free_pass_stack_sortable(const int *pi, int n, int passes)
{
    int stack[MAX_N], mid[MAX_N];
    return dfs_free_pass(pi, n, 0, stack, -1, mid, 0, passes);
}

// Brute-force count: how many permutations of size n are sortable by 2 free
// passes through a single stack (free_pass_stack_sortable(pi, n, 2)) but NOT
// by West's own deterministic "2-stack-sortable" definition
// (west_k_stack_sortable(pi, n, 2, false), i.e. S(S(pi)) == identity)? This
// isolates exactly the permutations where pass-1 freedom -- choosing a
// different valid intermediate result than the forced greedy pass would
// produce -- is what makes two-pass sorting possible. Iterates every
// permutation of size n via rank1/unrank1, so it is exact, not sampled.
long long free_pass_not_west2_count(int n)
{
    long long fact = 1;
    for (int x = 2; x <= n; x++)
        fact *= x;

    int pi[MAX_N];
    long long count = 0;

    for (long long r = 0; r < fact; r++)
    {
        initialize_identity_permutation(pi, n);
        unrank1(n, (int)r, pi);
        if (free_pass_stack_sortable(pi, n, 2) && !west_k_stack_sortable(pi, n, 2, false))
            count++;
    }

    return count;
}

// Original recursive rank1 function: computes the lexicographic rank of a permutation
int rank1(int n, int pi[], int pi_inv[])
{
    if (n == 1)
        return 0;

    int s = pi[n - 1];

    swap(&pi[n - 1], &pi[pi_inv[n - 1]]);
    swap(&pi_inv[s], &pi_inv[n - 1]);

    return s + n * rank1(n - 1, pi, pi_inv);
}

int rank_safe(int n, const int src[], int *inv_buf)
{
    int tmp[MAX_N], tmp_inv[MAX_N];
    memcpy(tmp, src, n * sizeof(int));         // work on a copy
    memcpy(tmp_inv, inv_buf, n * sizeof(int)); // inv must start correct
    return rank1(n, tmp, tmp_inv);             // rank1 can now swap freely
}

// Original recursive unrank1: Builds a permutation from a given rank
void unrank1(int n, int r, int pi[])
{
    if (n > 0)
    {
        swap(&pi[n - 1], &pi[r % n]);
        unrank1(n - 1, r / n, pi);
    }
}

//================== Compute distance array ====================
// BFS from the identity permutation over single-block transpositions.
// D[r] is the minimum number of transpositions needed to reach the
// permutation with rank r (rank1/unrank1) from the identity; D has n! entries.
int *ComputeTDistanceFromIdentity(int n)
{
    long long fact = 1;
    for (int x = 2; x <= n; x++)
        fact *= x;

    int *D = malloc(fact * sizeof(int));
    bool *visited = calloc(fact, sizeof(bool));
    int *queue = malloc(fact * sizeof(int));
    long long q_head = 0, q_tail = 0;

    int *pi = malloc(n * sizeof(int));
    int *pi_inv = malloc(n * sizeof(int));
    initialize_identity_permutation(pi, n);
    compute_inverse(pi, pi_inv, n);

    int start = rank_safe(n, pi, pi_inv);
    D[start] = 0;
    visited[start] = true;
    queue[q_tail++] = start;

    int *result = malloc(n * sizeof(int));
    int *tmp = malloc(n * sizeof(int));
    int *tmp_inv = malloc(n * sizeof(int));

    while (q_head < q_tail)
    {
        int current_rank = queue[q_head++];

        initialize_identity_permutation(result, n);
        unrank1(n, current_rank, result);

        for (int i = 0; i < n; ++i)
            for (int j = i + 1; j < n; ++j)
                for (int k = j; k < n; ++k)
                {
                    transposition(result, tmp, n, i, j, k);
                    compute_inverse(tmp, tmp_inv, n);
                    int rank_tmp = rank_safe(n, tmp, tmp_inv);

                    if (!visited[rank_tmp])
                    {
                        visited[rank_tmp] = true;
                        D[rank_tmp] = D[current_rank] + 1;
                        queue[q_tail++] = rank_tmp;
                    }
                }
    }

    // Comment after
    char filename[512];
    snprintf(filename, sizeof(filename),
             "/Users/nhattruong/Documents/Summer 2026/CALDAM2027/dArray/distances_n%d.txt", n);
    save_D_to_file(filename, D, fact);

    return D;
}
