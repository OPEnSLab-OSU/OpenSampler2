class KPTestApplication;

#include <unity.h>

#include <Application.hpp>
#include <KPApplicationRuntime.hpp>

Application app;

void TestStateMachine() {
	KPStateMachine & sm = app.sm;
	auto flush = sm.getState<StateFlush>(StateName::flush);
	flush->time = 3;

	auto sample = sm.getState<StateSample>(StateName::sample);
	sample->time = 3;
	
	app.sm.transitionTo(flush->getName());
}

void setup() {
    delay(7000);
    UNITY_BEGIN();
	Runtime::setInitialAppController(app);
    RUN_TEST(TestStateMachine);
}

void loop() {
	Runtime::update();
}