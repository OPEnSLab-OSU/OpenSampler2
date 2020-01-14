#pragma once

#define Power_Module_Pin A0
#define RTC_Interrupt_Pin        A1
#define Battery_Voltage_Pin      A2
#define Analog_Sensor_1_Pin      A3
#define Analog_Sensor_2_Pin      A4
#define Override_Mode_Pin        A5
#define Motor_Forward_Pin        5
#define Motor_Reverse_Pin        6
#define Shift_Register_Latch_Pin 9
#define SDCard_Pin               10
#define Shift_Register_Clock_Pin 11
#define Shift_Register_Data_Pin  12
#define Button_Pin               13

namespace JsonKey {
    // [+_+] State
    static constexpr const char * stateName{"stateName"};
    static constexpr const char * stateId{"stateId"};
    static constexpr const char * stateIndex{"stateIndex"};

    static constexpr const char * stateTimeLimit{"time"};
    static constexpr const char * statePressureLimit{"pressure"};
    static constexpr const char * stateVolumeLimit{"volume"};

    static constexpr const char * statePropertyName{"name"};
    static constexpr const char * statePropertyValue{"value"};
    static constexpr const char * statePropertyMax{"max"};
    static constexpr const char * statePropertyMin{"min"};
    static constexpr const char * statePropertyUnit{"unit"};

    // [+_+] Valves
    static constexpr const char * valveBegin{"valveBegin"};
    static constexpr const char * valveLowerBound{"valveLowerBound"};
    static constexpr const char * valveUpperBound{"valveUpperBound"};
    static constexpr const char * valveCurrent{"valveCurrent"};
    static constexpr const char * valves{"valves"};
    static constexpr const char * valveInterval{"valveInterval"};

    // [+_+] Time
    static constexpr const char * timeUTC{"timeUTC"};

    // [+_+] Sensor Data
    static constexpr const char * pressure{"pressure"};
    static constexpr const char * temperature{"temperature"};
    static constexpr const char * barometric{"barometric"};
    static constexpr const char * waterVolume{"waterVolume"};
    static constexpr const char * waterFlow{"waterFlow"};
    static constexpr const char * waterDepth{"waterDepth"};
};  // namespace JsonKey