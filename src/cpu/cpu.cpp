#include "cpu.h"
#include "microcode.h"

#include <cstdio>
#include <cstdlib>
#include <string>

void CPU8088::WriteM(uint16_t data)
{
	if (is_memory)
	{
		printf("[emu/8088]: Unhandled write to M -> memory\n");
		exit(1);
	}
	else if (is_word_targeted)
	{
		printf("[emu/8088]: Unhandled write to M -> 16-bit register\n");
		exit(1);
	}
	else if (!is_word_targeted)
	{
		WriteReg8(target_reg_addr, data);
	}
}

uint16_t CPU8088::ReadM()
{
	if (is_word_targeted)
	{
		printf("[emu/8088]: Unhandled 16-bit M read\n");
		exit(1);
	}
	else
	{
		switch (modrm.mod)
		{
		case 3:
			return ReadReg8(modrm.rm);
		}
	}
}

uint8_t CPU8088::ReadReg8(int reg)
{
	if (reg < 4)
		return regs[reg].reg8_lo;
	else
		return regs[reg - 4].reg8_hi;
}

void CPU8088::WriteReg8(int reg, uint8_t data)
{
	if (reg < 4)
		regs[reg].reg8_lo = data;
	else
		regs[reg - 4].reg8_hi = data;
}

// Returns whether we should run a microcode for modr/m or not
bool CPU8088::TranslateModrm()
{
	switch (modrm.mod)
	{
	case 3:
		return false;
	default:
		printf("[emu/8088]: Unknown modr/m mode %d\n", modrm.mod);
		exit(1);
	}
}

bool CPU8088::PassedCond(uint8_t cond)
{
	bool condPassed = false;

	switch (cond & 0xE)
	{
	case 2:
		condPassed = (flags >> 0) & 1;
		break;
	default:
		printf("[emu/8088]: Unknown cond %d\n", cond);
		exit(1);
	}

	if (!(cond & 1))
		condPassed = !condPassed;
	
	return condPassed;
}

void CPU8088::RunMicroOp(uint32_t d)
{
	printf("0x%06x: ", d);

	int s = ((d >> 13) & 1) + ((d >> 10) & 6) + ((d >> 11) & 0x18);
    int dd = ((d >> 20) & 1) + ((d >> 18) & 2) + ((d >> 16) & 4) + ((d >> 14) & 8) + ((d >> 12) & 0x10);
    int typ = (d >> 7) & 7;
    if ((typ & 4) == 0)
        typ >>= 1;

	// Step one, do a basic move operation between a source and a destination

	bool cont;
	uint16_t source = FetchMicroRegister(s, cont);

	if (!cont)
		return;

	WriteMicroRegister(dd, source);

	// Step two, do some type-specific operations

	switch (typ)
	{
	default:
		printf("[emu/8088]: Unknown type %d\n", typ);
		exit(1);
	}
}

uint16_t CPU8088::FetchMicroRegister(int reg, bool& cont)
{
	cont = true;

	switch (reg)
	{
	case 7:
	{
		uint8_t ret;
		if (!biu.Read8Queue(ret))
		{
			printf("Stalling on read from Q\n");
			cont = false;
			ucode_ip--; // Rerun the current ucode until Q is ready
			return 0;
		}
		printf("Q       ->    ");
		return ret;
	}
	case 12:
		printf("tmpa    ->    ");
		return tmpa;
	case 13:
		printf("tmpb    ->    ");
		return tmpb;
	case 15:
		printf("F       ->    ");
		return flags;
	case 16:
		printf("X       ->    ");
		return regs[0].reg8_hi;
	case 18:
		printf("M       ->    ");
		return ReadM();
	case 21:
		printf("ONES    ->    ");
		return 0xffff;
	case 23:
		printf("ZERO    ->    ");
		return 0;
	case 25:
		printf("BC      ->    ");
		return regs[CX].reg16;
	default:
		printf("[emu/8088]: Cannot fetch unknown register %d\n", reg);
		exit(1);
	}
}

void CPU8088::WriteMicroRegister(int reg, uint16_t data)
{
	switch (reg)
	{
	case 0:
		printf("RA   \t");
		segments[ES] = data;
		break;
	case 1:
		printf("RC   \t");
		segments[CS] = data;
		break;
	case 2:
		printf("RS   \t");
		segments[SS] = data;
		break;
	case 3:
		printf("RD   \t");
		segments[DS] = data;
		break;
	case 4:
		printf("PC   \t");
		ip = data;
		break;
	case 7:
		for (int i = 0; i < 14; i++)
			printf("\b");
		printf("                   \t");
		break;
	case 12:
		printf("tmpa \t");
		tmpa = data;
		break;
	case 13:
		printf("tmpb \t");
		tmpb = data;
		break;
	case 15:
		printf("F    \t");
		flags = data | 0x2;
		break;
	case 16:
		printf("X    \t");
		regs[0].reg8_hi = data;
		break;
	case 18:
		printf("M    \t");
		WriteM(data);
		break;
	case 20:
		tmpa &= ~0xFF;
		tmpa |= data;
		printf("tmpaL\t");
		break;
	case 21:
		tmpb &= ~0xFF;
		tmpb |= data;
		printf("tmpbL\t");
		break;
	case 22:
		tmpa &= 0xFF;
		tmpa |= (data << 8);
		printf("tmpaH\t");
		break;
	case 23:
		tmpb &= 0xFF;
		tmpb |= (data << 8);
		printf("tmpbH\t");
		break;
	default:
		printf("[emu/8088]: Cannot write to unknown register %d\n", reg);
		exit(1);
	}
}

CPU8088::CPU8088()
{
	ucode_ip = 0x1E4;
	cur_opcode = 0;
	has_cur_opcode = true; // So we don't skip reset
	biu.AttachCPU(this);
}

int global_cycles = 0;

void CPU8088::Clock()
{
	global_cycles++;
	printf("(%04d): ", global_cycles);

	biu.Tick();

	if (!has_cur_opcode)
	{
		if (!biu.Read8Queue(cur_opcode))
		{
			printf("Waiting on prefetch queue\n");
			return;
		}
		
		has_cur_opcode = true;

		switch (cur_opcode)
		{
		default:
			printf("[emu/8088]: Unknown opcode 0x%02x\n", cur_opcode);
			exit(1);
		}
	}

	uint32_t micro_op = ucode[ucode_ip++];

	RunMicroOp(micro_op);
}

void CPU8088::Dump()
{
	for (int i = 0; i < 8; i++)
		printf("R%d\t->\t0x%04x\n", i, regs[i].reg16);
}

extern CPU8088* cpu;

void Dump()
{
	cpu->Dump();
}
