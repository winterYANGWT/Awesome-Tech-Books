#include "fixed_audit.hpp"

#include <iostream>

int main() {
    AuditLog log;
    log.add("login");
    log.add("export");
    std::cout << "audit entries: " << log.size() << '\n';
}
