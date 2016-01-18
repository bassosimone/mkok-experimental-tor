// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

/// Example showing how to call SIGNAL SHUTDOWN

#include "example/tor/onion-ctrl-main.hpp"

int main(int argc, char **argv) {
    run_main_loop(argc, argv, [](OnionStatus status, Var<OnionCtrl> ctrl) {
        std::cout << "status: " << (int)status << "\n";
        if (status != OnionStatus::OK) {
            EventBase::loopbreak(ctrl->evbase);
            return;
        }
        OnionCtrl::signal_shutdown(ctrl, [ctrl](OnionStatus status) {
            std::cout << "status: " << (int)status << "\n";
            OnionCtrl::close(ctrl);
            EventBase::loopbreak(ctrl->evbase);
        });
    });
}
