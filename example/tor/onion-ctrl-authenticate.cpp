// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

/// Example showing how to authenticate to control port

#include "example/tor/onion-ctrl-main.hpp"

// XXX: should not raise exception on invalid addrport!!!

int main(int argc, char **argv) {
    run_main_loop(argc, argv, [](OnionStatus status, Var<OnionCtrl> ctrl) {
        std::cout << "result: " << (int)status << "\n";
        EventBase::loopbreak(ctrl->evbase);
        OnionCtrl::close(ctrl);
    });
}
