# The West-3 heuristic (`west_k_stack_sortable`)

Implementation: [function.c](function.c) (`west_stack_sort_pass`, `west_k_stack_sortable`), declared in [function.h](function.h).

## Why this exists

`sort_with_stacks` (the other sorting function in this codebase) answers "can this
permutation be sorted by `k` stacks chained in series?" *exactly*, via backtracking
search. That search is correct but can be very slow — proving a permutation is **not**
sortable means exhausting the search space, and that cost explodes with `n` (n=6→n=7
alone went from ~68μs to ~1.2ms per permutation in testing).

`west_k_stack_sortable` trades exactness for speed: it's a deterministic, non-backtracking
check that runs in O(n·k) time (measured ~160–190ns per call for n=8–11), making an
exhaustive sweep over billions of permutations (e.g. all 14! ≈ 87.2 billion for n=14)
actually finish in hours instead of never.

## The core algorithm: one greedy stack-sort pass

This is the classical single-stack sort algorithm (Knuth / Julian West), used as-is,
not invented for this project:

```c
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
```

Plain English: scan the input left to right. Before pushing the next value `x`, pop
anything off the stack that is *smaller* than `x` straight to the output (since once
`x` is on the stack in front of them, they'd never get a chance to leave in the right
order otherwise). Then push `x`. At the end, drain whatever's left on the stack.

This is a **single, deterministic pass** — no choices, no backtracking. It either
produces the identity or it doesn't.

### Worked example: one pass on `[2, 3, 1]`

| step | action | stack | output |
|---|---|---|---|
| read 2 | push | `[2]` | `[]` |
| read 3 | top(2) < 3 → pop 2; push 3 | `[3]` | `[2]` |
| read 1 | top(3) < 1? no → push 1 | `[3, 1]` | `[2]` |
| drain | pop 1, pop 3 | `[]` | `[2, 1, 3]` |

Result: `[2, 1, 3]` ≠ identity. One pass alone does **not** sort `[2, 3, 1]`. (This
matches the classical fact that a permutation is 1-stack-sortable iff it avoids the
pattern 231 — and `[2, 3, 1]` *is* the pattern 231.)

## West's definition, generalized to k passes

Julian West's actual definition of a "2-stack-sortable" permutation π is:
apply the pass above, then apply it *again* to the result, and check if you now
have the identity — i.e. `S(S(π)) == identity`.

`west_k_stack_sortable(pi, n, k)` generalizes this to any `k`: apply the pass `k`
times in a row, check for identity at the end.

```c
bool west_k_stack_sortable(const int *pi, int n, int k)
{
    int cur[MAX_N], next[MAX_N];
    memcpy(cur, pi, n * sizeof(int));

    for (int pass = 0; pass < k; pass++)
    {
        west_stack_sort_pass(cur, next, n);
        memcpy(cur, next, n * sizeof(int));
    }

    for (int i = 0; i < n; i++)
        if (cur[i] != i)
            return false;
    return true;
}
```

`k = 1` → the classical single-stack test. `k = 2` → West's own 2-stack-sortable
definition. `k = 3` → "West-3", the one used for the n=14 sweep.

### Continuing the example: two passes on `[2, 3, 1]`

Pass 1 gave `[2, 1, 3]`. Now feed *that* through the same pass again:

| step | action | stack | output |
|---|---|---|---|
| read 2 | push | `[2]` | `[]` |
| read 1 | top(2) < 1? no → push 1 | `[2, 1]` | `[]` |
| read 3 | top(1) < 3 → pop 1; top(2) < 3 → pop 2; push 3 | `[3]` | `[1, 2]` |
| drain | pop 3 | `[]` | `[1, 2, 3]` |

Result: `[1, 2, 3]` = identity. So `[2, 3, 1]` **is** West-2-sortable — it just
needed two passes, not one.

### Where it breaks down: three passes on `[2, 4, 3, 5, 7, 6, 1]`

This permutation showed up earlier in this project as an example that the *exact*
`sort_with_stacks` search proves **is** sortable by 3 stacks in series (a valid
28-move sequence was found). Running it through West-3:

```
start:          [2, 4, 3, 5, 7, 6, 1]
after pass 1:   [2, 3, 4, 5, 1, 6, 7]
after pass 2:   [2, 3, 4, 1, 5, 6, 7]
after pass 3:   [2, 3, 1, 4, 5, 6, 7]
```

