// Copyright (C) 2026 Sophos Limited
// SPDX-License-Identifier: GPL-3.0-or-later
//
// This file is part of pdf-qr-extractor.
//
// pdf-qr-extractor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// pdf-qr-extractor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with pdf-qr-extractor. If not, see <https://www.gnu.org/licenses/>

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

#include <chrono>

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

// Helper: deadline check
inline bool isDeadlineExceeded(const std::chrono::steady_clock::time_point& deadline)
{
    return std::chrono::steady_clock::now() > deadline;
}

} // anonymous namespace

PDFQRProcessor::PDFQRProcessor(const ExtractorConfig& cfg)
    : m_cfg(cfg)
{
}

ExtractResult PDFQRProcessor::extract(const std::string& pdfData,
                                      uint64_t rid,
                                      std::chrono::steady_clock::time_point deadline)
{
    ExtractResult result;
    result.status = ExtractStatus::OK;

    std::vector<std::string> collectedQrs;
   
    // INPUT VALIDATION
    if (pdfData.empty()) {
        LOG_ERROR("PROCESSOR", "RID=" + std::to_string(rid) + " Empty PDF input");
        result.status = ExtractStatus::INVALID_INPUT;
        return result;
    }

    if (pdfData.size() > m_cfg.m_maxPdfSizeBytes) {
        LOG_ERROR("PROCESSOR", "RID=" + std::to_string(rid) + " PDF exceeds max size");
        result.status = ExtractStatus::INVALID_INPUT;
        return result;
    }
    
    // Processing Deadline check (early)
    if (isDeadlineExceeded(deadline)) {
        LOG_ERROR("PROCESSOR", "RID=" + std::to_string(rid) + " Deadline exceeded before processing");
        result.status = ExtractStatus::TIMEOUT;
        return result;
    }

    try {
        std::unique_ptr<poppler::document> document(
            poppler::document::load_from_raw_data(pdfData.data(),
                                                  static_cast<int>(pdfData.size())));

        if (!document) {
            LOG_WARN("PROCESSOR", "RID=" + std::to_string(rid) + " Corrupt or unreadable PDF");
            result.status = ExtractStatus::INVALID_INPUT;
            return result;
        }

        if (document->is_locked()) {
            LOG_WARN("PROCESSOR", "RID=" + std::to_string(rid) + " Password-protected PDF, skipping");
            result.status = ExtractStatus::INVALID_INPUT;
            return result;
        }

        const int totalPages = document->pages();

        if (totalPages <= 0) {
            LOG_ERROR("PROCESSOR", "RID=" + std::to_string(rid) + " PDF has no pages");
            result.status = ExtractStatus::INVALID_INPUT;
            return result;
        }

        if (totalPages > MAX_PAGE_SINGLE_PDF) {
            LOG_ERROR("PROCESSOR", "RID=" + std::to_string(rid) + " PDF exceeds max pages");
            result.status = ExtractStatus::INVALID_INPUT;
            return result;
        }

        const int pagesToScan = std::min(
            totalPages,
            static_cast<int>(m_cfg.m_maxPagesToScan));

        poppler::page_renderer renderer;

        // Main Processing Loop 
        for (int i = 0; i < pagesToScan; ++i) {

            // DEADLINE CHECK (per page)
            if (isDeadlineExceeded(deadline)) {
                LOG_ERROR("PROCESSOR",
                          "RID=" + std::to_string(rid) +
                          " Deadline exceeded at page " + std::to_string(i));
                result.status = ExtractStatus::TIMEOUT;
                return result;
            }

            //auto pageStart = std::chrono::steady_clock::now();  //Intentionally commented.

            std::unique_ptr<poppler::page> page(document->create_page(i));

            if (!page) {
                LOG_WARN("PROCESSOR", "RID=" + std::to_string(rid) + " Invalid page, skipping");
                continue;
            }

            auto rect = page->page_rect();

            if (rect.width() > MAX_PAGE_WIDTH ||
                rect.height() > MAX_PAGE_HEIGHT) {
                LOG_WARN("PROCESSOR", "RID=" + std::to_string(rid) + " Page too large, skipping");
                continue;
            }

            // DEADLINE CHECK before render
            if (isDeadlineExceeded(deadline)) {
                result.status = ExtractStatus::TIMEOUT;
                return result;
            }

            auto image = renderer.render_page(page.get());

            if (!image.is_valid() ||
                image.width() == 0 ||
                image.height() == 0) {
                LOG_WARN("PROCESSOR", "RID=" + std::to_string(rid) + " Invalid rendered image, skipping");
                continue;
            }

            std::vector<unsigned char> pngData;
            renderPageToPng(image, pngData);

            if (pngData.empty() ||
                pngData.size() > m_cfg.m_maxQrImageBytes) {
                LOG_WARN("PROCESSOR", "RID=" + std::to_string(rid) + " Image invalid/too large, skipping");
                continue;
            }

            // DEADLINE CHECK before ZXing
            if (isDeadlineExceeded(deadline)) {
                result.status = ExtractStatus::TIMEOUT;
                return result;
            }

            auto pageQrs = decodeQrFromImage(pngData);

            for (auto& qr : pageQrs) {
                collectedQrs.push_back(std::move(qr));
            }

            //Intentionally left commented out for now - can be re-enabled if we want more granular per-page timing logs
            // // Optional: per-page time guard (soft warning)
            // auto pageDuration = std::chrono::steady_clock::now() - pageStart;

            // if (pageDuration > std::chrono::milliseconds(500)) {
            //     LOG_WARN("PROCESSOR",
            //              "RID=" + std::to_string(rid) +
            //              " Slow page processing: " +
            //              std::to_string(
            //                  std::chrono::duration_cast<std::chrono::milliseconds>(pageDuration).count()) +
            //              " ms");
            // }
        }

        // FINAL RESULT
        if (collectedQrs.empty()) {
            LOG_INFO("PROCESSOR", "RID=" + std::to_string(rid) + " No QR found");
            result.status = ExtractStatus::NO_QR;
            return result;
        }

        for (const auto& qr : collectedQrs) {
            result.data.append(qr);
            result.data.push_back('\n');
        }

        LOG_INFO("PROCESSOR",
                 "RID=" + std::to_string(rid) +
                 " QR extraction success, count=" +
                 std::to_string(collectedQrs.size()));

        result.status = ExtractStatus::OK;
        return result;
    }
    catch (const std::exception& e) {
        LOG_ERROR("PROCESSOR",
                  "RID=" + std::to_string(rid) +
                  " Exception: " + e.what());
        result.status = ExtractStatus::PROCESSING_ERROR;
        return result;
    }
    catch (...) {
        LOG_ERROR("PROCESSOR",
                  "RID=" + std::to_string(rid) +
                  " Unknown exception");
        result.status = ExtractStatus::PROCESSING_ERROR;
        return result;
    }
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
