/**
 * @file MarkdownEditor.cpp
 * @brief Implementation of MarkdownEditor with image attachment serialization and rendering.
 */

#include "MarkdownEditor.h"
#include "../infrastructure/ConfigManager.h"
#include <QDateTime>
#include <QFileInfo>
#include <QUrl>
#include <QTextDocumentFragment>

namespace presentation {

/**
 * @brief Constructs the MarkdownEditor widget.
 * @param parent Optional parent widget in the Qt object hierarchy.
 */
MarkdownEditor::MarkdownEditor(QWidget* parent) : QTextEdit(parent) {
    // Step 1: Initialize QTextEdit with rich-text capability enabled.
    // While our storage model is Markdown (via toMarkdown()), enabling rich text
    // is essential so that Qt's document layout engine can parse and render inline
    // HTML tags (such as <img> for image attachments) and format spans (bold, italic).
    setAcceptRichText(true);
}

/**
 * @brief Serializes a QImage to a timestamped PNG file in the active profile attachments directory.
 * @param image The bitmap image to save.
 * @return Absolute filepath if successfully written, or empty string on failure.
 */
QString MarkdownEditor::saveImageToDisk(const QImage& image) {
    // Step 1: Retrieve active profile ID from the singleton ConfigManager.
    auto& configMan = infrastructure::ConfigManager::instance();
    QString profileId = configMan.activeProfileId();
    if (profileId.isEmpty()) {
        return ""; // Edge case: No profile is currently active; abort write safely.
    }
    
    // Step 2: Determine attachment directory path: ~/.config/gutterTab/<profile>/attachments/
    QString dir = infrastructure::ConfigManager::getProfileAttachmentsPath(profileId);
    
    // Step 3: Construct a unique timestamped filename with millisecond resolution to avoid collisions.
    QString filename = "img_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz") + ".png";
    QString filepath = dir + "/" + filename;
    
    // Step 4: Save image data as PNG to disk and return file path.
    if (image.save(filepath, "PNG")) {
        return filepath;
    }
    return "";
}

/**
 * @brief Intercepts pasted or dropped MIME data to embed image content.
 * @param source Pointer to incoming QMimeData payload.
 */
void MarkdownEditor::insertFromMimeData(const QMimeData* source) {
    if (!source) {
        return; // Edge case: Guard against null MIME source pointers.
    }

    // Branch A: Direct bitmap image pasted from system clipboard (e.g., screenshot)
    if (source->hasImage()) {
        // Step 1: Extract QImage from MIME variant data.
        QImage image = qvariant_cast<QImage>(source->imageData());
        
        // Step 2: Persist image to profile attachments folder.
        QString filepath = saveImageToDisk(image);
        if (!filepath.isEmpty()) {
            // Step 3: Inject HTML <img> tag with local file URI.
            // QTextEdit's rich-text engine requires the file:// scheme to resolve local files.
            QString html = QString("<img src=\"file://%1\" />").arg(filepath);
            insertHtml(html);
            return;
        }
    } else if (source->hasUrls()) {
        // Branch B: File URL list pasted or dropped from file manager
        for (const QUrl& url : source->urls()) {
            if (url.isLocalFile()) {
                QFileInfo fi(url.toLocalFile());
                QString ext = fi.suffix().toLower();
                // Step 1: Validate file extension against supported raster image types.
                if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "gif") {
                    // Step 2: Directly reference existing local image file URI.
                    QString html = QString("<img src=\"file://%1\" />").arg(url.toLocalFile());
                    insertHtml(html);
                    return;
                }
            }
        }
    }
    
    // Branch C: Standard fallback for plain text, markdown text, or HTML pastes.
    QTextEdit::insertFromMimeData(source);
}

} // namespace presentation

