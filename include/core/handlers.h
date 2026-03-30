#pragma once

#include "IMessageHandler.h"
#include "processor/pdf_qr_processor.h"

namespace extractor {

class PdfHandler : public IMessageHandler {
public:
    explicit PdfHandler(PDFQRProcessor& processor);

    std::string handle(const IMessage& msg, uint64_t rid) override;

private:
    PDFQRProcessor& processor_;
};

class DefaultHandler : public IMessageHandler {
public:
    std::string handle(const IMessage& msg, uint64_t rid) override;
};

} // namespace extractor