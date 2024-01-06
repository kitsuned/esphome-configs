#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/components/remote_base/remote_base.h"

#include <array>
#include <utility>
#include <vector>
#include <bitset>

static const char *const TAG = "rapid_ir.proto";

namespace esphome
{
  namespace remote_base
  {
    class RapidData
    {
    public:
      RapidData() {}
      RapidData(std::initializer_list<uint8_t> data)
      {
        std::copy_n(data.begin(), std::min(data.size(), this->data_.size()), this->data_.begin());
      }
      RapidData(const std::vector<uint8_t> &data)
      {
        std::copy_n(data.begin(), std::min(data.size(), this->data_.size()), this->data_.begin());
      }
      RapidData(const RapidData &) = default;

      uint8_t *data() { return this->data_.data(); }
      const uint8_t *data() const { return this->data_.data(); }
      uint8_t size() const { return this->data_.size(); }

      bool is_valid() const
      {
        if (this->data_.size() != 12 || this->data_.front() != 0b11111111 || this->data_.back() != 0b01010101)
        {
          ESP_LOGW(TAG, "Bad signature: size = %d, header = %s, footer = %s", this->data_.size(),
                   std::bitset<8>(this->data_.front()).to_string().c_str(),
                   std::bitset<8>(this->data_.back()).to_string().c_str());

          return false;
        }

        for (size_t i = 0; i < this->size(); i += 2)
        {
          auto chsum = this->data_[i] ^ 0xff;
          auto data = this->data_[i + 1];

          if (chsum != data)
          {
            ESP_LOGW(TAG, "Pair %d mismatch", i);

            return false;
          }
        }

        return true;
      }

      void finalize()
      {
        for (size_t i = 0; i < this->size(); i += 2)
        {
          this->data_[i] = this->data_[i + 1] ^ 0xff;
        }
      }

      std::string to_string() const { return format_hex_pretty(this->data_.data(), this->data_.size()); }
      bool operator==(const RapidData &rhs) const
      {
        return std::equal(this->data_.begin(), this->data_.begin() + OFFSET_CS, rhs.data_.begin());
      }
      template <typename T>
      T to() const { return T(*this); }
      uint8_t &operator[](size_t idx) { return this->data_[idx]; }
      const uint8_t &operator[](size_t idx) const { return this->data_[idx]; }

    protected:
      uint8_t get_value_(uint8_t idx, uint8_t mask = 255, uint8_t shift = 0) const
      {
        return (this->data_[idx * 2 + 1] >> shift) & mask;
      }
      void set_value_(uint8_t idx, uint8_t value, uint8_t mask = 255, uint8_t shift = 0)
      {
        this->data_[idx * 2 + 1] &= ~(mask << shift);
        this->data_[idx * 2 + 1] |= (value << shift);
      }
      void set_mask_(uint8_t idx, bool state, uint8_t mask = 255) { this->set_value_(idx, state ? mask : 0, mask); }
      static const uint8_t OFFSET_CS = 10;
      std::array<uint8_t, 12> data_;
    };

    class RapidProtocol : public RemoteProtocol<RapidData>
    {
    public:
      void encode(RemoteTransmitData *dst, const RapidData &src) override;
      optional<RapidData> decode(RemoteReceiveData src) override;
      void dump(const RapidData &data) override;
    };

    class RapidBinarySensor : public RemoteReceiverBinarySensorBase
    {
    public:
      bool matches(RemoteReceiveData src) override
      {
        auto data = RapidProtocol().decode(src);
        return data.has_value() && data.value() == this->data_;
      }
      void set_code(const std::vector<uint8_t> &code) { this->data_ = code; }

    protected:
      RapidData data_;
    };

    using RapidTrigger = RemoteReceiverTrigger<RapidData>;
    using RapidDumper = RemoteReceiverDumper<RapidData>;

    template <typename... Ts>
    class RapidAction : public RemoteTransmitterActionBase<Ts...>
    {
      TEMPLATABLE_VALUE(std::vector<uint8_t>, code)
      void set_code_static(std::vector<uint8_t> code) { code_static_ = std::move(code); }
      void set_code_template(std::function<std::vector<uint8_t>(Ts...)> func) { this->code_func_ = func; }

      void encode(RemoteTransmitData *dst, Ts... x) override
      {
        RapidData data;
        if (!this->code_static_.empty())
        {
          data = RapidData(this->code_static_);
        }
        else
        {
          data = RapidData(this->code_func_(x...));
        }
        data.finalize();
        RapidProtocol().encode(dst, data);
      }

    protected:
      std::function<std::vector<uint8_t>(Ts...)> code_func_{};
      std::vector<uint8_t> code_static_{};
    };
  }
}
