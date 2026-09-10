## 2026 - 09-01 (evening 2)
STATE: rb_destroy method recursive vs iterative; 
DID: Created rb_create, rb_insert, rb_find, rb_validate, and rb_destroy
LEARNED: learned that == does not always mean it compares the addresses of two things. Instead if a pointer is
placed between the comparator, then the addresses will be compared. Everything else, the contents that are placed between these two are compared. booleans, int, strings. etc.
NEXT FIRST STEP: Next thing I will do is running the predict-then-scroll drill on section 5. This will be done on the insertion sequence.
OPEN:

## 2026-09-09
STATE: fixed Makefile (recipe lines were missing tab separators, blocking every target); built rotate_left/rotate_right (root-update handled inside the rotation) and insert_fixup (red-uncle recolor, triangle-to-line zig-zag, both mirrored) on top of rb_insert; wrote tests/test_rbtree.c (stale-root regression checks for the four insertion orders that force a rotation exactly at the root) and tests/fuzz.c (random-insert fuzz harness, single end-of-run rb_validate check); added rb_foreach.
DID: make test and make asan both pass clean. make memcheck cannot run on this machine at all: confirmed via `brew info valgrind` that Homebrew's valgrind formula requires Linux and has no macOS support (Intel or Apple Silicon) - not something fixable locally.
LEARNED: a node with exactly one NIL child in a valid RB-tree can only be black with a red-leaf child - not because of black-height alone (black-height math actually also allows a red parent with a red leaf child), but because that specific case is separately ruled out by the no-red-red-child rule. Needed both invariants together, not just one, to pin it down.
NEXT FIRST STEP: rb_delete and its own fixup (recolor-only case and one-child-splice case already discussed conceptually), plus rb_size.
OPEN: make memcheck needs to be run on a Linux machine/VM/container instead - the code itself doesn't need to change, just where this target runs.
