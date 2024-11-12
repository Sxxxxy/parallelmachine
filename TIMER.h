#pragma once
#include<chrono>
#include<vector>

typedef std::chrono::steady_clock::time_point TIME_POINT;
typedef std::chrono::steady_clock STEADY_CLOCK;
typedef std::chrono::system_clock SYS_CLOCK;

enum class RETURN_TYPE
{
    OK=0, TIME_OUT=1, ERR=2
};

enum class TimeType
{
	ms = 0,
	s = 1,
	m = 2
};

class TIMER
{
public:
	static void Start(double TimeLimit_s);		//seconds
	static int ClickMs();
	static bool isStop();
	static double GetRemainingTime();
private:
	static std::chrono::steady_clock::time_point _start;
	static int msTimeLimit;
};

