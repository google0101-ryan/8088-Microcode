#pragma once

#include <cstdint>
#include <queue>

class CPU8088;

class BusInterfaceUnit
{
private:
	std::queue<uint8_t> q;

	enum State
	{
		T_IDLE,
		T1,
		T2,
		T3,
		T4
	} state = State::T1;

	enum BusStatus
	{
		PASSIVE,
		CODE_FETCH
	} status = BusStatus::PASSIVE;

	enum FetchState
	{
		IDLE,
		Scheduled0,
		Scheduled1,
		Scheduled2,
	} fetch_state = FetchState::IDLE;

	bool fetch_scheduled = false;
	bool fetch_suspended = true;
	
	CPU8088* ptr;

	const char* TranslateBusState(BusStatus status);
public:
	void AttachCPU(CPU8088* cpu);

	void SuspendFetch();
	void FlushQueue();

	void Tick();

	bool Read8Queue(uint8_t& data);
};