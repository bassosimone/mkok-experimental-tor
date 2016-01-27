// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

/// Example showing how to GET/SETCONF DisableNetwork

#include "example/tor/onion-ctrl-main.hpp"

int main(int argc, char **argv) {
    run_main_loop(argc, argv, [](OnionStatus status, Var<OnionCtrl> ctrl) {
        std::cout << "status: " << (int)status << "\n";
        if (status != OnionStatus::OK) {
            ctrl->evbase->loopbreak();
            return;
        }
        OnionCtrl::setevents_client_status(
            ctrl,
            [ctrl](OnionStatus status, std::string severity,
                          std::string action, std::vector<std::string> others) {
                if (status != OnionStatus::OK && status != OnionStatus::ASYNC) {
                    std::cout << "status: " << (int)status << "\n";
                    OnionCtrl::close(ctrl);
                    ctrl->evbase->loopbreak();
                    return;
                }
                if (status == OnionStatus::OK) {
                    return;
                }
                std::cout << severity << " " << action;
                for (std::string &s : others) {
                    std::cout << s << " ";
                }
                std::cout << "\n";
            });
    });
}
