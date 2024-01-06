#pragma once

#include "esphome/components/climate_ir/climate_ir.h"
#include "rapid_data.h"

namespace esphome
{
  namespace rapid_ir
  {
    const uint8_t RAPID_TEMPC_MIN = 16;
    const uint8_t RAPID_TEMPC_MAX = 32;

    class RapidIR : public climate_ir::ClimateIR
    {
    public:
      RapidIR()
          : climate_ir::ClimateIR(
                RAPID_TEMPC_MIN, RAPID_TEMPC_MAX, 1.0f, true, true,
                {climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_LOW, climate::CLIMATE_FAN_MEDIUM,
                 climate::CLIMATE_FAN_HIGH},
                {climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL},
                {climate::CLIMATE_PRESET_NONE, climate::CLIMATE_PRESET_SLEEP, climate::CLIMATE_PRESET_BOOST}) {}

      void control(const climate::ClimateCall &call) override;

    protected:
      void transmit_state() override;
      void transmit_(RapidData &data);
      bool on_receive(remote_base::RemoteReceiveData data) override;
      bool on_rapid_(const RapidData &data);
    };
  }
}
