SYSTEM     = x86-64_sles10_4.1
LIBFORMAT  = static_pic

# ---------------------------------------------------------------------         
# Compiler selection                                                            
# ---------------------------------------------------------------------         

CCC = g++

# ---------------------------------------------------------------------         
# Compiler options                                                              
# ---------------------------------------------------------------------         

CCOPT = -m64 -O -fPIC -fexceptions -DNDEBUG -DIL_STD -g -Wall

# ---------------------------------------------------------------------         
# Link options and libraries                                                    
# ---------------------------------------------------------------------         

CCFLAGS = $(CCOPT) 
CCLNFLAGS = -lm -pthread 

# --------------------------------------------------------
CCFILES = main.o ece556.o printStuff.o
EXTRAFILES = printStuff.cpp
# --------------------------------------------------------

#------------------------------------------------------------                   
#  make all      : to compile.                                     
#  make execute  : to compile and execute.                         
#------------------------------------------------------------    

ROUTE.exe: ece556.o printStuff.o main.o
	/bin/rm -f ROUTE.exe
	$(CCC) $(LINKFLAGS) $(CCFLAGS) $(CCFILES) $(CCLNFLAGS) -o ROUTE.exe

main.o: main.cpp ece556.h printStuff.cpp printStuff.h
	/bin/rm -f main.o
	$(CCC) $(CCFLAGS) main.cpp $(EXTRAFILES) -c

ece556.o: ece556.cpp ece556.h printStuff.cpp printStuff.h
	/bin/rm -f ece556.o
	$(CCC) $(CCFLAGS) ece556.cpp $(EXTRAFILES) -c

printStuff.o: printStuff.cpp printStuff.h
	/bin/rm -f printStuff.o
	$(CCC) $(CCFLAGS) printStuff.cpp -c

clean:
	/bin/rm -f *~ *.o ROUTE.exe 
