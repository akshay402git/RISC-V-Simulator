CXX := g++
CXXFLAGS :=  -std=c++17

riscv_asm: CLab3.cc input.s
	$(CXX) $(CXXFLAGS) $< -o $@
	#./$@
	
clean: riscv_asm
	rm riscv_asm
	rm output.hex

