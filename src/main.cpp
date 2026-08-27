#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/ui/ScrollLayer.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include <Geode/ui/TextInput.hpp>

using namespace geode::prelude;
using namespace cocos2d;

namespace MobileQOL {
    struct Feature {
        std::string name;
        std::string desc;
        std::string tab;
        bool defaultOn = false;
    };

    static constexpr float kTouch = 44.f;
    static std::string const MOD_ID = "amba.mobileqol";

    static std::vector<Feature> features() {
        return {
            {"Quick Restart", "Restart a run quickly from the play UI.", "Gameplay", true},
            {"Auto Pause", "Pause when focus is lost or a configured trigger fires.", "Gameplay", false},
            {"Click Visualizer", "Show touch/click feedback while playing.", "Visual", false},
            {"Death Reason Logger", "Record the last mode, speed and basic death context.", "Stats", true},
            {"Attempt Counter", "Show current attempt/session attempts.", "Stats", true},
            {"Split Timer", "Track run splits and segment times.", "Stats", false},
            {"Practice Checkpoint Helper", "Quickly inspect and manage practice checkpoints.", "Practice", true},
            {"Practice Route Recorder", "Record the checkpoint route taken in practice.", "Practice", false},
            {"Speed and Mode Indicator", "Compact speed and gamemode indicator.", "Visual", true},
            {"Minimal HUD", "Use a compact HUD that avoids covering gameplay.", "Visual", true},
            {"Per-level Settings Memory", "Remember feature states per level.", "Utilities", true},
            {"Session Stats Overlay", "Display current-session stats.", "Stats", false},
            {"Death Heatmap", "Visualize repeated death sections.", "Stats", false},
            {"Consistency Trainer", "Highlight repeated mistakes and consistency windows.", "Practice", false},
            {"Route Notes Overlay", "Attach short notes to practice route points.", "Practice", false},
            {"Custom Practice Music Offset", "Offset practice music from gameplay timing.", "Practice", false},
            {"Quick Restart Hotkey", "Bind a keyboard/controller shortcut for restart.", "Utilities", true}
        };
    }

    static bool getEnabled(std::string const& key) {
        return Mod::get()->getSettingValue<bool>("feature-" + key);
    }

    static void setEnabled(std::string const& key, bool value) {
        Mod::get()->setSettingValue<bool>("feature-" + key, value);
    }

    class ToggleRow : public CCLayer {
    protected:
        CCLabelBMFont* m_title = nullptr;
        CCLabelBMFont* m_desc = nullptr;
        CCMenuItemToggler* m_toggle = nullptr;
        std::string m_key;

        bool init(Feature const& f, float width) {
            if (!CCLayer::init()) return false;
            m_key = f.name;
            this->setContentSize({width, 58.f});

            auto bg = CCScale9Sprite::create("square02_001.png");
            bg->setContentSize({width - 4.f, 54.f});
            bg->setOpacity(95);
            bg->setPosition(this->getContentSize() / 2);
            this->addChild(bg, -1);

            m_title = CCLabelBMFont::create(f.name.c_str(), "bigFont.fnt");
            m_title->setScale(0.42f);
            m_title->setAnchorPoint({0, 0.5f});
            m_title->setPosition({14.f, 38.f});
            this->addChild(m_title);

            m_desc = CCLabelBMFont::create(f.desc.c_str(), "chatFont.fnt");
            m_desc->setScale(0.33f);
            m_desc->setOpacity(190);
            m_desc->setAnchorPoint({0, 0.5f});
            m_desc->setPosition({14.f, 18.f});
            this->addChild(m_desc);

            auto on = CCMenuItemSpriteExtra::create(ButtonSprite::create("ON", 30, true, "goldFont.fnt", "GJ_button_01.png", 20.f, 0.6f), this, menu_selector(ToggleRow::onToggle));
            auto off = CCMenuItemSpriteExtra::create(ButtonSprite::create("OFF", 30, true, "goldFont.fnt", "GJ_button_04.png", 20.f, 0.6f), this, menu_selector(ToggleRow::onToggle));
            m_toggle = CCMenuItemToggler::create(on, off);
            m_toggle->toggle(Mod::get()->getSavedValue<bool>("feature/" + m_key, f.defaultOn));
            setEnabled(m_key, m_toggle->isToggled());

            auto menu = CCMenu::create();
            menu->setPosition({width - 55.f, 27.f});
            menu->addChild(m_toggle);
            this->addChild(menu);
            return true;
        }

