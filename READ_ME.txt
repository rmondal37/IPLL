To solve this assignment I have used g++ on Ubuntu Linux v21.10.

Input file format:

* Label field : 10 characters long
* Opcode field : 10 characters long
* Operand field : >= 30 characters long

Following are the steps for compiling the code:

1) g++ 190101072_Assign02_assembler.cpp -o assembler
2) ./assembler input.txt
3) g++ 190101072_Assign02_loader.cpp -o loader
4) ./loader


Input files necessary for running:

* input.txt : Contains the input assembly language program
* loaderInput.txt : Contains the input to the 2 pass linking loader program. It must be noted 
                    that it is just the output of the assembler program, but carets are removed.


3 output files are created:

* intermediate.txt : Intermediate file made by Pass 1 of assembler
* assemblerOutput.txt: Output of the assembler
* loaderOutput.txt: Output of the loader