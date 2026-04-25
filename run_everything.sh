rm out_*
make
time ./ROUTE.exe -d=0 -n=0 benchmarks/adaptec1.gr out_1_00.txt > time.txt
time ./ROUTE.exe -d=0 -n=1 benchmarks/adaptec1.gr out_1_01.txt >> time.txt
time ./ROUTE.exe -d=1 -n=0 benchmarks/adaptec1.gr out_1_10.txt >> time.txt
time ./ROUTE.exe -d=1 -n=1 benchmarks/adaptec1.gr out_1_11.txt >> time.txt

time ./ROUTE.exe -d=0 -n=0 benchmarks/adaptec2.gr out_2_00.txt >> time.txt
time ./ROUTE.exe -d=0 -n=1 benchmarks/adaptec2.gr out_2_01.txt >> time.txt
time ./ROUTE.exe -d=1 -n=0 benchmarks/adaptec2.gr out_2_10.txt >> time.txt
time ./ROUTE.exe -d=1 -n=1 benchmarks/adaptec2.gr out_2_11.txt >> time.txt

time ./ROUTE.exe -d=0 -n=0 benchmarks/adaptec3.gr out_3_00.txt >> time.txt
time ./ROUTE.exe -d=0 -n=1 benchmarks/adaptec3.gr out_3_01.txt >> time.txt
time ./ROUTE.exe -d=1 -n=0 benchmarks/adaptec3.gr out_3_10.txt >> time.txt
time ./ROUTE.exe -d=1 -n=1 benchmarks/adaptec3.gr out_3_11.txt >> time.txt

./556_eval.exe benchmarks/adaptec1.gr out_1_00.txt 0 > check.txt
./556_eval.exe benchmarks/adaptec1.gr out_1_01.txt 0 >> check.txt
./556_eval.exe benchmarks/adaptec1.gr out_1_10.txt 0 >> check.txt
./556_eval.exe benchmarks/adaptec1.gr out_1_11.txt 0 >> check.txt

./556_eval.exe benchmarks/adaptec2.gr out_2_00.txt 0 >> check.txt
./556_eval.exe benchmarks/adaptec2.gr out_2_01.txt 0 >> check.txt
./556_eval.exe benchmarks/adaptec2.gr out_2_10.txt 0 >> check.txt
./556_eval.exe benchmarks/adaptec2.gr out_2_11.txt 0 >> check.txt

./556_eval.exe benchmarks/adaptec3.gr out_3_00.txt 0 >> check.txt
./556_eval.exe benchmarks/adaptec3.gr out_3_01.txt 0 >> check.txt
./556_eval.exe benchmarks/adaptec3.gr out_3_10.txt 0 >> check.txt
./556_eval.exe benchmarks/adaptec3.gr out_3_11.txt 0 >> check.txt