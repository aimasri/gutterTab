/**
 * @file MarkdownEditor.h
 * @brief Rich-text and Markdown editor component with automatic image attachment handling.
 */

#pragma once

#include <QTextEdit>
#include <QString>
#include <QMimeData>
#include <QImage>

namespace presentation {

/**
 * @brief Specialized text editor widget supporting Markdown authoring and clipboard image attachments.
 * 
 * @details MarkdownEditor subclasses `QTextEdit` to deliver an authoring surface tailored for
 *          gutterTab notes. When users paste raster image data from the system clipboard (e.g. from
 *          screenshots or image viewers) or drop local image files, this editor intercepts the MIME
 *          payload, writes the image out as a uniquely named PNG to disk under the active profile's
 *          attachment directory (`~/.config/gutterTab/<profile>/attachments/`), and automatically
 *          injects an HTML image tag (`<img src="file://..." />`) into the document. This enables
 *          QTextEdit's internal document layout engine to render images inline while maintaining
 *          compatibility with standard Markdown content serialization via `toMarkdown()`.
 * 
 * @note Must be instantiated and interacted with exclusively on Qt's main GUI thread.
 *       File persistence for pasted images is performed synchronously on the GUI thread;
 *       image operations rely on `infrastructure::ConfigManager` to resolve valid profile paths.
 */
class MarkdownEditor : public QTextEdit {
    Q_OBJECT
public:
    /**
     * @brief Constructs a new MarkdownEditor widget.
     * 
     * @param parent Optional parent widget in the Qt object tree. Defaults to nullptr.
     * 
     * @details Configures the editor to accept rich-text formatting, which is required
     *          for Qt's document layout engine to display embedded inline image tags
     *          and handle styling actions (bold, italic, lists).
     */
    explicit MarkdownEditor(QWidget* parent = nullptr);

protected:
    /**
     * @brief Custom MIME handler intercepting clipboard pastes and drop payloads.
     * 
     * @param source Pointer to the QMimeData container holding clipboard or drop contents.
     * 
     * @details Checks if the incoming payload contains raster image data (`hasImage()`) or
     *          local file URLs pointing to image extensions (.png, .jpg, .jpeg, .gif). If an image
     *          is detected, it persists the image to the profile attachment directory via
     *          `saveImageToDisk()` and inserts an HTML `<img>` tag with a local file URI.
     *          If no image is found or if saving fails, it delegates back to the base
     *          `QTextEdit::insertFromMimeData` to preserve standard plain/rich text behavior.
     * 
     * @note Handles edge cases including:
     *       - Null or invalid MIME sources.
     *       - Missing active profile id (aborts disk save safely).
     *       - Non-image clipboard payloads (gracefully falls back to base class).
     *       - Case-insensitive extension matching for file URLs.
     */
    void insertFromMimeData(const QMimeData* source) override;

private:
    /**
     * @brief Serializes a QImage to a timestamped PNG file in the active profile's attachments folder.
     * 
     * @param image The image object retrieved from clipboard MIME data.
     * @return Absolute file path string to the persisted image, or an empty QString on failure.
     * 
     * @details Queries `infrastructure::ConfigManager` for the active profile identifier and its
     *          corresponding attachments folder path (`~/.config/gutterTab/<profile>/attachments/`).
     *          Generates a unique filename formatted as `img_yyyyMMdd_HHmmss_zzz.png` to avoid
     *          name collisions across rapid paste sequences.
     * 
     * @note Handles edge cases including:
     *       - Empty active profile ID (returns empty string immediately without attempting disk I/O).
     *       - Filesystem write permission failures or non-existent directories (returns empty string).
     */
    QString saveImageToDisk(const QImage& image);
};

} // namespace presentation

