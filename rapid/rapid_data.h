#pragma once

#include "esphome/components/climate/climate_mode.h"
#include "rapid_protocol.h"

namespace esphome
{
  namespace rapid_ir
  {

    using climate::ClimateFanMode;
    using climate::ClimateMode;
    using remote_base::RapidData;

    class ControlData : public RapidData
    {
    public:
      ControlData() : RapidData({0xFF, 0x00, 0xFF, 0x00, 0x7C, 0x83, 0xC5, 0x3A, 0xDD, 0x22, 0xAA, 0x55}) {}
      ControlData(const RapidData &data) : RapidData(data) {}

      void set_temp(float temp);
      float get_temp() const;

      void set_mode(ClimateMode mode);
      ClimateMode get_mode() const;

      void set_fan_mode(ClimateFanMode mode);
      ClimateFanMode get_fan_mode() const;

      void set_sleep_preset(bool value) { this->set_mask_(1, value, 64); }
      bool get_sleep_preset() const { return this->get_value_(1, 64); }

    protected:
      enum Mode : uint8_t
      {
        MODE_COOL = 0b001,
        MODE_DRY = 0b010,
        MODE_AUTO = 0b000,
        MODE_HEAT = 0b100,
        MODE_FAN_ONLY = 0b011
      };
      enum FanMode : uint8_t
      {
        FAN_AUTO = 0b00,
        FAN_LOW = 0b11,
        FAN_MEDIUM = 0b10,
        FAN_HIGH = 0b01,
      };
      void set_fan_mode_(FanMode mode) { this->set_value_(3, mode, 3, 5); }
      FanMode get_fan_mode_() const { return static_cast<FanMode>(this->get_value_(3, 3, 5)); }
      void set_mode_(Mode mode) { this->set_value_(4, mode, 0b111, 5); }
      Mode get_mode_() const { return static_cast<Mode>(this->get_value_(4, 0b111, 5)); }
      void set_power_(bool value) { this->set_mask_(3, value, 0b10); }
      bool get_power_() const { return this->get_value_(3, 0b10); }
    };
  }
}
