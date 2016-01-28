// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

#include <functional>
#include <iostream>
#include <openssl/ssl.h>
#include <string>
#include <sys/time.h>
#include "src/net/socks.hpp"
#include "src/common/evhelpers.hpp"

using namespace mk;

int main() {
    evhelpers::VERBOSE = true;
    std::string out;
    std::string *outp = &out;
    auto base = EventBase::create();
    Socks::connect(
        base, "130.192.16.172", 443,
        [base, outp](SocksStatus status, Var<Bufferevent> bev) {
            std::cout << "status: " << (int)status << "\n";
            if (status != SocksStatus::OK) {
                evhelpers::break_soon(base);
                return;
            }
            SSL *secure = SSL_new(evhelpers::SslContext::get());
            if (secure == nullptr) {
                bev->setcb(nullptr, nullptr, nullptr);
                throw std::exception();
            }
            auto ssl_bev = Bufferevent::openssl_filter_new(
                base, bev, secure, BUFFEREVENT_SSL_CONNECTING,
                BEV_OPT_CLOSE_ON_FREE);
            ssl_bev->setcb(
                nullptr, nullptr,
                [base, outp, ssl_bev](short what) {
                    std::cout << "what: " << Bufferevent::event_string(what) << "\n";
                    if (what != BEV_EVENT_CONNECTED) {
                        evhelpers::break_soon(base);
                        ssl_bev->setcb(nullptr, nullptr, nullptr);
                        return;
                    }
                    evhelpers::sendrecv(ssl_bev, "GET /robots.txt\r\n",
                                        [base]() {
                                            evhelpers::break_soon(base);
                                        }, outp);
                });
        });
    base->dispatch();
    std::cout << out << std::endl;
}
