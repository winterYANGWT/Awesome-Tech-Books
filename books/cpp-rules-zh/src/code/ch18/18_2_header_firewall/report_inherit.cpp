#include "report_inherit.h"

std::string InheritedReport::render(const std::string& text) const {
    return format(text);
}
