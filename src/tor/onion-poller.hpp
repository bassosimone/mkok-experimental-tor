// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

#ifndef SRC_TOR_ONION_POLLER_HPP
#define SRC_TOR_ONION_POLLER_HPP

#include <event2/event.h>
#include <functional>
#include <libtor.h>
#include <measurement_kit/common/error.hpp>
#include <measurement_kit/common/func.hpp>
#include <measurement_kit/common/var.hpp>
#include <string.h>
#include <sys/time.h>
#include "src/common/libevent-core.hpp"
#include "src/tor/onion-ctrl.hpp"

extern "C" {

static void mk_onion_poller_cb(void *opaque);

} // extern "C"

namespace mk {

class OnionPoller;

typedef std::function<void(Var<OnionPoller>)> OnionPollerCb;

class OnionPoller {
  public:
    Var<EventBase> evbase;

    static void loop(OnionPollerCb func) {
        static const char *argv[] = {
            "tor", "ControlPort", "9051", "DisableNetwork",
            "1",   "ConnLimit",   "50",   nullptr};
        auto cb = new OnionPollerCb(func);
        tor_on_started(mk_onion_poller_cb, cb);
        tor_main(7, (char **)argv);
    }

    static void break_loop(Var<OnionPoller> /*poller*/) { tor_break_loop(); }

    // Needed when testing; because now we GC terminated bufferevents
    // only after a I/O loop delay, so we need to defer exit to make sure
    // that they have actually been collected and no leaks appear
    static void break_soon(Var<OnionPoller> poller) {
        timeval timeo;
        timeo.tv_sec = timeo.tv_usec = 1;
        poller->evbase->once(-1, EV_TIMEOUT,
                        [poller](short) { break_loop(poller); }, &timeo);
    }

    static void enable_tor(Var<OnionPoller> poller, OnionReplyVoidCb cb,
                           unsigned timeout) {
        Var<OnionCtrl> ctrl = OnionCtrl::create(poller->evbase);
        OnionCtrl::connect_and_authenticate(ctrl, [cb, ctrl, poller, timeout](
                                                      OnionStatus status) {
            if (status != OnionStatus::OK) {
                OnionCtrl::close(ctrl);
                cb(status);
                return;
            }
            OnionCtrl::setconf_disable_network(
                ctrl, false, [cb, ctrl, poller, timeout](OnionStatus status) {
                    if (status != OnionStatus::OK) {
                        OnionCtrl::close(ctrl);
                        cb(status);
                        return;
                    }
                    wait_for_circuit_(poller, ctrl, cb, timeout);
                });
        });
    }

    static void disable_tor(Var<OnionPoller> poller, OnionReplyVoidCb cb) {
        Var<OnionCtrl> ctrl = OnionCtrl::create(poller->evbase);
        OnionCtrl::connect_and_authenticate(
            ctrl, [cb, ctrl](OnionStatus status) {
                if (status != OnionStatus::OK) {
                    OnionCtrl::close(ctrl);
                    cb(status);
                    return;
                }
                OnionCtrl::setconf_disable_network(
                    ctrl, true, [cb, ctrl](OnionStatus status) {
                        OnionCtrl::close(ctrl);
                        if (status != OnionStatus::OK) {
                            cb(status);
                            return;
                        }
                        cb(OnionStatus::OK);
                    });
            });
    }

    static void wait_for_circuit_(Var<OnionPoller> poller, Var<OnionCtrl> ctrl,
                                  OnionReplyVoidCb cb, unsigned timeout,
                                  unsigned counter = 0) {
        if (counter >= timeout) {
            // If we cannot establish a circuit within the given timeout, we
            // tell tor to stop bothering with connect and notify back
            OnionCtrl::setconf_disable_network(
                ctrl, true, [cb, ctrl](OnionStatus status) {
                    OnionCtrl::close(ctrl);
                    if (status != OnionStatus::OK) {
                        cb(status);
                        return;
                    }
                    cb(OnionStatus::GENERIC_ERROR);
                });
            return;
        }
        timeval timeo;
        memset(&timeo, 0, sizeof(timeo));
        timeo.tv_sec = 1;
        poller->evbase->once(
            -1, EV_TIMEOUT, [cb, ctrl, counter, poller, timeout](short) {
                OnionCtrl::getinfo_status_bootstrap_phase_as_int(
                    ctrl, [cb, ctrl, counter, poller,
                           timeout](OnionStatus status, int progress) {
                        if (status != OnionStatus::OK) {
                            // TODO: What is the correct thing to do here?
                            OnionCtrl::close(ctrl);
                            cb(status);
                            return;
                        }
                        if (progress != 100) {
                            OnionPoller::wait_for_circuit_(
                                poller, ctrl, cb, timeout, counter + 1);
                            return;
                        }
                        OnionCtrl::close(ctrl);
                        cb(OnionStatus::OK);
                    });
            }, &timeo);
    }
};

} // namespace mk

extern "C" {

static void mk_onion_poller_cb(void *opaque) {
    auto func = static_cast<mk::OnionPollerCb *>(opaque);
    mk::Var<mk::OnionPoller> poller(new mk::OnionPoller);
    poller->evbase = mk::EventBase::assign(tor_libevent_get_base(), false);
    (*func)(poller);
    delete func;
}

} // extern "C"

#endif
