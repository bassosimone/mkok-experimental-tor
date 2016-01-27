// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

#include <functional>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include "src/common/evhelpers.hpp"

#define USAGE "usage: %s [-A address] [-p port] [path]\n"

using namespace mk;

int main(int argc, char **argv) {
    std::string address = "130.192.181.193";
    std::string path = "/";
    std::string port = "443";
    const char *progname = argv[0];
    int ch;

    while ((ch = getopt(argc, argv, "A:p:")) != -1) {
        switch (ch) {
        case 'A':
            address = optarg;
            break;
        case 'p':
            port = optarg;
            break;
        default:
            fprintf(stderr, USAGE, progname);
            exit(1);
        }
    }
    argc -= optind, argv += optind;
    if (argc != 0 && argc != 1) {
        fprintf(stderr, USAGE, progname);
        exit(1);
    }
    if (argc == 1) path = argv[0];

    evhelpers::VERBOSE = true;
    std::string endpoint = address + ":" + port;
    std::string out;
    std::string *outp = &out;
    auto base = EventBase::create();
    evhelpers::ssl_connect(
        base, endpoint.c_str(), evhelpers::SslContext::get(),
        [base, path, outp](Var<Bufferevent> bev) {
            evhelpers::sendrecv(bev, "GET " + path + "\r\n", [base]() {
                evhelpers::break_soon(base);
            }, outp);
        });
    base->dispatch();
    std::cout << out << std::endl;
}
