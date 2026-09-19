import re

header_path = 'src/presentation/GutterStrip.h'
with open(header_path, 'r') as f:
    content = f.read()
if '#include "DashboardButton.h"' not in content:
    content = content.replace('#include "GutterTab.h"', '#include "GutterTab.h"\n#include "DashboardButton.h"')
    content = content.replace('QVector<GutterTab*> m_tabs;', 'DashboardButton* m_dashboardBtn;\n    QVector<GutterTab*> m_tabs;')
    with open(header_path, 'w') as f:
        f.write(content)

cpp_path = 'src/presentation/GutterStrip.cpp'
with open(cpp_path, 'r') as f:
    content = f.read()

# Add instantiation in updateTabs()
if 'm_dashboardBtn = new DashboardButton' not in content:
    insert_idx = content.find('// Step 4: Instantiate and configure each tab widget.')
    
    dashboard_code = """
    // Add Dashboard button at the top
    m_dashboardBtn = new DashboardButton(this);
    m_layout->addWidget(m_dashboardBtn);
    connect(m_dashboardBtn, &DashboardButton::clicked, m_controller, &domain::TabController::toggleDashboard);
    connect(m_dashboardBtn, &DashboardButton::hovered, this, [this]() {
        auto& config = infrastructure::ConfigManager::instance().config();
        m_collapseTimer->stop();
        if (m_controller->currentState() != domain::TabController::State::OPEN) {
            m_controller->setState(domain::TabController::State::PEEKING);
            m_dashboardBtn->animateToWidth(config.gutterPeekWidth);
            for (auto tab : m_tabs) {
                tab->animateToWidth(config.gutterHoverWidth);
            }
        }
    });
    connect(m_dashboardBtn, &DashboardButton::unhovered, this, [this]() {
        auto& config = infrastructure::ConfigManager::instance().config();
        if (m_controller->currentState() != domain::TabController::State::OPEN) {
            if (!m_dialogOpen) m_collapseTimer->start();
        }
    });
    
    // Add negative spacer before first tab
    m_layout->addSpacerItem(new QSpacerItem(0, -10, QSizePolicy::Fixed, QSizePolicy::Fixed));

"""
    content = content[:insert_idx] + dashboard_code + content[insert_idx:]
    
    # We also need to add m_dashboardBtn->deleteLater() in the cleanup phase
    cleanup_idx = content.find('m_tabs.clear();')
    content = content[:cleanup_idx] + 'm_dashboardBtn = nullptr;\n    ' + content[cleanup_idx:]
    
    with open(cpp_path, 'w') as f:
        f.write(content)

