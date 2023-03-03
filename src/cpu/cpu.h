#pragma once

#include <cstdint>
#include <queue>

class CPU8088
{
private:
	std::queue<uint8_t> q;

	int ucode_ip;
	const uint32_t* ucode_ptr;
	uint16_t ip;
	uint16_t prefetch_pointer;
	uint8_t cur_opcode;
	bool is_word = false;

	void write_m(uint16_t data);

	uint16_t ds, cs, es, ss;
	uint16_t flags;
	uint16_t* prefix = &ds;

	int target_reg = 0;

	uint16_t tmpb, tmpa;

	uint32_t TranslateAddr(uint32_t addr, uint16_t prefix)
	{
		return ((prefix << 4) + addr) & 0xFFFFF;
	}
public:
	CPU8088();

	void Clock();
	void Dump();
};

void Dump();