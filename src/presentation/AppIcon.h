#pragma once

#include <QIcon>
#include <QPixmap>

namespace presentation {

class AppIcon {
public:
    /**
     * @brief Renders the gutterTab brandmark application icon.
     * @return QIcon bundled with multi-resolution pixmaps.
     */
    static QIcon createAppIcon();

    /**
     * @brief Renders a single square pixmap of the icon at the requested size.
     * @param size Target width and height in pixels.
     * @return Antialiased QPixmap containing the icon.
     */
    static QPixmap createIconPixmap(int size);
};

} // namespace presentation
