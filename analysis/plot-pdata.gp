set terminal pngcairo size 600,400 enhanced font 'Arial,12'
set output 'plot-pdata.png'

set xlabel "Number of threads"
set ylabel "Speedup"
set key outside
set grid

# Plot each depth column against X-Values
plot 'pdata.txt' using 1:2 with linespoints title "Depth 5", \
     'pdata.txt' using 1:3 with linespoints title "Depth 8", \
     'pdata.txt' using 1:4 with linespoints title "Depth 10", \
     'pdata.txt' using 1:5 with linespoints title "Depth 15"
