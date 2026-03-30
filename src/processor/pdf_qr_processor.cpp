#include "processor/pdf_qr_processor.h"

#include "logging/logger.h"

#include <poppler-document.h>
#include <poppler-page.h>
#include <poppler-page-renderer.h>

#include <ZXing/BarcodeFormat.h>
#include <ZXing/DecodeHints.h>
#include <ZXing/ImageView.h>
#include <ZXing/ReadBarcode.h>

#include "stb_image.h"
#include "stb_image_write.h"

#include <iostream>
#include <memory>

namespace extractor {

namespace {

// Internal helper — NOT visible outside this file
void renderPageToPng(poppler::image& image, std::vector<unsigned char>& outPng)
{
    int width = image.width();
    int height = image.height();
    int channels = 4;
    int stride = image.bytes_per_row();

    stbi_write_png_to_func(
        [](void* ctx, void* data, int size) {
            auto* buf = static_cast<std::vector<unsigned char>*>(ctx);
            auto* bytes = static_cast<unsigned char*>(data);
            buf->insert(buf->end(), bytes, bytes + size);
        },
        &outPng,
        width,
        height,
        channels,
        image.data(),
        stride
    );
}

} // namespace


PDFQRProcessor::PDFQRProcessor(const ExtractorConfig& cfg)
    : m_cfg(cfg)
{
}

bool PDFQRProcessor::extract(const std::string& pdfData, std::string& outCombinedResult, uint64_t rid)
{
    outCombinedResult.clear();
    std::vector<std::string> collectedQrs;

    // Enforce MAX_PDF_SIZE_QR
    if (pdfData.size() > m_cfg.m_maxPdfSizeKiloBytes) {        
        LOG_ERROR ("PROCESSOR", "[INFO] PDF exceeds max size, skipping QR scan");
        return true;
    }

    poppler::byte_array bytes(pdfData.begin(), pdfData.end());

    std::unique_ptr<poppler::document> document(poppler::document::load_from_data(&bytes));

    if (!document || document->is_locked()) {        
        LOG_ERROR ("PROCESSOR", "[WARN] Unable to load PDF or PDF is locked");
        return true;
    }

    const int totalPages = document->pages();

    poppler::page_renderer renderer;

    // Enforce MAX_PAGE_SINGLE_PDF
    if (totalPages > MAX_PAGE_SINGLE_PDF) {        
        LOG_ERROR ("PROCESSOR",  "PDF has " + std::to_string(totalPages) + " pages, exceeds max pages for single pdf= "  + std::to_string(MAX_PAGE_SINGLE_PDF)  + ", skipping QR scan");
        return true;
    }
    
    const int pagesToScan = std::min(totalPages, static_cast<int>(m_cfg.m_maxPagesToScan));       

    for (int i = 0; i < pagesToScan; ++i) {       
        
        std::unique_ptr<poppler::page> page(document->create_page(i));        
        LOG_INFO("PROCESSOR", "RID=" + std::to_string(rid) + " start");
        if (!page) {           
            LOG_ERROR ("PROCESSOR", "[WARN] Invalid page, skipping");
            continue;
        }

        auto rect = page->page_rect();
        if (rect.width() > MAX_PAGE_WIDTH ||
            rect.height() > MAX_PAGE_HEIGHT) {
            LOG_ERROR ("PROCESSOR", "[INFO] Page dimensions exceed limits, skipping page");
            continue;  // page auto-freed
        }

        auto image = renderer.render_page(page.get());        

        if (!image.is_valid() ||
            image.width() == 0 ||
            image.height() == 0) {            
            LOG_ERROR ("PROCESSOR", "[WARN] Rendered image invalid, skipping page");
            continue;
        }

        std::vector<unsigned char> pngData;
        renderPageToPng(image, pngData);

        if (pngData.empty() ||
            pngData.size() > m_cfg.m_maxQrImageKiloBytes) {            
            LOG_ERROR ("PROCESSOR", "[INFO] Rendered image exceeds QR size limit, skipping");
            continue;
        }

        // Decode ALL QR codes from this page
        auto pageQrs = decodeQrFromImage(pngData);

        for (auto& qr : pageQrs)
            collectedQrs.push_back(std::move(qr));
    }

    // Aggregate results (newline-delimited)
    for (const auto& qr : collectedQrs) {
        outCombinedResult.append(qr);
        outCombinedResult.push_back('\n');
    }
    LOG_ERROR("PROCESSOR", "RID=" + std::to_string(rid) +  " PDF processed successfully (" + std::to_string(pdfData.size()) + ")  bytes");
    return true;
}


std::vector<std::string> PDFQRProcessor::decodeQrFromImage(const std::vector<unsigned char>& pngData)
{
    std::vector<std::string> results;

    int width = 0, height = 0, channels = 0;
   
    using StbImagePtr = std::unique_ptr<unsigned char, decltype(&stbi_image_free)>;

    StbImagePtr img(
        stbi_load_from_memory(
            pngData.data(),
            static_cast<int>(pngData.size()),
            &width, &height, &channels,
            STBI_grey),
        stbi_image_free
    );

    if (!img) {        
        LOG_ERROR ("PROCESSOR", "[WARN] Failed to decode image");
        return results;
    }

    ZXing::ImageView view(img.get(), width, height, ZXing::ImageFormat::Lum);

    auto hints = ZXing::DecodeHints()
                     .setFormats(ZXing::BarcodeFormat::QRCode);

    auto decoded = ZXing::ReadBarcodes(view, hints);

    for (const auto& r : decoded)
        results.push_back(r.text());

    return results;
}

} // namespace extractor
