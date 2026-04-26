make

rm out_1_00.txt

# Run with valgrind
# valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./ROUTE.exe -d=1 -n=0 benchmarks/adaptec1.gr out_1_00.txt

# Run without valgrind
time ./ROUTE.exe -d=1 -n=1 benchmarks/adaptec3.gr out_3_11.txt >> time.txt
./556_eval.exe benchmarks/adaptec1.gr out_1_00.txt 0

# Run gdb
# gdb --args ./ROUTE.exe in1.txt potentialOut.txt