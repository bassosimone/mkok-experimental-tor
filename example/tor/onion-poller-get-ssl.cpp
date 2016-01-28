// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

#include <functional>
#include <iostream>
#include <string>
#include <sys/time.h>
#include "src/common/libevent.hpp"
#include "src/common/evhelpers.hpp"
#include "src/net/socks.hpp"
#include "src/tor/onion-poller.hpp"

using namespace mk;

int main() {
    static const char *request = "GET /robots.txt\r\n";
    evhelpers::VERBOSE = true;
    std::string out;
    std::string *outp = &out;
    OnionPoller::loop([outp](Var<OnionPoller> poller) {
        OnionPoller::enable_tor(poller, [outp, poller](OnionStatus status) {
            std::cout << "enable... " << (int)status << "\n";
            if (status != OnionStatus::OK) {
                OnionPoller::break_soon(poller);
                return;
            }
            std::cout << "connecting to socks proxy...\n";
            Socks::connect(
                poller->evbase, "130.192.16.172", 443,
                [outp, poller](SocksStatus status, Var<Bufferevent> bev) {
                    std::cout << "proxy connect... " << (int)status << "\n";
                    if (status != SocksStatus::OK) {
                        OnionPoller::break_soon(poller);
                        return;
                    }
                    std::cout << "ssl...\n";
                    SSL *secure = SSL_new(evhelpers::SslContext::get());
                    if (secure == nullptr) {
                        bev->setcb(nullptr, nullptr, nullptr);
                        throw std::exception();
                    }
                    auto ssl_bev = Bufferevent::openssl_filter_new(
                        poller->evbase, bev, secure, BUFFEREVENT_SSL_CONNECTING,
                        BEV_OPT_CLOSE_ON_FREE);
                    ssl_bev->setcb(
                        nullptr, nullptr,
                        [outp, poller, ssl_bev](short what) {
                            // FIXME: do not assumre we're connected here
                            std::cout << "ssl... ok\n";
                            timeval timeo;
                            timeo.tv_sec = timeo.tv_usec = 3;
                            evhelpers::sendrecv(ssl_bev, request, [poller]() {
                                std::cout << "sendrecv done... \n";
                                OnionPoller::disable_tor(
                                    poller, [poller](OnionStatus status) {
                                        std::cout << "disable... "
                                                  << (int)status << "\n";
                                        OnionPoller::break_soon(poller);
                                    });
                            }, outp, &timeo);
                        });
                });
        }, 5);
    });
    std::cout << out << std::endl;
}
