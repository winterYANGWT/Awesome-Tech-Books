#include "report_pimpl.h"

#include "legacy_formatter.h"

struct PimplReport::Impl {
    LegacyFormatter formatter;
};

PimplReport::PimplReport() : impl_(std::make_unique<Impl>()) {}
PimplReport::~PimplReport() = default;

std::string PimplReport::render(const std::string& text) const {
    return impl_->formatter.format(text);
}