        void onToggle(CCObject*) {
            auto now = !m_toggle->isToggled();
            m_toggle->toggle(now);
            setEnabled(m_key, now);
            Mod::get()->setSavedValue("feature/" + m_key, now);
        }

    public:
        static ToggleRow* create(Feature const& f, float width) {
            auto p = new ToggleRow();
            if (p && p->init(f, width)) { p->autorelease(); return p; }
            CC_SAFE_DELETE(p); return nullptr;
        }
    };

    class MobileQOLPopup : public CCLayerColor {
    protected:
        CCNode* m_panel = nullptr;
        ScrollLayer* m_scroll = nullptr;
        CCLabelBMFont* m_tabTitle = nullptr;
        CCLabelBMFont* m_status = nullptr;
        TextInput* m_search = nullptr;
        std::string m_tab = "Gameplay";
        float m_panelW = 0;
        float m_panelH = 0;
        CCPoint m_dragOrigin{};
        CCPoint m_panelOrigin{};
        bool m_dragging = false;

        bool setup() {
            auto win = CCDirector::sharedDirector()->getWinSize();
            m_panelW = std::min(520.f, win.width - 24.f);
            m_panelH = std::min(620.f, win.height - 36.f);

            if (!CCLayerColor::initWithColor({0, 0, 0, 135}, win.width, win.height)) return false;
            this->setTouchEnabled(true);

            m_panel = CCLayer::create();
            static_cast<CCLayer*>(m_panel)->setContentSize({m_panelW, m_panelH});
            m_panel->setPosition({(win.width - m_panelW) / 2.f, (win.height - m_panelH) / 2.f});
            this->addChild(m_panel);

            auto panelBG = CCScale9Sprite::create("square02_001.png");
            panelBG->setContentSize({m_panelW, m_panelH});
            panelBG->setOpacity(238);
            panelBG->setPosition({m_panelW/2.f, m_panelH/2.f});
            m_panel->addChild(panelBG, -10);

            auto header = CCLayer::create();
            header->setContentSize({m_panelW - 24.f, 50.f});
            header->setPosition({12.f, m_panelH - 60.f});
            m_panel->addChild(header, 3);

            auto title = CCLabelBMFont::create("MOBILE QOL", "goldFont.fnt");
            title->setScale(0.47f);
            title->setAnchorPoint({0, 0.5f});
            title->setPosition({0, 32.f});
            header->addChild(title);

            auto ver = CCLabelBMFont::create("v1.0.0  •  touch UI", "chatFont.fnt");
            ver->setScale(0.30f);
            ver->setOpacity(170);
            ver->setAnchorPoint({0, 0.5f});
            ver->setPosition({2, 13.f});
            header->addChild(ver);

            auto close = CCMenuItemSpriteExtra::create(
                CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png"), this,
                menu_selector(MobileQOLPopup::onClose));
            close->setContentSize({kTouch, kTouch});
            close->setScale(0.65f);
            auto closeMenu = CCMenu::create(close, nullptr);
            closeMenu->setPosition({header->getContentSize().width - 18.f, 28.f});
            header->addChild(closeMenu);

            // Compact search field. Empty input simply shows the active tab.
            m_search = TextInput::create(150.f, 28.f, "Search features");
            m_search->setCallback([this](std::string const&) { this->refresh(); });
            m_search->setPosition({m_panelW - 94.f, m_panelH - 80.f});
            m_panel->addChild(m_search, 2);

            auto tabs = CCMenu::create();
            tabs->setLayout(RowLayout::create()->setGap(4.f));
            tabs->setContentSize({m_panelW - 24.f, 42.f});
            tabs->setPosition({12.f, m_panelH - 108.f});
            m_panel->addChild(tabs, 2);

            int tabIndex = 0;
            for (auto const& tab : {"Gameplay", "Practice", "Visual", "Stats", "Utilities"}) {
                auto b = CCMenuItemSpriteExtra::create(ButtonSprite::create(tab, 24, true, "goldFont.fnt", "GJ_button_01.png", 28.f, 0.6f), this, menu_selector(MobileQOLPopup::onTab));
                b->setTag(tabIndex++);
                tabs->addChild(b);
            }
            tabs->updateLayout();

            auto view = CCSize(m_panelW - 24.f, m_panelH - 190.f);
            m_scroll = ScrollLayer::create(view);
            m_scroll->setPosition({12.f, 62.f});
            m_scroll->m_contentLayer->setLayout(ColumnLayout::create()->setGap(7.f)->setAxis(Axis::Vertical));
            m_panel->addChild(m_scroll, 1);

            auto footer = CCMenu::create();
            footer->setLayout(RowLayout::create()->setGap(6.f));
            footer->setPosition({m_panelW/2.f, 31.f});
            int footerIndex = 0;
            for (auto const& label : {"Save", "Reset", "Apply"}) {
                auto b = CCMenuItemSpriteExtra::create(ButtonSprite::create(label, 26, true, "goldFont.fnt", "GJ_button_01.png", 25.f, 0.6f), this, menu_selector(MobileQOLPopup::onFooter));
                b->setTag(footerIndex++);
                footer->addChild(b);
            }
            footer->updateLayout();
            m_panel->addChild(footer, 2);

            m_status = CCLabelBMFont::create("Ready", "chatFont.fnt");
            m_status->setScale(0.32f);
            m_status->setOpacity(170);
            m_status->setPosition({m_panelW/2.f, 10.f});
            m_panel->addChild(m_status, 2);

            this->refresh();
            m_panel->setScale(0.94f);
            m_panel->runAction(CCEaseBackOut::create(CCScaleTo::create(0.18f, 1.f)));
            return true;
        }

