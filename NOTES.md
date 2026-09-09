# Red-Black Tree

1. Five Rules That governs the colors
    - Every Node is red or black
    - The root is always Black
    - Every Missing Child ends at a black sentinel leaf, NIL.
        - We treat NULL as black as well.
    - Red Nodes always have black children. I.E. red never stacks
    - Every path down to any NIL nodes crosses the same number of black nodes. This is called a black-height.

2. Possible errors with RED-BLACK TREES
    - Insert
        - Might place a RED node in a leaf position.
    - Delete
        - Might remove a BLACK node and breaks the black-height rule.


3. Possible FIX with Insertion and Deletion
    - Recoloring; flips colors and moves no pointers
    - Rotation: constant time pointer surger that lifts a child over its parent while preserving the in-order sequence.
        - Insertion: requires at most 2 rotations.
        - Deletion: requires at most 3.

# Java to C: Formerly Garbaged-Collected

1. Pointer is not the object
    - an adress stored in a variable. Nothing stops the address from being garbaged. In Java it stops it but in C it does not.
2. " & "
    - &x is an address of x
3. " * "
    - *x is thing at that adress
    - *x is a declaration
    - *x is a deference
4. Being able to reach an object through a pointer means that you are not responsible for it.
    - It is up to you how to handle it

# Memory You Must Give Back

1. malloc(n) only gives raw bytes
    - this produces no zeroing
2. free(p) is called only once and never to touch a pointer after
3. Memory Problems
    - Leak: forgetting to free something
    - Double Free: Freeing something twice
    - Use-After-Free: Something that is freed but is still being used.

# Strings Are Just Bytes But With A Promis

1. A C String is a char array ending in '\0' byte with no built in length tracking
2. Losing the terminator can result in leaks or unrelated memory being accessed
3. == compares adresses, not contents
4. Copying a string is as follows; malloc(strlen(s) + 1)
5. The RB tree must have its own copy of every key. NEVER STORE THE CALLERS POINTER
6. Two seperate allocations (the node itself and key copy)
    - both must be eventually freed.

# Structs and the Anatomy of A Node

1. Struct is just a named field in memory; contains no methods,constructors, or anything
2. Suggested Node layout is key (heap copy), value (owned per the API contract), left, right, color
3. sizeof*n = means "size of the thing n points to"; compile time and not a deference
4. Whether to add a parent pointer is a real time decision.

# Ownership is the Contract
1. Pointer tells you how to reach an object; Ownership tells you who is responsible
    - Many pointers reach an object but an owner tells it how to free it
2. Track three things seperately per node
    - the node allocation
    - the tree private key copy
    - the opaque value
3. The tree owns every node and every key copy
4. OwnerShip of Value follows the following
    - Successful rb_insert; the tree takes ownership of value; caller must never free
    - Failed rb_insert; ownership is never transferred; caller still owns and must handle the value
    - Overwriting an existing key: the tree frees the old value before installing the said new one.
5. Rotations change the topology and never change who owns the node
