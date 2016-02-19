// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

#ifndef MEASUREMENT_KIT_COMMON_ERROR_HPP
#define MEASUREMENT_KIT_COMMON_ERROR_HPP

#include <exception>
#include <iosfwd>
#include <string>

namespace mk {

/// An error that occurred
class Error : public std::exception {
  public:
    const char *file = ""; ///< Offending file name
    int lineno = 0;        ///< Offending line number
    const char *func = ""; ///< Offending function

    /// Constructor with error code and OONI error
    Error(int e, std::string ooe) {
        if (e != 0 && ooe == "") {
            ooe = "unknown_failure " + std::to_string(e);
        }
        error_ = e;
        ooni_error_ = ooe;
    }

    Error() : Error(0, "") {}               ///< Default constructor (no error)
    operator int() const { return error_; } ///< Cast to integer

    /// Equality operator
    bool operator==(int n) const { return error_ == n; }

    /// Equality operator
    bool operator==(Error e) const { return error_ == e.error_; }

    /// Unequality operator
    bool operator!=(int n) const { return error_ != n; }

    /// Unequality operator
    bool operator!=(Error e) const { return error_ != e.error_; }

    /// Return error as OONI error
    std::string as_ooni_error() { return ooni_error_; }

  private:
    int error_ = 0;
    std::string ooni_error_;
};

#define MK_DECLARE_ERROR(code, name, ooni_error)                               \
    class name : public Error {                                                \
      public:                                                                  \
        name() : Error(code, ooni_error) {}                                      \
    }

#define MK_THROW(classname)                                                    \
    do {                                                                       \
        auto error = classname();                                              \
        error.file = __FILE__;                                                 \
        error.lineno = __LINE__;                                               \
        error.func = __func__;                                                 \
        throw error;                                                           \
    } while (0)

MK_DECLARE_ERROR(0, NoError, "");
MK_DECLARE_ERROR(1, GenericError, "");
MK_DECLARE_ERROR(2, MaybeNotInitializedError, "");
MK_DECLARE_ERROR(3, NullPointerError, "");
MK_DECLARE_ERROR(4, MallocFailedError, "");

MK_DECLARE_ERROR(5, EvutilMakeSocketNonblockingError, "");
MK_DECLARE_ERROR(6, EvutilParseSockaddrPortError, "");
MK_DECLARE_ERROR(7, EvutilMakeListenSocketReuseableError, "");

MK_DECLARE_ERROR(8, EventBaseDispatchError, "");
MK_DECLARE_ERROR(9, EventBaseLoopError, "");
MK_DECLARE_ERROR(10, EventBaseLoopbreakError, "");
MK_DECLARE_ERROR(11, EventBaseOnceError, "");

MK_DECLARE_ERROR(12, BuffereventSocketNewError, "");
MK_DECLARE_ERROR(13, BuffereventSocketConnectError, "");
MK_DECLARE_ERROR(14, BuffereventWriteError, "");
MK_DECLARE_ERROR(15, BuffereventWriteBufferError, "");
MK_DECLARE_ERROR(16, BuffereventReadBufferError, "");
MK_DECLARE_ERROR(17, BuffereventEnableError, "");
MK_DECLARE_ERROR(18, BuffereventDisableError, "");
MK_DECLARE_ERROR(19, BuffereventSetTimeoutsError, "");
MK_DECLARE_ERROR(20, BuffereventOpensslFilterNewError, "");

MK_DECLARE_ERROR(21, EvbufferAddError, "");
MK_DECLARE_ERROR(22, EvbufferAddBufferError, "");
MK_DECLARE_ERROR(23, EvbufferPeekError, "");
MK_DECLARE_ERROR(24, EvbufferPeekMismatchError, "");
MK_DECLARE_ERROR(25, EvbufferDrainError, "");
MK_DECLARE_ERROR(26, EvbufferRemoveBufferError, "");
MK_DECLARE_ERROR(27, EvbufferPullupError, "");

MK_DECLARE_ERROR(28, EvdnsBaseNewError, "");
MK_DECLARE_ERROR(29, EvdnsBaseResolveIpv4Error, "");
MK_DECLARE_ERROR(30, EvdnsBaseResolveIpv6Error, "");
MK_DECLARE_ERROR(31, EvdnsBaseResolveReverseIpv4Error, "");
MK_DECLARE_ERROR(32, EvdnsBaseResolveReverseIpv6Error, "");
MK_DECLARE_ERROR(33, InvalidIPv4AddressError, "");
MK_DECLARE_ERROR(34, InvalidIPv6AddressError, "");
MK_DECLARE_ERROR(35, EvdnsBaseClearNameserversAndSuspendError, "");

MK_DECLARE_ERROR(37, EvdnsBaseCountNameserversError, "");
MK_DECLARE_ERROR(38, EvdnsBaseNameserverIpAddError, "");
MK_DECLARE_ERROR(39, EvdnsBaseResumeError, "");
MK_DECLARE_ERROR(40, EvdnsBaseSetOptionError, "");

MK_DECLARE_ERROR(41, HttpParserUpgradeError, "");
MK_DECLARE_ERROR(42, HttpParserGenericParseError, "");

MK_DECLARE_ERROR(42, TypeError, "");

} // namespace mk
#endif
