#include "TIMER.h"

TIME_POINT TIMER::_start;
int TIMER::msTimeLimit;

void TIMER::Start(double TimeLimit_s)
{
	_start = std::chrono::steady_clock::now();
	msTimeLimit = TimeLimit_s * 1000;
}

int TIMER::ClickMs()
{
	auto currTime =STEADY_CLOCK::now();
	
	auto last = std::chrono::duration_cast<std::chrono::milliseconds> (currTime - _start);
	return last.count();	
}

double TIMER::GetRemainingTime()
{
	return (msTimeLimit - ClickMs()) / 1000.0;
}

bool TIMER::isStop()
{
	return ClickMs() >= msTimeLimit;
}
