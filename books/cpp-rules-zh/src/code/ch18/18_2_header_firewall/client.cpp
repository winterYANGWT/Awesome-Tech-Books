#include "report_pimpl.h"

#include <iostream>

int main() {
    PimplReport report;
    std::cout << "PimplReport: " << report.render("Ada") << '\n';
}
