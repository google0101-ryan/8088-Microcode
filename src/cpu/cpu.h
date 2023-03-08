#pragma once

#include <cstdint>
#include <queue>

#include "biu.h"

class CPU8088
{
	friend class BusInterfaceUnit;
private:
	int ucode_ip;
	uint8_t cur_opcode;
	bool has_cur_opcode = false;
	bool waiting_on_modrm = false;

	BusInterfaceUnit biu;

	uint16_t segments[6];

	uint16_t ip = 0;
	uint16_t flags = 0;
	uint16_t tmpb, tmpa, sigma;

	bool is_word_targeted = false;
	bool is_memory = false;

	void WriteM(uint16_t data);
	uint16_t ReadM();

	uint8_t ReadReg8(int reg);

	void WriteReg8(int reg, uint8_t data);

	bool TranslateModrm();

	union
	{
		uint8_t value;
		struct
		{
			uint8_t rm : 3;
			uint8_t reg : 3;
			uint8_t mod : 2;
		};
	} modrm;

	enum REGISTERS
	{
		AX,
		CX,
		DX,
		BX,
		SP,
		BP,
		SI,
		DI
	};

	union Register
	{
		uint16_t reg16;
		struct
		{
			uint8_t reg8_lo;
			uint8_t reg8_hi;
		};
	} regs[8];

	uint16_t target_reg_addr = 0;

	enum SEGMENTS
	{
		ES,
		CS,
		SS,
		DS,
		FS,
		GS
	};

	bool PassedCond(uint8_t cond);

	uint32_t TranslateAddr(uint32_t addr, uint16_t prefix)
	{
		return ((prefix << 4) + addr) & 0xFFFFF;
	}

	void RunMicroOp(uint32_t micro_op);

	uint16_t FetchMicroRegister(int reg, bool& cont);
	void WriteMicroRegister(int reg, uint16_t data);
public:
	CPU8088();

	void Clock();
	void Dump();
};

void Dump();