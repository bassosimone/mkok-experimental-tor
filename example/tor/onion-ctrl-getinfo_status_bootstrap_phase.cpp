// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

/// Example showing how to GETINFO status/bootstrap-phase

#include "example/tor/onion-ctrl-main.hpp"

int main(int argc, char **argv) {
    run_main_loop(argc, argv, [](OnionStatus status, Var<OnionCtrl> ctrl) {
        std::cout << "status: " << (int)status << "\n";
        if (status != OnionStatus::OK) {
            ctrl->evbase->loopbreak();
            return;
        }
        OnionCtrl::getinfo_status_bootstrap_phase_as_int(
            ctrl, [ctrl](OnionStatus status, int phase) {
                std::cout << "status: " << (int)status << "\n";
                std::cout << "phase: " << phase << "\n";
                OnionCtrl::close(ctrl);
                ctrl->evbase->loopbreak();
            });
    });
}