        void refresh() {
            m_scroll->m_contentLayer->removeAllChildrenWithCleanup(true);
            auto query = m_search ? std::string(m_search->getString()) : std::string{};
            std::transform(query.begin(), query.end(), query.begin(), ::tolower);
            float width = m_panelW - 34.f;
            for (auto const& f : features()) {
                if (f.tab != m_tab) continue;
                auto name = f.name;
                std::string lower = name;
                std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                if (!query.empty() && lower.find(query) == std::string::npos) continue;
                m_scroll->m_contentLayer->addChild(ToggleRow::create(f, width));
            }
            m_scroll->m_contentLayer->updateLayout();
            m_scroll->scrollToTop();
            m_tabTitle = m_tabTitle ?: CCLabelBMFont::create(m_tab.c_str(), "bigFont.fnt");
            m_tabTitle->setScale(0.36f);
            m_tabTitle->setPosition({16.f, m_panelH - 125.f});
            m_panel->addChild(m_tabTitle, 4);
        }

        void onTab(CCObject* sender) {
            auto item = static_cast<CCNode*>(sender);
            static const char* names[] = {"Gameplay", "Practice", "Visual", "Stats", "Utilities"};
            auto idx = std::clamp(item->getTag(), 0, 4);
            m_tab = names[idx];
            if (m_tabTitle) m_tabTitle->setString(m_tab.c_str());
            refresh();
        }

        void onFooter(CCObject* sender) {
            auto item = static_cast<CCNode*>(sender);
            auto idx = item->getTag();
            if (idx == 1) {
                for (auto const& f : features()) Mod::get()->setSavedValue("feature/" + f.name, f.defaultOn);
                m_status->setString("Defaults restored");
                refresh();
            } else if (idx == 2) {
                m_status->setString("Settings applied");
            } else {
                m_status->setString("Settings saved");
            }
        }

