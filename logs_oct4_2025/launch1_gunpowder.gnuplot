set title "launch 1 with gunpowder discharge"
set xlabel "microseconds"
set ylabel "newtons"
plot "launch1_gunpowder.txt" using 1:4 with lines title ""
unset xtics
