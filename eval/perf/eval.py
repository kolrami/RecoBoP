import sys
import re
import Gnuplot
import subprocess

gp = Gnuplot.Gnuplot()
gp("set terminal pdf font 'Arial,12' size 14cm,5.5cm linewidth 1 monochrome")
gp("set datafile separator '\t'")
gp("set style data histograms")
gp("set style histogram cluster gap 3")

gp("set style fill pattern border -1")
#gp("set boxwidth 0.5")
gp("set key autotitle columnheader")

gp("set ylabel 'Execution Time in ms'")
gp("set y2label 'Power Consumption in W'")
gp("set xtics scale 0 rotate by -45")
gp("set y2tics 0.1")
gp("set xrange [-1:*]")
gp("set yrange [0.001:100]")
gp("set y2range [3.15:3.45]")
gp("set ytics nomirror")
gp("set logscale y")

gp("set output 'eval_perf.pdf'")
#gp("test")
gp.plot("""'data_perf.dat' using 3:xtic(1) axes x1y1 fs pattern 7,\
           '' using 4 axes x1y1 fs pattern 6,\
           '' using 5 axes x1y1 fs pattern 3,\
           '' using 6 axes x1y2 fs pattern 0""")