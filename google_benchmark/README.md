# Google Benchmark: structural kernels and rebuilding split

These experiments measure search, insertion, ordered union, and split in
`src/skip_list.cpp`, plus its `split_rebuild` baseline on the same split inputs.

## Files

| File | Purpose |
| --- | --- |
| `src/benchmark_support.hpp` | Configuration and untimed input helpers |
| `src/search_benchmark.cpp` | Query generation, timed searches and registration |
| `src/insertion_benchmark.cpp` | Absent keys and insertion batches |
| `src/union_benchmark.cpp` | Balanced ordered union of fresh inputs |
| `src/split_benchmark.cpp` | Structural and rebuilding split on identical fresh inputs |
| `src/main.cpp` | Google Benchmark command-line entry point |
| `notebooks/analysis.ipynb` | Build, run, export, plot and interpret the experiment |

The notebook runs these source files; it does not generate C++ files. Open it
locally inside the project, or upload it to Colab and then upload the project
ZIP/TAR.GZ when prompted. The archive must contain the source and CMake files,
without build directories or downloaded dependencies.

## Build and run from the project root

You need Git, CMake 3.17 or newer, and a C++17 compiler. Python analysis uses
pandas and matplotlib. CMake downloads Google Benchmark v1.10.0, pinned to commit
`a6b4a9f00b36c19af9997d68a1cf6e0b5b3d032c`, inside the build directory.
No global installation or Google Test dependency is needed.

```sh
cmake -S . -B build/google_benchmark -DCMAKE_BUILD_TYPE=Release -DBUILD_GOOGLE_BENCHMARK=ON
cmake --build build/google_benchmark --target skip_list_benchmark --parallel 2
cmake --build build/google_benchmark --target skip_list_test --parallel 2
./build/google_benchmark/skip_list_test --split-rebuild --seeds 42,900,5783

./build/google_benchmark/skip_list_benchmark --benchmark_list_tests=true

mkdir -p google_benchmark/JSON/local
./build/google_benchmark/skip_list_benchmark --benchmark_min_time=0.1s --benchmark_out=google_benchmark/JSON/local/all_methods.json --benchmark_out_format=json
```

The executable runs all four structural methods and rebuilding split by default.
To select one, use a filter:

```sh
./build/google_benchmark/skip_list_benchmark --benchmark_filter='^insertion/'
./build/google_benchmark/skip_list_benchmark --benchmark_filter='^ordered_union/'
./build/google_benchmark/skip_list_benchmark --benchmark_filter='^split/'
./build/google_benchmark/skip_list_benchmark --benchmark_filter='^split_rebuild/'
```

The notebook has a shared Setup section followed by Search, Insert, Ordered
union, and Split, in that order. Each method has its own run-and-save, conversion,
and plot cells. The Split section ends with a rebuilding comparison: run its
structural cells first, then the rebuilding cells. The rebuilding run first
checks `--split-rebuild` and stops on correctness failures. Run Setup once,
then run the sections you need. Use a different
`RUN_LABEL` when keeping results from separate experiments.

Benchmarks are optional in CMake, so ordinary correctness builds do not download
the timing library. Build and run correctness/sanitizer executables separately.
The benchmark target links the ordinary library without sanitizer flags.

The notebook runs these same steps, then writes:

- `JSON/local/search.json`, `insertion.json`, `ordered_union.json`, `split.json`, `split_rebuild.json`:
  separate raw repetitions, summaries and complexity fits for each method.
- `JSON/local/environment.json`: library version, compiler commands, OS and processor.
- `CSVs/local/`: individual measurements, summaries and fits for each method.
- `figures/local/`: one PNG/PDF figure per structural method and `split_comparison`.
- `CSVs/local/split_comparison.csv`: mean times and rebuilding/structural ratios
  matched by input size and height seed.

Colab uses `colab` instead of `local`. Change the notebook's `RUN_LABEL` to retain
multiple runs. Reusing a label replaces files with the same names in that run's
output directories. Save the Colab results before its temporary runtime resets.

## Workload

- Sizes: 1024, 4096, 16384, 65536, 262144, 1048576.
- Independent height seeds: 42, 900, 5783; promotion probability 1/2.
- Three timing repetitions per seed and size: 54 records per family, 270 in total.
- Initial keys are even, inserted in ascending order.
- Search and insertion use a separate workload generator seeded with 17.

| Method | Meaning of N and input | Operations per iteration |
| --- | --- | --- |
| Search | N keys from 0 through `2*(N-1)` | 256 searches: half hits, half misses |
| Insertion | N initial keys from 0 through `2*(N-1)` | 32 successful insertions |
| Ordered union | N total keys, N/2 in each ordered disjoint input | One union |
| Split | N keys, absent pivot N-1 giving equal output sizes | One split |
| Rebuilding split | Identical initial keys, heights and pivot to structural split | One rebuilding split |

Search queries cover 128 parts of the key range. In each part, sample an index,
use its even key and adjacent odd key, then shuffle all queries. The same batch
is reused. A small untimed check warms its paths, so this measures repeated
queries on a warmed structure.

Insertion keys cover 32 disjoint parts of the range. Sample one odd key per
part, then shuffle them. All are distinct and absent from the initial even keys.
The size grows from N to N+32 within a batch. Each iteration reconstructs the
original N-key input and resets its handler to the selected seed.

