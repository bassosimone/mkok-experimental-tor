// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

/// Example showing how to GETCONF SOCKSPort

#include "example/tor/onion-ctrl-main.hpp"

int main(int argc, char **argv) {
    run_main_loop(argc, argv, [](OnionStatus status, Var<OnionCtrl> ctrl) {
        std::cout << "status: " << (int)status << "\n";
        if (status != OnionStatus::OK) {
            EventBase::loopbreak(ctrl->evbase);
            return;
        }
        OnionCtrl::getconf_socks_port(
            ctrl, [ctrl](OnionStatus status, int port) {
                std::cout << "status: " << (int)status << "\n";
                std::cout << "port: " << port << "\n";
                OnionCtrl::close(ctrl);
                EventBase::loopbreak(ctrl->evbase);
            });
    });
}
