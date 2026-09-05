#include "function.h"
#include <stdio.h>
#include <time.h>

// Generates the lexicographically next permutation of arr[0..n-1] in place
// (standard next_permutation algorithm). Returns false once arr is already
// the last (fully descending) permutation, i.e. enumeration is complete.
static bool next_permutation(int *arr, int n)
{
    int i = n - 2;
    while (i >= 0 && arr[i] >= arr[i + 1])
        i--;
    if (i < 0)
        return false;

    int j = n - 1;
    while (arr[j] <= arr[i])
        j--;

    int tmp = arr[i];
    arr[i] = arr[j];
    arr[j] = tmp;

    for (int l = i + 1, r = n - 1; l < r; l++, r--)
    {
        tmp = arr[l];
        arr[l] = arr[r];
        arr[r] = tmp;
    }
    return true;
}

// Sweeps every permutation of size n=14 and checks it against the West-3
// heuristic (west_k_stack_sortable(pi, n, 3)). This is an exhaustive count,
// not a full exact classification: the heuristic can miss permutations that
// a smarter (backtracking) 3-stacks-in-series strategy could still sort, so
// "not sortable" here means "the West-3 heuristic didn't find a way," not a
// proof of true unsortability. Chosen deliberately over the exact
// backtracking search because n=14 has 14! ~= 87.2 billion permutations,
// which is intractable for the exact search but feasible (~5-6 hours) at the
// heuristic's ~200ns/permutation.
// int main()
// {
//     int n = 14;
//     int pi[MAX_N];
//     initialize_identity_permutation(pi, n);

//     long long fact = 1;
//     for (int x = 2; x <= n; x++)
//         fact *= x;

//     const char *out_path = "/Users/nhattruong/Documents/Summer 2026/CALDAM2027/dArray/west3_not_sortable_n14.txt";
//     FILE *out = fopen(out_path, "w");
//     if (!out)
//     {
//         printf("Error: cannot open %s for writing.\n", out_path);
//         return 1;
//     }

//     const long long sample_cap = 1000000;
//     const long long progress_every = 100000000LL;

//     long long checked = 0, not_sortable = 0;
//     time_t start = time(NULL);

//     do
//     {
//         checked++;
//         if (!west_k_stack_sortable(pi, n, 3, false))
//         {
//             not_sortable++;
//             if (not_sortable <= sample_cap)
//             {
//                 for (int i = 0; i < n; i++)
//                     fprintf(out, "%d%c", pi[i] + 1, i + 1 < n ? ' ' : '\n');
//             }
//         }

//         if (checked % progress_every == 0)
//         {
//             double elapsed = difftime(time(NULL), start);
//             printf("Checked %lld / %lld (%.4f%%), not_sortable so far = %lld, elapsed = %.0fs\n",
//                    checked, fact, 100.0 * checked / fact, not_sortable, elapsed);
//             fflush(stdout);
//         }
//     } while (next_permutation(pi, n));

//     fclose(out);

//     double elapsed = difftime(time(NULL), start);
//     printf("\nDone in %.0fs.\n", elapsed);
//     printf("Total permutations checked: %lld\n", checked);
//     printf("Not sortable by West-3 heuristic: %lld (%.4f%%)\n", not_sortable, 100.0 * not_sortable / checked);
//     printf("Wrote %lld example(s) to %s\n", not_sortable < sample_cap ? not_sortable : sample_cap, out_path);

//     return 0;
// }

// int main()
// {
//     printf("\nSorting with 3 stacks in series:\n");
//     int small[] = {10, 9, 5, 4, 0, 8, 7, 3, 2, 1, 6}; // 0-indexed permutation
//     clock_t start = clock();
//     bool sorted = sort_with_stacks(small, sizeof(small) / sizeof(small[0]), 3, true);
//     clock_t end = clock();
//     printf("Sorted: %s\n", sorted ? "yes" : "no");
//     printf("Time: %.4fs\n", (double)(end - start) / CLOCKS_PER_SEC);
//     return 0;
// }

