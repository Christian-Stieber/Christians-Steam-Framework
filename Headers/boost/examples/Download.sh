#! /bin/sh

rm -rf tmp-fiber
git clone https://github.com/boostorg/fiber tmp-fiber

rm -rf fiber
mkdir --parents fiber/asio/detail
cp "tmp-fiber/examples/asio/round_robin.hpp" "fiber/asio/round_robin.hpp"
cp "tmp-fiber/examples/asio/detail/yield.hpp" "fiber/asio/detail/yield.hpp"
grep -Fx -v "thread_local yield_t yield{};" "tmp-fiber/examples/asio/yield.hpp" > "fiber/asio/yield.hpp"

rm -rf tmp-fiber
