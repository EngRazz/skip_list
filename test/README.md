# Correctness tests

One runner tests the four structural methods and `split_rebuild` from
`src/skip_list.cpp`. Each method has a small source file; there are no timing
loops or CSV writes in the tests.

| File | Purpose |
| --- | --- |
| `skip_list_test.cpp` | Command-line entry point and readable per-seed summary |
| `insertion_tests.cpp` | Ordered/shuffled inputs, duplicates, signed boundaries |
| `search_tests.cpp` | Hits, misses, sentinel collisions, unchanged state |
| `union_tests.cpp` | Empty/unbalanced operands, consumed-input reuse, output lifetime |
| `split_tests.cpp` | Shared structural/rebuilding pivot cases, round trips, independence and input reuse |
| `mixed_tests.cpp` | 1,000 mixed steps, interleaved shared RNG use, disabled copying |
| `test_support.cpp` | Invariants, reference checks, ownership helpers, crash isolation |
| `../support/implementation.hpp` | Aliases for the shared implementation |
| `../support/runner_options.hpp` | Shared method/seed argument parsing |

Run commands from the project directory:

```sh
./build/skip_list_test --all
./build/skip_list_test_sanitized --all

./build/skip_list_test --split-rebuild
./build/skip_list_test_sanitized --split-rebuild --seeds 0,7,17,27
```

Select `--insert`, `--search`, `--ordered-union`, `--split`, or `--split-rebuild`
to run one method. There is no separate rebuilding library or executable.
`--seeds 42` and `--seeds 7,17,27` are accepted; the default is `7,17,27`.
`--all` runs every suite, including rebuilding split, mixed operations and copy
checks. `--split-rebuild` also runs 1,000 mixed steps with rebuilding split and
structural union for every selected seed.

The handler is initialized once per case and outlives every list using it.
Consumed objects must remain empty and reusable. Stack inputs and heap-owned
outputs are both exercised; all surviving objects are cleaned up by their owners.
Structural union/split must preserve data-node addresses and tower heights.
Rebuilding split must create fresh data nodes using ordinary insertions. Its
checks predict the new heights and final generator state from the shared RNG,
following the implementation's ascending-key traversal. The existing input
nodes are still alive during these insertions, so output addresses must differ.
Both variants must consume the input, preserve its handler, and allow reuse.

The RNG check predicts the next geometric height from a copy of the exposed
handler's `promote` distribution and `generator`, then checks both the actual
height and generator state. It checks every insertion in the RNG regressions,
including both outputs and reused inputs; it does not merely check that the
generator changed once.

Each case runs in a child process with a 90-second timeout, so a crash does not
hide the remaining cases. This uses POSIX `fork`/`waitpid` (macOS and Linux).
Sanitized targets instrument the tests and implementation with ASan and UBSan.
UBSan findings stop the case. Leak detection depends on the platform's sanitizer
support; Apple ASan is not a complete leak checker.
The sanitized tests also check that each pre-existing node or sentinel is either
reachable from an input/output or marked freed by ASan after union/split. This
catches detached old allocations, but is not a complete process-wide leak scan.
