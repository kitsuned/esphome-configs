#include "rapid.h"
#include "rapid_data.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome
{
  namespace rapid_ir
  {
    static const char *const TAG = "rapid_ir.climate";

    void ControlData::set_temp(float temp)
    {
      this->set_value_(4, lroundf(temp) - RAPID_TEMPC_MIN, 0b1111);
    }

    float ControlData::get_temp() const
    {
      return static_cast<float>(this->get_value_(4, 0b1111) + RAPID_TEMPC_MIN);
    }

    void ControlData::set_mode(ClimateMode mode)
    {
      switch (mode)
      {
      case ClimateMode::CLIMATE_MODE_OFF:
        this->set_power_(false);
        return;
      case ClimateMode::CLIMATE_MODE_COOL:
        this->set_mode_(MODE_COOL);
        break;
      case ClimateMode::CLIMATE_MODE_DRY:
        this->set_mode_(MODE_DRY);
        break;
      case ClimateMode::CLIMATE_MODE_FAN_ONLY:
        this->set_mode_(MODE_FAN_ONLY);
        break;
      case ClimateMode::CLIMATE_MODE_HEAT:
        this->set_mode_(MODE_HEAT);
        break;
      default:
        this->set_mode_(MODE_AUTO);
        break;
      }
      this->set_power_(true);
    }

    ClimateMode ControlData::get_mode() const
    {
      if (!this->get_power_())
        return ClimateMode::CLIMATE_MODE_OFF;
      switch (this->get_mode_())
      {
      case MODE_COOL:
        return ClimateMode::CLIMATE_MODE_COOL;
      case MODE_DRY:
        return ClimateMode::CLIMATE_MODE_DRY;
      case MODE_FAN_ONLY:
        return ClimateMode::CLIMATE_MODE_FAN_ONLY;
      case MODE_HEAT:
        return ClimateMode::CLIMATE_MODE_HEAT;
      default:
        return ClimateMode::CLIMATE_MODE_HEAT_COOL;
      }
    }

    void ControlData::set_fan_mode(ClimateFanMode mode)
    {
      switch (mode)
      {
      case ClimateFanMode::CLIMATE_FAN_LOW:
        this->set_fan_mode_(FAN_LOW);
        break;
      case ClimateFanMode::CLIMATE_FAN_MEDIUM:
        this->set_fan_mode_(FAN_MEDIUM);
        break;
      case ClimateFanMode::CLIMATE_FAN_HIGH:
        this->set_fan_mode_(FAN_HIGH);
        break;
      default:
        this->set_fan_mode_(FAN_AUTO);
        break;
      }
    }

    ClimateFanMode ControlData::get_fan_mode() const
    {
      switch (this->get_fan_mode_())
      {
      case FAN_LOW:
        return ClimateFanMode::CLIMATE_FAN_LOW;
      case FAN_MEDIUM:
        return ClimateFanMode::CLIMATE_FAN_MEDIUM;
      case FAN_HIGH:
        return ClimateFanMode::CLIMATE_FAN_HIGH;
      default:
        return ClimateFanMode::CLIMATE_FAN_AUTO;
      }
    }

    void RapidIR::control(const climate::ClimateCall &call)
    {
      if (call.get_mode() == climate::CLIMATE_MODE_OFF)
      {
        this->swing_mode = climate::CLIMATE_SWING_OFF;
        this->preset = climate::CLIMATE_PRESET_NONE;
      }

      climate_ir::ClimateIR::control(call);
    }

    void RapidIR::transmit_(RapidData &data)
    {
      data.finalize();

      ESP_LOGI(TAG, "Encoded Rapid Data: %s", data.to_string().c_str());

      auto transmit = this->transmitter_->transmit();
      remote_base::RapidProtocol().encode(transmit.get_data(), data);
      transmit.perform();
    }

    void RapidIR::transmit_state()
    {
      ControlData data;

      data.set_temp(this->target_temperature);
      data.set_mode(this->mode);
      data.set_fan_mode(this->fan_mode.value_or(ClimateFanMode::CLIMATE_FAN_AUTO));
      data.set_sleep_preset(this->preset == climate::CLIMATE_PRESET_SLEEP);

      this->transmit_(data);
    }

    bool RapidIR::on_receive(remote_base::RemoteReceiveData data)
    {
      auto rapid = remote_base::RapidProtocol().decode(data);

      return rapid.has_value() && this->on_rapid_(*rapid);
    }

    bool RapidIR::on_rapid_(const RapidData &data)
    {
      ESP_LOGI(TAG, "Decoded Rapid IR data: %s", data.to_string().c_str());
      const ControlData status = data;

      if (status.get_mode() != climate::CLIMATE_MODE_FAN_ONLY)
        this->target_temperature = status.get_temp();

      this->mode = status.get_mode();
      this->fan_mode = status.get_fan_mode();

      if (status.get_sleep_preset())
        this->preset = climate::CLIMATE_PRESET_SLEEP;
      else if (this->preset == climate::CLIMATE_PRESET_SLEEP)
        this->preset = climate::CLIMATE_PRESET_NONE;

      this->publish_state();

      return true;
    }
  }
}
