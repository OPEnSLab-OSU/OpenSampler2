#ifndef UNIT_TEST
#include <Application.hpp>
#include <KPApplicationRuntime.hpp>

Application app;

void setup() {
    Runtime::setInitialAppController(app);
}

void loop() {
    Runtime::update();
}
#endif