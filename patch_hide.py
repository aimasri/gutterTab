import re

cpp_path = 'src/presentation/BentoDashboard.cpp'
with open(cpp_path, 'r') as f:
    content = f.read()

new_hide = """void BentoDashboard::hideDashboard() {
    m_active = false;
    auto& config = infrastructure::ConfigManager::instance().config();
    bool isLeft = (config.edge == infrastructure::Config::Edge::Left);
    
    for (auto card : m_cards) {
        QRect r = card->geometry();
        if (isLeft) {
            r.moveRight(-50);
        } else {
            r.moveLeft(width() + 50);
        }
        card->animateToOrigin(r);
    }
"""

content = content.replace("void BentoDashboard::hideDashboard() {\n    m_active = false;\n    for (auto card : m_cards) {\n        card->animateToOrigin(m_originRect);\n    }", new_hide)

with open(cpp_path, 'w') as f:
    f.write(content)
