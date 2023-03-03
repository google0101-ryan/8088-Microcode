#include "src/cpu/cpu.h"
#include "src/bus/Bus.h"

#include <cstdlib>

CPU8088* cpu;

int main()
{
	Bus::Init("data/bios18.bin", "data/bios19.bin");

	cpu = new CPU8088();

	std::atexit(Dump);

	while (1)
		cpu->Clock();
}