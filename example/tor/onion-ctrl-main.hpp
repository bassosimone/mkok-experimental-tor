// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

/// Common code for main()

#include <functional>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include "src/common/libevent-core.hpp"
#include "src/tor/onion-ctrl.hpp"

#define USAGE "usage: %s [-A addr] [-f auth_cookie] [-p port] [-t timeout]\n"

using namespace mk;

static void run_main_loop(int argc, char **argv,
                          std::function<void(OnionStatus, Var<OnionCtrl>)> callback) {
    std::string address = "127.0.0.1";
    std::string auth_token = "";
    int port = 9051;
    timeval tv;
    const timeval *timeout = nullptr;
    int ch;
    memset(&tv, 0, sizeof(tv));
    const char *progname = argv[0];

    while ((ch = getopt(argc, argv, "A:f:p:t:")) != -1) {
        switch (ch) {
        case 'A':
            address = optarg;
            break;
        case 'f':
            auth_token = OnionCtrl::read_auth_cookie_as_hex(optarg);
            break;
        case 'p':
            port = atoi(optarg); // XXX
            break;
        case 't':
            tv.tv_sec = atoi(optarg); // XXX
            break;
        default:
            printf(USAGE, progname);
            exit(1);
        }
    }
    argc -= optind, argv += optind;
    if (argc != 0) {
        printf(USAGE, progname);
        exit(1);
    }

    Var<EventBase> evbase = EventBase::create();
    Var<OnionCtrl> ctrl = OnionCtrl::create(evbase);
    OnionCtrl::connect_and_authenticate(
        ctrl, [callback, ctrl](OnionStatus status) {
            callback(status, ctrl);
        }, auth_token, port, address, timeout);
    evbase->dispatch();
}
