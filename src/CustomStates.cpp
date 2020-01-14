#include <Application.hpp>

void StateIdle::enter(KPStateMachine & sm) {
}

void StateStop::enter(KPStateMachine & sm) {
    auto _pump  = sm.controller->getComponent<Pump>("pump");
    auto _shift = sm.controller->getComponent<ShiftRegister>("shift-register");
    if (!_pump || !_shift) {
        raise(Error("Missing components"));
    }

    Application & app = *static_cast<Application *>(sm.controller);
	
    app.pump.off();
    app.shift.writeZeros();
    app.shift.writeLatchOut();

    app.status().setValveStatus(app.status().valveCurrent, ValveStatus::sampled);
    app.status().setCurrentValveStatus(ValveStatus::unselected);
    sm.transitionTo(StateName::idle);
}

void StateSample::enter(KPStateMachine & sm) {
	if (time == 0 || time > 15) {
        println("Invalid state configuration");
        return;
    }

    auto _pump  = sm.controller->getComponent<Pump>("pump");
    auto _shift = sm.controller->getComponent<ShiftRegister>("shift-register");
    if (!_pump || !_shift) {
        raise(Error("Missing components"));
    }

    Application & app = *static_cast<Application *>(sm.controller);

    int r = app.shift.toRegisterIndex(app.status().valveCurrent) + 1;
    int b = app.shift.toBitIndex(app.status().valveCurrent);
    app.pump.off();
    app.shift.setZeros();
    app.shift.setRegister(r, b, HIGH);
    app.shift.flush();
    app.pump.on();

	// Flush the main pipe with air
	setTimeCondition(secsToMillis(time), [&]() {
		app.shift.setRegister(r, b, LOW);
		app.shift.flush();
		app.pump.on(Direction::reverse);
	});

	// Transition to stop state
	setTimeCondition(secsToMillis(time + 5), [&]() {
        sm.transitionTo(StateName::stop);
    });
}

void StateFlush::enter(KPStateMachine & sm) {
    if (time == 0) {
        println("Invalid state configuration");
        return;
    }

    auto _pump  = sm.controller->getComponent<Pump>("pump");
    auto _shift = sm.controller->getComponent<ShiftRegister>("shift-register");
    if (!_pump || !_shift) {
        raise(Error("Missing components"));
    }

    Application & app = *static_cast<Application *>(sm.controller);

    // Flush main pipe
    auto flushMainPipe = [&]() {
        app.pump.off();
        app.shift.writeZeros();
        app.shift.setBit(FlushValveBitIndex, HIGH);
        app.shift.flush();
        app.pump.on();
    };

    // Flush water fron connecting tube
    // Flush valve off - Current Bag valve ON - Motor reverse	(fixed 15 seconds)
    // (purpose: take the last bit of old water from connected tubing out)
    auto flushWaterFromConnectingTube = [&]() {
        int r = app.shift.toRegisterIndex(app.status().valveCurrent) + 1;
        int b = app.shift.toBitIndex(app.status().valveCurrent);

        app.pump.off();
        app.shift.setZeros();
        app.shift.setBit(FlushValveBitIndex, LOW);
        app.shift.setRegister(r, b, HIGH);
        app.shift.flush();
        app.pump.on(Direction::reverse);
    };

    setTimeCondition(0, flushMainPipe);
    setTimeCondition(secsToMillis(time), flushWaterFromConnectingTube);
    setTimeCondition(secsToMillis(time + 15), flushMainPipe);
    setTimeCondition(secsToMillis(time + 15 + time), [&]() {
        sm.transitionTo(StateName::sample);
    });
}
