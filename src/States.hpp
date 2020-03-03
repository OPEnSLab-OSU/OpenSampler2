#pragma once

// Application Logic
#include <KPState.hpp>

namespace StateName {
	constexpr const char * idle	  = "idle-state";
	constexpr const char * stop	  = "stop-state";
	constexpr const char * flush  = "flush-state";
	constexpr const char * clean  = "clean-state";
	constexpr const char * sample = "sample-state";
};	// namespace StateName

class StateIdle : public KPState {
public:
	void enter(KPStateMachine & sm) override;
};

class StateStop : public KPState {
public:
	void enter(KPStateMachine & sm) override;
};

class StateClean : public KPState {
public:
	void enter(KPStateMachine & sm) override;
};

class StateSample : public KPState {
public:
	int time = 0;
	void enter(KPStateMachine & sm) override;
};

class StateFlush : public KPState {
public:
	int time = 0;
	void enter(KPStateMachine & sm) override;
};