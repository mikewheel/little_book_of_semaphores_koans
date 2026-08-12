// koans.hpp — the one tiny header starter code may include.
// (The test harness lives in koan_test.hpp; starter code never includes it.)
#pragma once

#include <stdexcept>
#include <string>

namespace koans {

// Thrown by starter code you haven't written yet. Delete the `throw` when
// you implement a method.
struct NotImplemented : std::logic_error {
    explicit NotImplemented(const std::string& where)
        : std::logic_error("not implemented yet: " + where) {}
};

}  // namespace koans
