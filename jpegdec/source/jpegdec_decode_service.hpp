#pragma once

#include <stratosphere/sf.hpp>

namespace ams::jpegdec {

    class DecodeService final : public sf::IServiceObject {
        protected:
            /* Command IDs. */
            enum class CommandId {
                DecodeJpeg  = 3001,
            };
        public:
            virtual Result DecodeJpeg(sf::OutNonSecureBuffer out, sf::InBuffer in, const u32 width, const u32 height, const u64 q, const u64 qw, const u64 qwe, const u64 qwer);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(DecodeJpeg)
            };
    };
}