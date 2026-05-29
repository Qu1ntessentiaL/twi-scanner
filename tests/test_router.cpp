#include "cli/CommandRouter.hpp"

#include <cassert>
#include <iostream>

int main() {
    CommandRouter router;
    bool called = false;

    router.registerCommand("ping", [&](AppContext &, std::istringstream &) { called = true; },
                           "Ping test");

    AppContext ctx;
    assert(router.execute(ctx, "ping"));
    assert(called);

    assert(!router.execute(ctx, "missing"));
    assert(router.execute(ctx, ""));

    const std::string *desc = router.findDescription("ping");
    assert(desc != nullptr && *desc == "Ping test");
    assert(router.findDescription("nope") == nullptr);

    std::cout << "test_router: OK\n";
    return 0;
}
