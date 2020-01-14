#ifndef UNIT_TEST
#include <KPApplicationRuntime.hpp>
#include <Application.hpp>

Application app;

void setup() {
	Runtime::setInitialAppController(app);
}

void loop() {
	Runtime::update();
}
#endif