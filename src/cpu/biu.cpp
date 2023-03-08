#include "biu.h"
#include <src/bus/Bus.h>
#include <src/cpu/cpu.h>

#include <cstdio>
#include <cstdlib>

const char *BusInterfaceUnit::TranslateBusState(BusStatus status)
{
	switch (status)
	{
	case PASSIVE:
		return "PASV";
	case CODE_FETCH:
		return "CODE";
	}
}

void BusInterfaceUnit::AttachCPU(CPU8088 *cpu)
{
	ptr = cpu;
}

void BusInterfaceUnit::SuspendFetch()
{
	fetch_suspended = true;
}

void BusInterfaceUnit::FlushQueue()
{
	std::queue<uint8_t> empty;
	q.swap(empty);

	state = State::T_IDLE;
	fetch_suspended = false;
}

void BusInterfaceUnit::Tick()
{
	printf("%s\t", TranslateBusState(status));
	
	if (fetch_suspended)
	{
		printf("Suspended       ");
		return;
	}

	if (!fetch_scheduled && q.size() < 4 && fetch_state == FetchState::IDLE)
	{
		printf("Scheduling      ");
		fetch_state = FetchState::Scheduled0;
		state = State::T1;
		fetch_scheduled = true;
		return;
	}

	switch (fetch_state)
	{
	case FetchState::IDLE:
		break;
	case FetchState::Scheduled0:
		printf("Scheduled(0)    ");
		fetch_state = FetchState::Scheduled1;
		break;
	case FetchState::Scheduled1:
		printf("Scheduled(1)    ");
		fetch_state = FetchState::Scheduled2;
		status = BusStatus::CODE_FETCH;
		break;
	case FetchState::Scheduled2:
	{
		printf("Scheduled(2) T%d ", (int)state);
		switch (state)
		{
		case State::T1:
			state = State::T2;
			break;
		case State::T2:
			state = State::T3;
			status = BusStatus::PASSIVE;
			break;
		case State::T3:
			state = State::T4;
			break;
		case State::T4:
			status = BusStatus::PASSIVE;
			q.push(Bus::Read8(ptr->TranslateAddr(ptr->ip++, ptr->segments[1])));
			state = State::T_IDLE;
			fetch_state = FetchState::IDLE;
			fetch_scheduled = false;
			break;
		default:
			state = State::T1;
			break;
		}
		break;
	}
	default:
		printf("[emu/8088]: Unknown state %d\n", (int)fetch_state);
		exit(1);
	}
}

bool BusInterfaceUnit::Read8Queue(uint8_t &data)
{
	if (q.size() != 0)
	{
		data = q.front();
		q.pop();
		return true;
	}
	

	return false;
}
