make

# Run with valgrind
# valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./ROUTE.exe in1.txt potentialOut.txt

# Run without valgrind
./ROUTE.exe in1.txt potentialOut.txt 

# Run gdb
# gdb --args ./ROUTE.exe in1.txt potentialOut.txt