// int main()
// {
//     int pi[] = {10, 6, 2, 1, 9, 8, 7, 0, 5, 4, 3}; // 0-indexed; prints as [11 7 3 2 10 9 8 1 6 5 4]
//     int n = sizeof(pi) / sizeof(pi[0]);

//     printf("=== Exact search: sort_with_stacks (3 live stacks in series) ===\n");
//     clock_t start = clock();
//     bool exact_sorted = sort_with_stacks(pi, n, 3, true);
//     clock_t end = clock();
//     printf("Sortable (exact): %s\n\n", exact_sorted ? "yes" : "no");

//     printf("=== Heuristic: West-3 (3 separate greedy passes) ===\n");
//     bool heuristic_sorted = west_k_stack_sortable(pi, n, 3, true);
//     printf("Sortable (West-3): %s\n\n", heuristic_sorted ? "yes" : "no");

//     printf("=== Heuristic: 3-pyramid greedy (patience-sorting piles) ===\n");
//     bool pyramid_sorted = pyramid_sortable(pi, n, 3, true);
//     printf("Sortable (3-pyramid): %s\n", pyramid_sorted ? "yes" : "no");
//     printf("Time: %.4fs\n", (double)(end - start) / CLOCKS_PER_SEC);
//     return 0;
// }

// Sweeps every permutation of size n and checks it against the 3-pyramid
// heuristic (pyramid_sortable(pi, n, 3, false)), which succeeds exactly when
// the permutation's longest increasing subsequence (LIS) is <= 3 (proven
// exhaustively for n=1..7 earlier). Every not-sortable permutation found is
// printed and written to file -- no sample cap. That is only safe for
// moderate n: n=14 alone has ~86 billion not-sortable permutations, which
// would mean printing billions of lines and writing several terabytes to
// disk, so this defaults to a much smaller n. Raise n only with that growth
// in mind (checked count is n!, and the not-sortable share climbs fast: it
// was already ~84% by n=11 and ~99% by n=14).
int main()
{
    int n = 7;
    int pi[MAX_N];
    initialize_identity_permutation(pi, n);

    long long fact = 1;
    for (int x = 2; x <= n; x++)
        fact *= x;

    char out_path[512];
    snprintf(out_path, sizeof(out_path),
             "/Users/nhattruong/Documents/Summer 2026/CALDAM2027/dArray/pyramid_not_sortable_n%d.txt", n);
    FILE *out = fopen(out_path, "w");
    if (!out)
    {
        printf("Error: cannot open %s for writing.\n", out_path);
        return 1;
    }

    const long long progress_every = 100000000LL;

    long long checked = 0, sortable = 0, not_sortable = 0;
    time_t start = time(NULL);

    do
    {
        checked++;
        if (pyramid_sortable(pi, n, 3, false))
        {
            sortable++;
        }
        else
        {
            not_sortable++;
            print_array(pi, n);
            for (int i = 0; i < n; i++)
                fprintf(out, "%d%c", pi[i] + 1, i + 1 < n ? ' ' : '\n');
        }

        if (checked % progress_every == 0)
        {
            double elapsed = difftime(time(NULL), start);
            printf("Checked %lld / %lld (%.4f%%), sortable so far = %lld, elapsed = %.0fs\n",
                   checked, fact, 100.0 * checked / fact, sortable, elapsed);
            fflush(stdout);
        }
    } while (next_permutation(pi, n));

    fclose(out);

    double elapsed = difftime(time(NULL), start);
    printf("\nDone in %.0fs.\n", elapsed);
    printf("Total permutations checked: %lld\n", checked);
    printf("Sortable by 3-pyramid heuristic (LIS <= 3): %lld (%.4f%%)\n", sortable, 100.0 * sortable / checked);
    printf("Not sortable: %lld (%.4f%%)\n", not_sortable, 100.0 * not_sortable / checked);
    printf("Wrote all %lld not-sortable permutation(s) to %s\n", not_sortable, out_path);

    return 0;
}

// int main()
// {
//     ComputeTDistanceFromIdentity(12);
//     return 0;
// }
