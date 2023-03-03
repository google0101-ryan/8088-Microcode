#pragma once

#include <cstdint>
#include <string>

namespace Bus
{
void Init(std::string bios_u18, std::string bios_u19);

uint8_t Read8(uint32_t addr);

void Write16(uint32_t addr, uint16_t data);

void IoWrite8(uint16_t port, uint8_t data);
};