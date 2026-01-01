#ifndef TIMER_H
#define TIMER_H

class Timer
{
public:
	Timer();

	void Start();
	void Stop();
	void Reset();
	void Tick();

	float GetDeltaTime() const;
	float GetTotalTime() const;

private:
	double mSecondsPerCount;
	double mDeltaTime;

	__int64 mBaseTime;
	__int64 mPausedTime;
	__int64 mStopTime;
	__int64 mPrevTime;
	__int64 mCurrTime;

	bool mStopped;
};

#endif
