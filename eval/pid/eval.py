import sys
import re
import Gnuplot
import subprocess

gp = Gnuplot.Gnuplot()
gp("set terminal pdf font 'Arial,12' size 7.3cm,4cm linewidth 1")
gp("set datafile separator '\t'")
#gp("set style data histograms")
#gp("set style histogram rowstacked")
gp("set xlabel 'X'")
gp("set ylabel 'Y'")
#gp("set style fill solid border -1")
gp("set key off")
#gp("set boxwidth 0.5")

gp("set xtics 300")
gp("set ytics 300")
gp("set xrange [-1100:1100]")
gp("set yrange [-800:800]")

gp("set output 'eval_pos.pdf'")
gp.plot("'data_pos0.dat' using 1:2 with points ps 0.3 title 'Sample 1', 'data_pos1.dat' using 1:2 with points ps 0.3 title 'Sample 2', 'data_pos2.dat' using 1:2 with points ps 0.3 lt 6 title 'Sample 3'")

gp("set output 'eval_traj.pdf")
gp.plot("""'data_pos3.dat' using 1:2 with points ps 0.3,\
           '' using 3:4 with line""")
#gp("set output 'eval_router.pdf'")
#gp.plot("'data_router.dat' using 2:xtic(1) linecolor rgb'black', '' using 3 linecolor rgb'white'")

#subprocess.call("epstopdf eval_arbiter.ps", shell=True)
#subprocess.call("epstopdf eval_router.ps", shell=True)
