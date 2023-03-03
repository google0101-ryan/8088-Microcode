#include "cpu.h"
#include "microcode.h"
#include "../bus/Bus.h"

#include <cstdio>
#include <cstdlib>

uint8_t code[] =
{
	0xb4, 0xd5, // MOV AH, 0xD5
	0x9e, // SAHF
}; // A flat array for now

bool prefetch_scheduled = false;
int cycles_till_prefetch = 0;
int delay_cycles = 0;
bool prefetch_paused = true;
bool opcode_fetched = true;
bool is_memory = false;
bool ready = true; // The ready pin, if it's not true, we go to W-state and start waiting. Used by DMA and CGA

union
{
	uint16_t value;
	struct
	{
		uint8_t lo, hi;
	};
} regs[8];

void write_reg8(int reg, uint8_t data)
{
	if (reg < 4)
		regs[reg].lo = data;
	else
		regs[reg - 4].hi = data;
}

void CPU8088::write_m(uint16_t data)
{
	if (is_memory || is_word)
	{
		exit(1);
	}
	else
	{
		if (is_word)
			;
		else
			write_reg8(target_reg, data);
	}
}

int t_state = 1;
int cycles = 0;

CPU8088::CPU8088()
{
	ucode_ptr = ucode_reset;
	ucode_ip = 0;
}

