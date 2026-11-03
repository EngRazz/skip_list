# Skip list

This project implements a randomized skip list of signed 64-bit integer keys.
It includes search, insertion, ordered union, structural split, and a rebuilding
split used for comparison.

## Academic Context

Developed as part of the **Advanced Algorithms and Parallel Programming** course at **Politecnico di Milano**. The project received the maximum score of **3/3** in the course challenge.

## Requirements

To build locally, install CMake 3.17 or newer, Git, and a C++17 compiler.

## Build and run the correctness tests

From the project directory:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

./build/skip_list_test --all
./build/skip_list_test_sanitized --all
```

The sanitized executable uses AddressSanitizer and UndefinedBehaviorSanitizer.
To run only one group, replace `--all` with `--insert`, `--search`,
`--ordered-union`, `--split`, or `--split-rebuild`.

## Run the notebook on Colab

1. Create `skip_list.zip` from the project directory.
2. Do not include `build/`, `.cache/`, downloaded dependencies, or other built
   files in the ZIP.
3. Open `google_benchmark/notebooks/analysis_colab.ipynb` in Google Colab.
4. Run the first cell and select `skip_list.zip` when Colab asks for it.
5. Set `RERUN_BENCHMARKS = True` to build and collect a new benchmark run.
   Set it to `False` to reuse saved JSON and regenerate only the statistics and
   figures.
6. Run Setup, then the method sections in their displayed order.
7. Run the final download cell to save the JSON data, CSV files, and figures
   before the Colab runtime is reset.

## Disclaimer

LLMs were used to help develop the correctness tests, the notebooks, and the
integration with the Google Benchmark library. No LLM was used to write the
skip-list source files or header files.

