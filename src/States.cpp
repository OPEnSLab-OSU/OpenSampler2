#include <Application.hpp>

//
// ─── SECTION IDLE STATE ─────────────────────────────────────────────────────────
//

void StateIdle::enter(KPStateMachine & sm) {}

//
// ─── SECTION ENTER STATE ────────────────────────────────────────────────────────
//

void StateStop::enter(KPStateMachine & sm) {
	Application & app = *static_cast<Application *>(sm.controller);

	app.pump.off();
	app.shift.writeZeros();
	app.shift.writeLatchOut();

	app.status().setValveStatus(app.status().valveCurrent, ValveStatus::sampled);
	app.status().setCurrentValveStatus(ValveStatus::unselected);
	sm.transitionTo(StateName::idle);
}

//
// ─── SECTION CLEAN STATE ────────────────────────────────────────────────────────
//

void StateClean::enter(KPStateMachine & sm) {
	Application & app = *static_cast<Application *>(sm.controller);

	int r = app.shift.toRegisterIndex(app.status().valveCurrent) + 1;
	int b = app.shift.toBitIndex(app.status().valveCurrent);

	// Flush the main pipe with air
	app.shift.setRegister(r, b, LOW);
	app.shift.setBit(FlushValveBitIndex, LOW);
	app.shift.flush();
	app.pump.on(Direction::reverse);

	// NOTE:
	// Be careful with the captures in lambdas,
	// make sure that local variables are captured by copies)
	setTimeCondition(secsToMillis(5), [&, r, b]() {
		app.shift.setRegister(r, b, HIGH);
		app.shift.flush();
		app.pump.on();
	});

	// Transition to stop state
	setTimeCondition(secsToMillis(5 + 1), [&]() {
		sm.transitionTo(StateName::stop);
	});
}

//
// ─── SECTION SAMPLE STATE ───────────────────────────────────────────────────────
//

void StateSample::enter(KPStateMachine & sm) {
	if (time == 0 || time > 15) {
		println("Invalid state configuration");
		return;
	}

	Application & app = *static_cast<Application *>(sm.controller);

	int r = app.shift.toRegisterIndex(app.status().valveCurrent) + 1;
	int b = app.shift.toBitIndex(app.status().valveCurrent);
	app.pump.off();
	app.shift.setZeros();
	app.shift.setRegister(r, b, HIGH);
	app.shift.flush();
	app.pump.on();

	setTimeCondition(secsToMillis(time), [&]() {
		sm.transitionTo(StateName::clean);
	});
}

//
// ─── SECTION FLUSH STATE ────────────────────────────────────────────────────────
//

void StateFlush::enter(KPStateMachine & sm) {
	if (time == 0) {
		println("Invalid state configuration");
		return;
	}

	auto _pump	= sm.controller->getComponent<Pump>("pump");
	auto _shift = sm.controller->getComponent<ShiftRegister>("shift-register");
	if (!_pump || !_shift) {
		raise("Missing components");
	}

	Application & app = *static_cast<Application *>(sm.controller);

	// Flush main pipe
	auto flush = [&]() {
		app.pump.off();
		app.shift.writeZeros();
		app.shift.setBit(FlushValveBitIndex, HIGH);
		app.shift.flush();
		app.pump.on();
	};

	// Flush water fron connecting tube
	// Flush valve off - Current Bag valve ON - Motor reverse	(fixed 15 seconds)
	// (purpose: take the last bit of old water from connected tubing out)
	auto vaccuum = [&]() {
		int r = app.shift.toRegisterIndex(app.status().valveCurrent) + 1;
		int b = app.shift.toBitIndex(app.status().valveCurrent);

		app.pump.off();
		app.shift.setZeros();
		app.shift.setBit(FlushValveBitIndex, LOW);
		app.shift.setRegister(r, b, HIGH);
		app.shift.flush();
		app.pump.on(Direction::reverse);
	};

	setTimeCondition(0, flush);
	setTimeCondition(secsToMillis(time), vaccuum);
	setTimeCondition(secsToMillis(time + 15), flush);
	setTimeCondition(secsToMillis(time + 15 + time), [&]() {
		sm.transitionTo(StateName::sample);
	});
}