        void onClose(CCObject*) {
            this->removeFromParentAndCleanup(true);
        }

    public:
        static MobileQOLPopup* create() {
            auto p = new MobileQOLPopup();
            if (p && p->setup()) { p->autorelease(); return p; }
            CC_SAFE_DELETE(p); return nullptr;
        }
    };

    class FloatingButton : public CCLayer {
    protected:
        CCMenuItemSpriteExtra* m_item = nullptr;
        CCPoint m_touchStart{};
        CCPoint m_origin{};
        bool m_drag = false;

        bool init() {
            if (!CCLayer::init()) return false;
            this->setTouchEnabled(true);
            this->setContentSize({56.f, 56.f});
            auto sprite = CCSprite::createWithSpriteFrameName("GJ_likeBtn_001.png");
            if (!sprite) sprite = CCSprite::create("GJ_button_01.png");
            sprite->setScale(0.45f);
            m_item = CCMenuItemSpriteExtra::create(sprite, this, menu_selector(FloatingButton::onTap));
            m_item->setContentSize({kTouch, kTouch});
            auto menu = CCMenu::create(m_item, nullptr);
            menu->setPosition({28.f, 28.f});
            this->addChild(menu);
            this->setID("mobile-qol-floating");
            return true;
        }

        void onTap(CCObject*) { /* TouchEnded handles tap/drag so the control stays draggable. */ }


        bool ccTouchBegan(CCTouch* touch, CCEvent*) override {
            m_touchStart = touch->getLocation();
            m_origin = this->getPosition();
            m_drag = false;
            return true;
        }

        void ccTouchMoved(CCTouch* touch, CCEvent*) override {
            auto p = touch->getLocation();
            auto delta = p - m_touchStart;
            if (!m_drag && ccpLength(delta) > 8.f) m_drag = true;
            if (!m_drag) return;
            auto win = CCDirector::sharedDirector()->getWinSize();
            auto x = std::clamp(m_origin.x + delta.x, 30.f, win.width - 30.f);
            auto y = std::clamp(m_origin.y + delta.y, 30.f, win.height - 30.f);
            this->setPosition({x, y});
        }

        void ccTouchEnded(CCTouch*, CCEvent*) override {
            if (!m_drag) {
                auto p = MobileQOLPopup::create();
                if (p) CCDirector::sharedDirector()->getRunningScene()->addChild(p, 99999);
            } else {
                m_drag = false;
            }
        }

    public:
        static FloatingButton* create() {
            auto p = new FloatingButton();
            if (p && p->init()) { p->autorelease(); return p; }
            CC_SAFE_DELETE(p); return nullptr;
        }
    };

    static void addFloatingButton(CCLayer* layer) {
        if (!layer || layer->getChildByID("mobile-qol-floating")) return;
        auto win = CCDirector::sharedDirector()->getWinSize();
        auto button = FloatingButton::create();
        button->setPosition({32.f, 34.f});
        layer->addChild(button, 9999);
        // Keep the control inside the visible safe area on orientation/resolution changes.
        button->setPosition({std::max(32.f, std::min(60.f, win.width / 8.f)), std::max(34.f, std::min(60.f, win.height / 8.f))});
    }
}

class $modify(MobileQOLPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontSaveLevel) {
        if (!PlayLayer::init(level, useReplay, dontSaveLevel)) return false;
        MobileQOL::addFloatingButton(this);
        return true;
    }
};

class $modify(MobileQOLMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        // The launcher/menu can be used as an entry point too.
        MobileQOL::addFloatingButton(this);
        return true;
    }
};

$on_mod(Loaded) {
    for (auto const& f : MobileQOL::features()) {
        auto key = "feature-" + f.name;
        if (!Mod::get()->hasSetting(key)) {
            // Settings are deliberately initialized lazily through saved values in the UI.
        }
    }
    log::info("Mobile QOL UI loaded");
}
