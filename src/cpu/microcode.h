#pragma once

#include <cstdint>

static constexpr uint32_t ucode_reset[] =
{
	0b110001011101001111011, // Set DS to 0, suspend prefetcher
	0b100001011001001111111, // Set CS to 0xffff
	0b001001011101000001111, // Set PC to 0, flush prefetch queue
	0b111101011101001111111, // Set flags to 0
	0b000001011101001111111, // Set ES to 0
	0b010001011101001111000 // Set SS to 0, Run Next Instruction
};

static constexpr uint32_t ucode_mov_r_i[] =
{
	// Move the next prefetch operation to the bottom half of tmpb. 
	// If the instruction is 8-bit, skip the next instruction
	0b101010011100000100010, // Q -> tmpbL L8 2
	// If the instruction is 16-bit, fetch the next value from the prefetcher
	0b111010011101001111111, // Q -> tmpbH
	// Write the result to the appropriate register/memory address. Run next instruction
	0b010010111001001111000, // tmpb -> M
};

static constexpr uint32_t ucode_sahf[] =
{
	0b111001011001001111111, // Do nothing, not sure why this is here
	// Grab the current value of the flags register
	0b001100111101001111111, // F -> tmpa
	// Replace the bottom half with AH
	0b001011000001001111101, // X -> tmpaL
	// Move that value into the flags register. Run next instruction
	0b111100101001001111000 // tmpa -> F RNI
};

static constexpr uint32_t ucode_jmp_far[] =
{
	0b101010011101001111111,
	0b111010011101001111111,
	0b001010011101001111111,
	0b011010011101001111111,
	0b111001011001001111011,
	0b001000111001001111111,
	0b100000101001000001000
};

static constexpr uint32_t ucode_cond_jmp[] =
{
	
};