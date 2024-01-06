#include "esphome/core/log.h"
#include "rapid_protocol.h"

namespace esphome
{
  namespace remote_base
  {
    static const char *const TAG = "remote.rapid";

    static const int32_t TICK_US = 540;
    static const int32_t HEADER_MARK_US = 11 * TICK_US;
    static const int32_t HEADER_SPACE_US = 13 * TICK_US;
    static const int32_t BIT_MARK_US = 1 * TICK_US;
    static const int32_t BIT_ONE_SPACE_US = 3 * TICK_US;
    static const int32_t BIT_ZERO_SPACE_US = 1 * TICK_US;
    static const int32_t FOOTER_MARK_US = 1 * TICK_US;
    static const int32_t FOOTER_SPACE_US = 13 * TICK_US;

    void RapidProtocol::encode(RemoteTransmitData *dst, const RapidData &src)
    {
      dst->set_carrier_frequency(38000);
      dst->reserve(2 + 48 * 2 + 2 + 48 * 2 + 1);

      dst->item(HEADER_MARK_US, HEADER_SPACE_US);

      for (unsigned idx = 0; idx < 12; idx++)
      {
        for (uint8_t shift = 0; shift < 8; shift++)
        {
          dst->item(BIT_MARK_US, (src[idx] & (1 << shift)) ? BIT_ONE_SPACE_US : BIT_ZERO_SPACE_US);
        }
      }

      dst->item(FOOTER_MARK_US, FOOTER_SPACE_US);
    }

    static bool decode_data(RemoteReceiveData &src, RapidData &dst)
    {
      for (unsigned idx = 0; idx < 12; idx++)
      {
        uint8_t data = 0;

        for (uint8_t shift = 0; shift < 8; shift++)
        {
          if (!src.expect_mark(BIT_MARK_US))
            return false;

          if (src.expect_space(BIT_ONE_SPACE_US))
            data |= 1 << shift;

          else if (!src.expect_space(BIT_ZERO_SPACE_US))
            return false;
        }

        dst[idx] = data;
      }

      return true;
    }

    optional<RapidData> RapidProtocol::decode(RemoteReceiveData src)
    {
      RapidData out;

      if (src.expect_item(HEADER_MARK_US, HEADER_SPACE_US) && decode_data(src, out) && out.is_valid() &&
          src.expect_item(FOOTER_MARK_US, FOOTER_SPACE_US))
        return out;

      return {};
    }

    void RapidProtocol::dump(const RapidData &data) {
      ESP_LOGD(TAG, "Received Rapid Data: %s", data.to_string().c_str());
    }
  }
}