Union builds A from even keys 0 through N-2 and B from N through `2*N-2`.
They share a handler and use successive draws, without reseeding between them.
Every iteration builds fresh A and B. Split likewise reconstructs its N-key
input before each iteration; its pivot is determined before timing.
Both split families start from the same seed and construction order. Rebuilding
continues the shared height generator after constructing the input; it does not
reseed the output insertions. The timed public `split_rebuild` call includes
ordinary insertions into fresh outputs and disposal of all consumed data nodes.
The input object remains empty and reusable. Post-operation checks reject a
baseline that does not consume the input or draw fresh heights.

`PauseTiming`/`ResumeTiming` exclude input construction, validation, destruction
of outputs, and destruction of consumed input objects. Kernel allocations,
height generation, header handling and internal cleanup remain timed. Results
pass through `DoNotOptimize`; mutation drivers also use `ClobberMemory`.

Search uses automatic calibration (`--benchmark_min_time=0.1s` in the notebook).
The modifying methods use `Iterations(10)` to avoid unlimited calibration and
reconstruction of large inputs. Change `mutation_iterations` in the support
header and rebuild to adjust this count; `--benchmark_min_time` does not override
it. Each repetition therefore averages 320 insertions, 10 unions or 10 splits.
Untimed setup still consumes real time: allow several minutes for a full run.
The short timed regions remain sensitive to timer/loop overhead and system noise.

Height seeds sample structures; timing repetitions measure variability for the
same seeded structure. Google Benchmark separately calibrates loop iterations.
Seeds reproduce the workload within the same toolchain/library environment;
do not combine measurements from different machines into one timing curve.

## Read the results

Google Benchmark reports time **per iteration**, in nanoseconds. Divide CPU and
elapsed times by `operations_per_iteration`: 256 for search, 32 for insertion,
and 1 for union/split. `SetItemsProcessed` reports throughput but does not perform
this division for timings or complexity fits.

Each structural method uses `Complexity(benchmark::oLogN)`, while rebuilding
split uses `Complexity(benchmark::oN)`: 15 Google Benchmark fits across the
five families and three seeds. The shapes are supplied explicitly; Google
Benchmark estimates the coefficient in `a * log2(N)` for the structural kernels
or `a * N` for rebuilding split, with no intercept. The notebook divides that coefficient by the
batch size and converts normalized RMS to a percentage. RMS itself is not divided
by the batch size. The JSON context records the structural model as
`complexity_model` and the rebuilding model as `split_rebuild_complexity_model`.
Rebuild and rerun before analyzing the new fits; older JSON is not relabelled.

Each structural method section also fits `a * log2(N) + b`; rebuilding split
fits `a * N + b`. These custom fits use `numpy.linalg.lstsq` on
all individual CPU timings per operation for each seed. Both fits use the same
observations. The custom normalized error is
`100 * sqrt(mean((measured - predicted)**2)) / mean(measured)`, matching Google
Benchmark's CPU RMS definition. A comparison table reports both coefficients
and errors; a separate figure shows the measurements and both curves per seed.
The custom model has an extra parameter, so a lower error is expected and does
not prove asymptotic complexity. A negative intercept is permitted, and the
fit should not be extrapolated beyond the measured sizes.

Each family writes `<method>_custom_complexity.csv` and
`<method>_fit_comparison.csv` alongside its Google Benchmark complexity CSV.
The model-comparison figures are `<method>_fit_comparison.png` and `.pdf`.
The existing `<method>_complexity.csv` continues to contain the Google Benchmark
coefficient and RMS. Raw JSON/CSV results from earlier runs are preserved until
the corresponding notebook run/export cells are executed again.

Each plot uses mean CPU time per operation and sample standard deviation over three
repetitions for each seed/size. Error bars do not represent confidence intervals.
A second panel plots time divided by `log2(N)`. Discuss the fitted model, noise,
cache effects and workload limitations; a timing fit is not a complexity proof.

The notebook records compiler commands, library revision, OS and processor once
in Setup. Section 7 of the assignment asks for this information to document the
measurement conditions. Raw JSON also contains Google Benchmark's machine
context. The lecture notebook prints CPU/cache information in its benchmark
output and shows compiler flags in build commands, but does not save a separate
environment file. The metadata file is a convenient record, not an installation
or a copy of the software environment.

## Interpreting the split comparison

Structural split targets expected O(log(N+2)) work and transfers existing data
nodes. The rebuilding baseline uses N ordinary insertions, giving expected
O(N log(N+2)) work, plus expected O(N) disposal of consumed nodes. Both variants
use the same workload, seeds, iteration count and timing controls. The comparison
plot shows mean CPU times and their ratio; it does not establish either bound.
The rebuilding benchmark uses the explicitly requested linear fit.
This is a hypothesis to evaluate, not its expected asymptotic cost; a large RMS
for either linear model should be reported.

Logical-work counts for all four structural kernels and data-node allocation
counts for this comparison still belong in separate instrumented runs. They
are not implemented by the timing driver or inferred from timings. Instrumentation
must be disabled during timing. Changes to `src` and `include` remain with the
implementation's author.

## Sources and assistance

- Course notebook `lecture3-DandC-profiling.ipynb`.
- Assignment `docs/Skip_List_Challenge.pdf`, Sections 6 and 7.
- [Google Benchmark user guide](https://google.github.io/benchmark/user_guide.html).
- [Google Benchmark v1.10.0](https://github.com/google/benchmark/releases/tag/v1.10.0).

The benchmark drivers, build integration, documentation and notebook were prepared
with OpenAI Codex assistance. Include this assistance in the final project README.
