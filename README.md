# Parallel Implementation of Random Art with OpenMP

## Build
Run `make`.

## Usage
`./rart GRAMMAR_FILE [-o OUTPUT_FILE] [-w WIDTH] [-h HEIGHT] [-d DEPTH] [-t NUM_THREADS] [-c] [-p] [-r]`
where
- `GRAMMAR_FILE`: the grammar file as the input (see `grammar_example` file)
- `-o OUTPUT_FILE`: the output file
- `-w WIDTH`: width of the output random art image
- `-h HEIGHT`: height of the output random art image
- `-d DEPTH`: depth of the expression tree
- `-t NUM_THREADS`: number of threads
- `-c`: compare the time with the case where the program is run sequentially
- `-p`: print expression trees for RGB channels
- `-r`: use expression tree evaluation level parallelism (default pixel level parallelism)

## Analysis
The report is under `analysis/report.pdf`
