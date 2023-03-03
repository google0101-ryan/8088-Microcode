#include "Bus.h"

#include <fstream>

uint8_t u18[0x8000];
uint8_t u19[0x2000];
uint8_t* ram;

void Bus::Init(std::string bios_u18, std::string bios_u19)
{
	ram = new uint8_t[0xA0000];

	std::ifstream file(bios_u18);
	file.seekg(0, std::ios::beg);
	file.read((char*)u18, 0x8000);
	file.close();

	file.open(bios_u19);
	file.seekg(0, std::ios::beg);
	file.read((char*)u19, 0x2000);
}

uint8_t Bus::Read8(uint32_t addr)
{
	if (addr >= 0xF8000 && addr < 0xFFFFF)
		return u18[addr - 0xF8000];
	
	printf("Read8 from unknown addr 0x%08x\n", addr);
	exit(1);
}

void Bus::Write16(uint32_t addr, uint16_t data)
{
	if (addr < 0xA0000)
	{
		ram[addr] = data;
		return;
	}
	
	printf("Write16 to unknown addr 0x%08x\n", addr);
	exit(1);
}

void Bus::IoWrite8(uint16_t port, uint8_t data)
{
	printf("Write8 to unknown port 0x%04x\n", port);
	exit(1);
}
