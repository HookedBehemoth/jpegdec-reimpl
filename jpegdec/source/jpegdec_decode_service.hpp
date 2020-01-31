#pragma once
#include <stratosphere.hpp>

namespace ams::jpegdec {

    class DecodeService final : public sf::IServiceObject {
        protected:
            /* Command IDs. */
            enum class CommandId {
                DecodeJpeg  = 3001,
            };
        public:
            virtual Result DecodeJpeg(const sf::OutNonSecureBuffer &out, const sf::InBuffer &in, const u32 width, const u32 height, const CapsScreenShotDecodeOption &opts);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(DecodeJpeg)
            };
    };
}