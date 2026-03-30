#include "core/handlers.h"
#include "processor/pdf_qr_processor.h"
#include "logging/logger.h"
#include <stdexcept>
#include <iostream>

namespace extractor
{
    PdfHandler::PdfHandler(PDFQRProcessor &processor)
        : processor_(processor)
    {
    }

    std::string PdfHandler::handle(const IMessage &msg, uint64_t rid)
    {
        const std::string &pdfData = msg.raw();

        std::string result;

        if (!processor_.extract(pdfData, result, rid))
        {
            throw std::runtime_error("QR extraction failed");
        }
        if (result.empty()) {
            LOG_ERROR("PROCESSOR", "No QR found in PDF");
        }
        else {
            LOG_ERROR("PROCESSOR", "QR detected, size=" + std::to_string(result.size()));
        }
        return result;
    }

    std::string DefaultHandler::handle(const IMessage &msg, uint64_t rid)
    {
        (void)msg;
        return R"({"status":"error","message":"Unknown message type"})";
    }

}