rm out_*
make

echo "1_00" > check.txt
(time ./ROUTE.exe -d=0 -n=0 benchmarks/adaptec1.gr out_1_00.txt) &>> check.txt
./556_eval.exe benchmarks/adaptec1.gr out_1_00.txt 0 >> check.txt
echo "" >> check.txt
echo "" >> check.txt

echo "1_01" >> check.txt
(time ./ROUTE.exe -d=0 -n=1 benchmarks/adaptec1.gr out_1_01.txt) &>> check.txt
./556_eval.exe benchmarks/adaptec1.gr out_1_01.txt 0 >> check.txt
echo "" >> check.txt
echo "" >> check.txt

echo "1_10" >> check.txt
(time ./ROUTE.exe -d=1 -n=0 benchmarks/adaptec1.gr out_1_10.txt) &>> check.txt
./556_eval.exe benchmarks/adaptec1.gr out_1_10.txt 0 >> check.txt
echo "" >> check.txt
echo "" >> check.txt

echo "1_11" >> check.txt
(time ./ROUTE.exe -d=1 -n=1 benchmarks/adaptec1.gr out_1_11.txt) &>> check.txt
./556_eval.exe benchmarks/adaptec1.gr out_1_11.txt 0 >> check.txt
echo "" >> check.txt
echo "" >> check.txt

echo "2_00" >> check.txt
(time ./ROUTE.exe -d=0 -n=0 benchmarks/adaptec2.gr out_2_00.txt) &>> check.txt
./556_eval.exe benchmarks/adaptec2.gr out_2_00.txt 0 >> check.txt
echo "" >> check.txt
echo "" >> check.txt

echo "2_01" >> check.txt
(time ./ROUTE.exe -d=0 -n=1 benchmarks/adaptec2.gr out_2_01.txt) &>> check.txt
./556_eval.exe benchmarks/adaptec2.gr out_2_01.txt 0 >> check.txt
echo "" >> check.txt
echo "" >> check.txt

echo "2_10" >> check.txt
(time ./ROUTE.exe -d=1 -n=0 benchmarks/adaptec2.gr out_2_10.txt) &>> check.txt
./556_eval.exe benchmarks/adaptec2.gr out_2_10.txt 0 >> check.txt
echo "" >> check.txt
echo "" >> check.txt

echo "2_11" >> check.txt
(time ./ROUTE.exe -d=1 -n=1 benchmarks/adaptec2.gr out_2_11.txt) &>> check.txt
./556_eval.exe benchmarks/adaptec2.gr out_2_11.txt 0 >> check.txt
echo "" >> check.txt
echo "" >> check.txt

echo "3_00" >> check.txt
(time ./ROUTE.exe -d=0 -n=0 benchmarks/adaptec3.gr out_3_00.txt) &>> check.txt
./556_eval.exe benchmarks/adaptec3.gr out_3_00.txt 0 >> check.txt
echo "" >> check.txt
echo "" >> check.txt

echo "3_01" >> check.txt
(time ./ROUTE.exe -d=0 -n=1 benchmarks/adaptec3.gr out_3_01.txt) &>> check.txt
./556_eval.exe benchmarks/adaptec3.gr out_3_01.txt 0 >> check.txt
echo "" >> check.txt
echo "" >> check.txt

echo "3_10" >> check.txt
(time ./ROUTE.exe -d=1 -n=0 benchmarks/adaptec3.gr out_3_10.txt) &>> check.txt
./556_eval.exe benchmarks/adaptec3.gr out_3_10.txt 0 >> check.txt
echo "" >> check.txt
echo "" >> check.txt

echo "3_11" >> check.txt
(time ./ROUTE.exe -d=1 -n=1 benchmarks/adaptec3.gr out_3_11.txt) &>> check.txt
./556_eval.exe benchmarks/adaptec3.gr out_3_11.txt 0 >> check.txt
echo "" >> check.txt
echo "" >> check.txt