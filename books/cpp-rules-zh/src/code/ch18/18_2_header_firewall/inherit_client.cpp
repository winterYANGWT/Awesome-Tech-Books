#include "report_inherit.h"

#include <iostream>

int main() {
    InheritedReport report;
    std::cout << "InheritedReport: " << report.render("Ada") << '\n';
}
