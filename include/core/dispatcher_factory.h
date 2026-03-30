#pragma once

#include "dispatcher.h"
#include "handlers.h"
#include "processor/pdf_qr_processor.h"

namespace extractor {

inline Dispatcher createDispatcher(PDFQRProcessor& processor)
{
    Dispatcher dispatcher;

    dispatcher.registerHandler(
        "pdf",
        std::make_unique<PdfHandler>(processor)
    );

    dispatcher.registerHandler(
        "default",
        std::make_unique<DefaultHandler>()
    );

    return dispatcher;
}

} // namespace extractor