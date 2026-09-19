import re

cpp_path = 'src/presentation/BentoDashboard.cpp'
with open(cpp_path, 'r') as f:
    content = f.read()

new_layout = """void BentoDashboard::computeMasonryLayout() {
    int margin = 100;
    int gap = 16;
    int cols = 4; // fewer cols makes them larger
    if (m_cards.size() > 12) cols = 5;
    if (m_cards.size() > 20) cols = 6;
    
    // 1. Assign spans
    struct CardSpan { int w; int h; int r; int c; };
    QVector<CardSpan> spans(m_cards.size());
    QVector<QVector<bool>> grid(200, QVector<bool>(cols, false));
    int maxUsedRow = 0;
    
    for (int i = 0; i < m_cards.size(); ++i) {
        BentoCard* card = m_cards[i];
        int spanW = 1;
        int spanH = 1;
        
        QString plainText = m_controller->notes()[i].content;
        bool hasAttachment = plainText.contains(QRegularExpression("!\\\\[.*?\\\\]\\\\(.*?\\\\)"));
        
        // Stable pseudo-random generator based on note ID
        int randVal = ((card->noteId() * 9301 + 49297) % 233280) % 100;
        
        if (hasAttachment) {
            spanW = 2; spanH = 2;
        } else if (randVal < 20) {
            spanW = 2; spanH = 2;
        } else if (randVal < 50) {
            spanW = 2; spanH = 1;
        } else if (randVal < 80) {
            spanW = 1; spanH = 2;
        }
        
        // If cols is small, clamp spanW
        if (spanW > cols) spanW = cols;
        
        // Find free slot
        bool placed = false;
        for (int r = 0; r < grid.size() && !placed; ++r) {
            for (int c = 0; c <= cols - spanW && !placed; ++c) {
                bool free = true;
                for (int dr = 0; dr < spanH; ++dr) {
                    for (int dc = 0; dc < spanW; ++dc) {
                        if (r + dr >= grid.size() || grid[r + dr][c + dc]) {
                            free = false;
                            break;
                        }
                    }
                    if (!free) break;
                }
                
                if (free) {
                    for (int dr = 0; dr < spanH; ++dr) {
                        for (int dc = 0; dc < spanW; ++dc) {
                            grid[r + dr][c + dc] = true;
                            if (r + dr + 1 > maxUsedRow) maxUsedRow = r + dr + 1;
                        }
                    }
                    spans[i] = {spanW, spanH, r, c};
                    placed = true;
                }
            }
        }
    }
    
    if (maxUsedRow == 0) maxUsedRow = 1;
    
    QScreen* screen = QGuiApplication::primaryScreen();
    int screenWidth = screen ? screen->geometry().width() : width();
    int screenHeight = screen ? screen->geometry().height() : height();
    
    int availWidth = screenWidth - 2 * margin;
    int availHeight = screenHeight - 2 * margin;
    
    int unitW = (availWidth - (cols - 1) * gap) / cols;
    int unitH = (availHeight - (maxUsedRow - 1) * gap) / maxUsedRow;
    
    // Prevent cards from becoming too squished vertically if there are many rows
    if (unitH < 150) unitH = 150;
    
    // Re-center if we have few rows and it doesn't take up the whole screen height
    int totalH = maxUsedRow * unitH + (maxUsedRow - 1) * gap;
    int startY = margin;
    if (totalH < availHeight) {
        startY += (availHeight - totalH) / 2;
    }
    
    for (int i = 0; i < m_cards.size(); ++i) {
        int x = margin + spans[i].c * (unitW + gap);
        int y = startY + spans[i].r * (unitH + gap);
        int w = spans[i].w * unitW + (spans[i].w - 1) * gap;
        int h = spans[i].h * unitH + (spans[i].h - 1) * gap;
        m_cards[i]->setTargetGeometry(QRect(x, y, w, h));
    }
}"""

# Find the bounds of computeMasonryLayout
start_idx = content.find('void BentoDashboard::computeMasonryLayout() {')
end_idx = content.find('void BentoDashboard::showDashboard(', start_idx)

if start_idx != -1 and end_idx != -1:
    content = content[:start_idx] + new_layout + "\n\n" + content[end_idx:]
    with open(cpp_path, 'w') as f:
        f.write(content)
else:
    print("Could not find function bounds")
