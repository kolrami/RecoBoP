import sys
import re
import Gnuplot
import subprocess

gp = Gnuplot.Gnuplot()
gp("set terminal pdf font 'Arial,12' size 14cm,4cm linewidth 1")
gp("set datafile separator '\t'")
#gp("set style data histograms")
#gp("set style histogram rowstacked")

gp("set xlabel 'Time in s'")
gp("set ylabel 'Scan Rate in ms'")
gp("set y2label 'Angle in °'")
gp("set xtics 4")
gp("set y2tics 2")
gp("set xrange [0:16]")
gp("set yrange [0:110]")
gp("set y2range [0:12]")
gp("set ytics nomirror")
gp("set output 'eval_saw_rate.pdf'")
#gp("test")
gp.plot("'data_saw_rate.dat' using (($1-6591000) / 1000000.0):4 axes x1y2 with lines dt 3 lc rgb'black' title 'Angle of Plate', 'data_saw_rate.dat' using (($1-6591000) / 1000000.0):5 axes x1y1 with lines dt 1 lc black title 'Scan Rate'")


gp("set xlabel 'Time in s'")
gp("set ylabel 'Scan Rate in ms'")
gp("set y2label 'Power Consumption in W'")
gp("set xtics 4")
gp("set y2tics .1")
gp("set xrange [0:16]")
gp("set yrange [0:110]")
gp("set y2range [3.4:3.7]")
gp("set ytics nomirror")
gp("set output 'eval_saw_power.pdf");
gp.plot("'data_saw_pw.dat' using (($1-6487000) / 1000000.0):2 axes x1y1 with lines dt 3 lc black title 'Scan Rate', 'data_saw_pw.dat' using (($1-6487000) / 1000000.0):3 axes x1y2 with lines dt 1 lc rgb'black' title 'Power Consumption'")