void CPU8088::Clock()
{
	if (!ready)
	{
		t_state = 5;
		printf("W: Waiting on ready pin\n");
		return;
	}

	printf("%d (%05d): ", t_state, cycles++);

cpu_step:
	if (!prefetch_scheduled && !prefetch_paused)
	{
		prefetch_scheduled = true;
		cycles_till_prefetch = 1;
		printf("Scheduled          ");
	}
	else if (prefetch_scheduled)
	{
		if (cycles_till_prefetch != 0)
		{
			printf("Scheduled(%d)       ", 1 - cycles_till_prefetch);
			cycles_till_prefetch--;
			t_state++;
			if (cycles_till_prefetch == 0)
			{
				delay_cycles = 2;
			}
		}
		else if (cycles_till_prefetch == 0)
		{
			printf("FETCHING(%d)        ", 2 - delay_cycles);
			if (delay_cycles)
			{
				t_state++;
				delay_cycles--;
			}
			else if (q.size() < 4)
			{
				q.push(Bus::Read8(TranslateAddr(ip++, cs)));
				printf("\b\b\b\b\b\b0x%02x  ", q.front());
				prefetch_scheduled = false;
				prefetch_paused = false;
				t_state = 1;
			}
			else
			{
				t_state = 1;
				prefetch_paused = true;
			}
		}
	}
	else if (prefetch_paused)
	{
		printf("IDLE               ");
	}

	if (!opcode_fetched)
	{
		if (q.size() == 0)
		{
			printf("[8088]: Waiting on prefetch queue\n");
			return;
		}

		opcode_fetched = true;
		cur_opcode = q.front();
		q.pop();

		ucode_ip = 0;

		switch (cur_opcode)
		{
		case 0x9e:
			ucode_ptr = ucode_sahf;
			break;
		case 0xb0:
		case 0xb1:
		case 0xb2:
		case 0xb3:
		case 0xb4:
		case 0xb5:
		case 0xb6:
		case 0xb7:
		case 0xb8:
			target_reg = cur_opcode - 0xb0;
			ucode_ptr = ucode_mov_r_i;
			is_word = false;
			break;
		case 0xea:
			ucode_ptr = ucode_jmp_far;
			break;
		case 0xfa:
			flags &= ~0x200;
			cur_opcode = 0;
			opcode_fetched = false;
			printf("cli\n");
			return;
		default:
			printf("[8088]: Unknown opcode 0x%02x\n", cur_opcode);
			exit(1);
		}
	}

	uint32_t micro_op = ucode_ptr[ucode_ip++];
	int s = ((micro_op >> 13) & 1) + ((micro_op >> 10) & 6) + ((micro_op >> 11) & 0x18);
    int dd = ((micro_op >> 20) & 1) + ((micro_op >> 18) & 2) + ((micro_op >> 16) & 4) + ((micro_op >> 14) & 8) + ((micro_op >> 12) & 0x10);

	uint16_t data;

	switch (s)
	{
	case 0x05:
		printf("IND   ->");
		break;
	case 0x07:
		if (!q.size())
		{
			printf("Stalling for prefetch queue\n");
			ucode_ip--;
			return;
		}
		data = q.front();
		q.pop();
		printf("Q    -> ");
		break;
	case 0x0c:
		printf("tmpa -> ");
		data = tmpa;
		break;
	case 0x0d:
		printf("tmpb -> ");
		data = tmpb;
		break;
	case 0x0f:
		printf("F    -> ");
		data = flags;
		break;
	case 0x10:
		printf("X    -> ");
		data = regs[0].hi;
		break;
	case 0x15:
		printf("ONES -> ");
		data = 0xffff;
		break;
	case 0x17:
		printf("ZERO -> ");
		data = 0;
		break;
	default:
		printf("[8088]: Unknown source register %d %x\n", s, s);
		exit(1);
	}

	switch (dd)
	{
	case 0:
		printf("RA    ");
		es = data;
		break;
	case 1:
		printf("RC    ");
		cs = data;
		break;
	case 2:
		printf("RS    ");
		ss = data;
		break;
	case 3:
		printf("RD    ");
		ds = data;
		break;
	case 4:
		ip = data;
		printf("PC    ");
		break;
	case 7:
		printf("\b\b\b\b\b\b\b\b           ");
		break;
	case 12:
		tmpa = data;
		printf("tmpa  ");
		break;
	case 15:
		flags = data;
		flags |= 2; // Bit 2 is always set
		printf("F     ");
		break;
	case 18:
		write_m(data);
		printf("M     ");
		break;
	case 20:
		tmpa &= ~0xFF;
		tmpa |= data;
		printf("tmpaL ");
		break;
	case 21:
		tmpb &= ~0xFF;
		tmpb |= data;
		printf("tmpbL ");
		break;
	case 22:
		tmpa &= 0xFF;
		tmpa |= (data << 8);
		printf("tmpaH ");
		break;
	case 23:
		tmpb &= 0xFF;
		tmpb |= (data << 8);
		printf("tmpbH ");
		break;
	default:
		printf("[8088]: Unknown dest register %d\n", dd);
		exit(1);
	}
	
	if (dd != 7)
		printf("0x%02x\t", data);
	else
		printf("          \t");

	int type = (micro_op >> 7) & 7;

	if ((type & 4) == 0)
		type >>= 1;

	switch (type)
	{
	case 0:
	case 5:
	case 7:
		switch ((micro_op >> 4) & 0xf)
		{
		case 0x02:
			printf("L8   \t");
			if (!is_word)
				ucode_ip += 1; // Skip the next instruction
			break;
		default:
			printf("[8088]: Unknown u-op sub-op type 0 %d\n", (micro_op >> 4) & 0xf);
			exit(1);
		}

		if (type == 5)
		{
			printf("Type 5\n");
			exit(1);
		}
		else
			printf("\n");
		break;
	case 4:
		switch ((micro_op >> 3) & 0xf)
		{
		case 0x01:
		{
			printf("FLUSH\t");
			std::queue<uint8_t> empty;
			q.swap(empty);
			prefetch_paused = false;
			break;
		}
		case 0x0f:
			printf("NONE \t");
			break; // None			
		default:
			printf("[8088]: Unknown u-op sub-op type %d\n", (micro_op >> 3) & 0xf);
			exit(1);
		}

		switch (micro_op & 7)
		{
		case 0:
			printf("RNI\n");
			cur_opcode = 0;
			opcode_fetched = false;
			break;
		case 3:
			printf("SUSP\n");
			prefetch_paused = true;
			break;
		case 5:
			printf("NX\n           ");
			goto cpu_step;
			break;
		case 7:
			printf("NONE\n");
			break;
		default:
			printf("[8088]: Unknown u-op sub-op2 type %d\n", micro_op & 7);
			exit(1);
		}
		break;
	default:
		printf("[8088]: Unknown u-op type %d\n", type);
		exit(1);
	}
}

void CPU8088::Dump()
{
	printf("flags\t->\t0x%04x\n", flags);
	printf("0x%04x:0x%04x\n", cs, ip);
}

extern CPU8088* cpu;

void Dump()
{
	for (int i = 0; i < 8; i++)
		printf("r%d\t->\t0x%04x\n", i, regs[i].value);
	cpu->Dump();
}
