// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

#ifndef MEASUREMENT_KIT_HTTP_ERROR_HPP
#define MEASUREMENT_KIT_HTTP_ERROR_HPP

#include <measurement_kit/common/error.hpp>

namespace mk {

MK_DECLARE_ERROR(3000, HttpParserUpgrade, "");
MK_DECLARE_ERROR(3001, HttpParserGenericParse, "");

} // namespace mk
#endif
