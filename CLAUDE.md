# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project state

This is a CS370 homework assignment implementing a red-black tree in C. The repo currently
contains only the frozen public API header (`include/rbtree.h`) and a `Makefile`. The
implementation and tests do not exist yet and need to be created:

- `src/rbtree.c` — the red-black tree implementation (not yet created)
- `tests/test_rbtree.c` — unit tests (not yet created)
- `tests/fuzz.c` — a fuzz-testing harness invoked as `./build/fuzz <iterations>` (not yet created)

## The API contract (include/rbtree.h)

`include/rbtree.h` is called out in the git history as "frozen" — treat its declared function
signatures and ownership semantics as fixed. Implement against it rather than editing it, unless
the user explicitly asks for the header itself to change.

Key ownership rules baked into the API (see the header's comments for full detail):
- `rb_create` takes an optional `rb_value_free_fn` used to free values the tree takes ownership of.
- `rb_insert` copies the key (tree owns the copy) and takes ownership of the value **only on
  success**; on allocation failure (`-1`) the tree is unchanged and the caller retains ownership
  of the value it passed in. Overwriting an existing key frees the old value via `value_free`.
- `rb_find` returns a borrowed pointer — the tree retains ownership.
- `rb_delete` frees both the key copy and the value.
- `rb_validate` returns `0` iff all red-black invariants hold.
- `rb_destroy` is NULL-safe and frees all nodes, key copies, and owned values.

## Build and test commands

```sh
make                # build build/test_rbtree and build/fuzz
make test           # build, then run the unit tests followed by the fuzz harness (100000 iterations)
make asan           # clean + rebuild with -fsanitize=address,undefined, then run make test
make memcheck       # build, then run both binaries under valgrind (--leak-check=full, error on leaks)
make clean          # remove build/
```

To run just one binary directly (e.g. after `make`, for a single test run or a custom fuzz
iteration count):

```sh
./build/test_rbtree
./build/fuzz 20000
```

Compiler flags (`Makefile`): `gcc -std=c23 -Wall -Wextra -Werror -g -O1 -Iinclude`. Warnings are
errors — code must compile clean under `-Wall -Wextra`. `-O1` is intentional (not `-O0`/`-O2`);
preserve it unless there's a specific reason to change optimization behavior.


# rbtree-lab: project rules
## Commands
- Build & unit tests: ‘make test‘
- Sanitizers: ‘make asan‘ Valgrind: ‘make memcheck‘
- A change is DONE only when all three pass. Always run them; show output.
## Hard constraints
- NEVER modify include/rbtree.h. It is the graded contract.
- Check every allocation. malloc can return NULL; a NULL return must
leave the tree unchanged and return the documented error code.
- NEVER weaken, skip, or delete a test to make the suite pass. If a test
looks wrong, stop and explain why instead.
## Style
- C23. -Wall -Wextra -Werror must stay clean. No VLAs.
- Error handling: goto-cleanup pattern for multi-allocation functions.
- Prefer the smallest diff that passes. Do not refactor unrelated code.
- Every non-obvious loop gets a one-line invariant comment.
## Workflow
- For any multi-file or algorithmic change: propose a plan and wait for
approval before editing.
- Commit only from a green state; message format "M<n>: <what>".