Three passes still isn't the identity — `west_k_stack_sortable(pi, 7, 3)` returns
**false** for this permutation, even though it genuinely *can* be sorted by 3 stacks
in series (just not by this particular greedy, non-backtracking strategy). This is
the heuristic's incompleteness, made concrete.

## Why "false" here is not proof of unsortability

This is the most important thing to understand about this function:

- **`true` is a real proof.** If `west_k_stack_sortable` returns `true`, it just
  *constructed* a valid sort — 3 separate greedy left-to-right passes that reached
  the identity. That's a witness, not a guess.
- **`false` only means "this specific greedy method didn't find one."** The 3
  West-style passes are 3 separate *complete* left-to-right sweeps with no
  backtracking — a fundamentally more restrictive process than the live
  3-stacks-in-series pipeline `sort_with_stacks` explores (which can interleave
  pushes/shifts/pops across the whole stream and backtrack out of dead ends).
  A permutation can fail the heuristic and still be genuinely sortable by 3 stacks
  in series, as shown above.

So a "not sortable (West-3)" result is really "not sortable by this fast heuristic" —
a candidate list, not a verified list.

## Correctness validation (exhaustive, against known math)

Before trusting this for a 14! sweep, it was checked against two independently
known results, exhaustively (every permutation, not a sample) for n = 1–8:

**k=1 must equal the Catalan numbers** (the known count of 231-avoiding permutations):

| n | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
|---|---|---|---|---|---|---|---|---|
| got / expected | 1 | 2 | 5 | 14 | 42 | 132 | 429 | 1430 |

Exact match at every n.

**k=2 must equal West's own published 2-stack-sortable formula**
`2·(3n)! / ((n+1)!·(2n+1)!)`:

| n | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
|---|---|---|---|---|---|---|---|---|
| got / expected | 1 | 2 | 6 | 22 | 91 | 408 | 1938 | 9614 |

Exact match at every n.

## How it compares to the exact search (k=3), exhaustively for n = 1–7

| n | heuristic says sortable | exact search says sortable | heuristic wrong (says yes, exact says no) | heuristic misses (exact yes, heuristic no) |
|---|---|---|---|---|
| 1–4 | matches exactly | matches exactly | 0 | 0 |
| 5 | 114 / 120 | 120 / 120 | 0 | 6 |
| 6 | 606 / 720 | 720 / 720 | 0 | 114 |
| 7 | 3494 / 5040 | 5040 / 5040 | 0 | 1546 |

Two things stand out:
1. **"heuristic wrong" is always 0** — every permutation the heuristic accepts, the
   exact search also accepts. It never lies in the optimistic direction.
2. **"heuristic misses" grows fast** — by n=7 it's already missing ~31% of the
   permutations that are genuinely sortable.

## Performance (why n=14 uses this instead of the exact search)

Measured directly (n=8–11, `-O3`), enumerating every permutation with a standard
`next_permutation`:

| n | permutations | not-sortable rate (West-3) | time/permutation |
|---|---|---|---|
| 8 | 40,320 | 46.9% | 162 ns |
| 9 | 362,880 | 62.0% | 177 ns |
| 10 | 3,628,800 | 74.6% | 179 ns |
| 11 | 39,916,800 | 84.0% | 191 ns |

Two takeaways used to justify the n=14 sweep design:
- Cost per permutation grows slowly (~O(n)), so 14! ≈ 87.2 billion permutations at
  ~220ns extrapolated ≈ 5–6 hours — feasible as a background job. The exact
  backtracking search, by contrast, was already impractical to even estimate past
  n=7 for a full 14! sweep.
- The "not sortable" rate is climbing fast (47% → 84% from n=8 to n=11), so by n=14
  the *large majority* of permutations are expected to fail the heuristic — a
  reminder that the resulting count is dominated by the heuristic's own blind spots,
  not necessarily by true unsortability.

## Function reference

```c
bool west_k_stack_sortable(const int *pi, int n, int k);
```

- `pi` — 0-indexed permutation of size `n` (values `0..n-1`), same convention as the
  rest of this codebase.
- `n` — permutation length.
- `k` — number of greedy passes to apply. `k=2` reproduces West's own definition;
  `k=3` is the "West-3" heuristic.
- Returns `true` iff `k` passes of the greedy single-stack sort reach the identity.
