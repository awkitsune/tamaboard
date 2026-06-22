#include "PageStack.h"
#include "Header.h"
#include <Arduino.h>

Page *PageStack::currentPage() const
{
    if (_modal)
        return _modal;
    if (_roots.empty())
        return nullptr;
    return _roots[_rootIdx];
}

void PageStack::onShortPress()
{
    if (_modal || _entered)
    {
        if (auto *p = currentPage())
            p->onShortPress(*this);
    }
    else if (!_roots.empty())
    {
        _rootIdx = (_rootIdx + 1) % _roots.size();
        _display.markFullRefresh(); // root carousel: hard transition
        _dirty = true;
    }
}

void PageStack::onLongPress()
{
    if (_modal || _entered)
    {
        if (auto *p = currentPage())
            p->onLongPress(*this);
    }
    else if (!_roots.empty())
    {
        if (auto *p = currentPage())
            if (p->interceptsRootLongPress(*this))
                return;
        _entered = true;
        if (auto *p = currentPage())
            p->onEnter();
        _display.markFullRefresh(); // entering a page: hard transition
        _dirty = true;
    }
}

void PageStack::leave()
{
    if (_modal)
    {
        popModal();
        return;
    }
    if (_entered)
    {
        if (auto *p = currentPage())
            p->onLeave();
        _entered = false;
        _display.markFullRefresh(); // leaving a page: hard transition
        _dirty = true;
    }
}

void PageStack::goHome()
{
    if (_modal) { _modal->onLeave(); _modal = nullptr; }
    if (_entered) { if (auto *p = currentPage()) p->onLeave(); _entered = false; }
    _rootIdx = 0;
    _display.markFullRefresh();
    _dirty = true;
}

void PageStack::pushModal(Page *p)
{
    _modal = p;
    if (p)
        p->onEnter();
    _display.markFullRefresh(); // modal overlay: hard transition
    _dirty = true;
}

void PageStack::popModal()
{
    if (_modal)
    {
        _modal->onLeave();
        _modal = nullptr;
        _display.markFullRefresh(); // modal dismissed: hard transition
        _dirty = true;
    }
}

void PageStack::renderIfDirty()
{
    if (!_dirty)
        return;
    _dirty = false;

    Page *page = currentPage();
    if (!page)
        return;

    _display.draw([&](Canvas &c) {
        int yTop = drawHeader(c, page->title(), entered());
        page->render(c, yTop + 1);
    });

    Serial.println("[PageStack] rendered dirty page");
}
