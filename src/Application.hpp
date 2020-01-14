#pragma once

#include <ArduinoJson.h>

#include <Constants.hpp>
#include <CustomStates.hpp>
#include <KPAction.hpp>
#include <KPArray.hpp>
#include <KPController.hpp>
#include <KPSDCard.hpp>
#include <KPSerialInput.hpp>
#include <KPServer.hpp>
#include <Pump.hpp>
#include <ShiftRegister.hpp>
#include <array>

#define AirValveBitIndex     2
#define AlcoholValveBitIndex 3
#define FlushValveBitIndex   5

enum class ValveStatus : int {
    unselected = -1,
    sampled,
    free,
    operating
};

struct Status {
    std::array<int, 24> valves;
    int numberOfAvailableValves = 0;
    int valveCurrent            = 0;
    KPState * stateCurrent      = nullptr;

    const char * logfile = "logs.txt";

    Status() {
        valves.fill(static_cast<int>(ValveStatus::free));
    }

    void setValveStatus(int i, ValveStatus s) {
        valves[i] = static_cast<int>(s);
    }

    void setCurrentValveStatus(ValveStatus s) {
        valveCurrent = static_cast<int>(s);
    }
};

class Application : public KPController, KPSerialInputListener {
private:
    void setupServer() {
        web.serveStaticFile("/", "/index.htm", card);

        web.get("/status", [this](Request & req, Response & res) {
            constexpr const int size = 1000;
            StaticJsonDocument<size> doc;

            using namespace JsonKey;
            doc[stateName]    = sm.getCurrentState()->getName();
            doc[stateIndex]   = sm.getCurrentStateIndex();
            doc[valveCurrent] = status().valveCurrent;

            JsonArray valveArray = doc.createNestedArray(valves);
            copyArray(status().valves.data(), status().valves.size(), valveArray);

            res.setHeader("Content-Type", "application/json");
            res.sendJson<size>(doc);
            res.end();
        });

        web.get("/stop", [this](Request & req, Response & res) {
            constexpr const int size = 1000;
            StaticJsonDocument<size> doc;
            doc["success"] = "Successfully transitioned to stop state";
            res.setHeader("Content-Type", "application/json");
            res.sendJson<size>(doc);
            res.end();

            sm.transitionTo(StateName::stop);
        });

        web.post("/start", [this](Request & req, Response & res) {
            constexpr const int size = 2000;
            StaticJsonDocument<size> doc;
            deserializeJson(doc, req.body);
            serializeJsonPretty(doc, Serial);

            int valve             = doc["valve"];
            status().valveCurrent = valve;
            status().setValveStatus(valve, ValveStatus::operating);

            auto flush  = sm.getState<StateFlush>(StateName::flush);
            flush->time = doc["flushTime"];

            auto sample  = sm.getState<StateSample>(StateName::sample);
            sample->time = doc["sampleTime"];

            println("Received: ", valve, " ", flush->time, " ", sample->time);

            StaticJsonDocument<1000> responseMessage;
            responseMessage["success"] = "Successfully transitioned to flushing state";
            res.setHeader("Content-Type", "application/json");
            res.sendJson<1000>(responseMessage);
            res.end();

            sm.transitionTo(StateName::flush);
        });
    }

    void commandReceived(const String & line) override {
        if (line == "printlogs") {
            File logs = SD.open("LOGS.TXT", FILE_READ);
            if (!logs) {
                println("Logs file not found");
                logs.close();
                return;
            }

            int c;
            while ((c = logs.read()) != EOF) {
                print(static_cast<char>(c));
            }

            logs.close();
        }

        if (line == "clearlogs") {
            char filename[] = "logs.txt";
            SD.remove(filename);
        }
    }

public:
    KPActionScheduler<10> scheduler{"scheduler"};
    KPServer web{"web-server", "test", "password"};
    KPSDCard card{"sd-card", SDCard_Pin};
    KPStateMachine sm{"state-machine"};

    Pump pump{"pump", Motor_Forward_Pin, Motor_Reverse_Pin};
    ShiftRegister shift{"shift-register", 32, Shift_Register_Data_Pin, Shift_Register_Clock_Pin, Shift_Register_Latch_Pin};

    void setup() override {
        KPController::setup();

        // Serial monitor
        Serial.begin(9600);
        delay(7000);

        // For SD card module to work
        pinMode(5, OUTPUT);
        digitalWrite(5, LOW);

        // State registrations
        sm.registerState(StateIdle(), StateName::idle);
        sm.registerState(StateStop(), StateName::stop, 1);
        sm.registerState(StateSample(), StateName::sample, 3);
        sm.registerState(StateFlush(), StateName::flush, 4);

        // KPSerialInput delegate
        KPSerialInput::instance().addListener(this);

        // Adding components
        addComponent(sm);
        addComponent(scheduler);
        addComponent(web);
        addComponent(pump);
        addComponent(card);
        addComponent(shift);

        // Setup server
        setupServer();
        web.begin();

        // Transition to idle state
        sm.transitionTo(StateName::idle);

        KPActionChain<1> logging;
        logging.delay(1000, [this]() {
            logToFile("logs.txt", card);
        });
        runForever(logging, scheduler);
    }

    void update() override {
        KPController::update();
    }

    Status & status() {
        static Status s;
        return s;
    }

    void logToFile(const char * _filepath, KPDataStoreInterface & datastore) {
        char filepath[16]{0};
        strcpy(filepath, _filepath);

        if (!SD.exists(filepath)) {
            File log = SD.open(filepath, FILE_WRITE);
            log.println("UTC Time, Current State, Current Valve");
            log.close();
        }

        File log = SD.open(filepath, FILE_WRITE);
        log.printf("%d, %s, %d\n", millis(), sm.getCurrentState()->getName(), status().valveCurrent);
        log.close();
    }
};