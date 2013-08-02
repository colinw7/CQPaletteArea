#include <CQPaletteArea.h>
#include <CQPaletteGroup.h>
#include <CQPalettePreview.h>

#include <CQWidgetResizer.h>
#include <CQRubberBand.h>
#include <CQWidgetUtil.h>

#include <QApplication>
#include <QDesktopWidget>
#include <QMainWindow>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QSplitter>
#include <QStylePainter>
#include <QStyleOption>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QMenu>
#include <cassert>
#include <iostream>

#include <pin.xpm>
#include <unpin.xpm>

namespace Constants {
  int             splitter_tol  = 8;
  Qt::WindowFlags normalFlags   = Qt::Widget;
  Qt::WindowFlags floatingFlags = Qt::Tool | Qt::FramelessWindowHint |
                                  Qt::X11BypassWindowManagerHint;
  Qt::WindowFlags detachedFlags = Qt::Tool | Qt::FramelessWindowHint;
};

CQPaletteAreaMgr::
CQPaletteAreaMgr(QMainWindow *window) :
 window_(window)
{
  setObjectName("mgr");

  Qt::DockWidgetArea dockAreas[] = {
    Qt::LeftDockWidgetArea, Qt::RightDockWidgetArea,
    Qt::TopDockWidgetArea , Qt::BottomDockWidgetArea
  };

  QString dockAreaNames[] = { "leftArea", "rightArea", "topArea", "bottomArea" };

  for (int i = 0; i < 4; ++i) {
    Qt::DockWidgetArea dockArea = dockAreas[i];

    CQPaletteArea *area = new CQPaletteArea(this, dockArea);

    area->setObjectName(dockAreaNames[i]);

    palettes_[dockArea] = area;

    area->hide();
  }

  rubberBand_ = new CQRubberBand;

  rubberBand_->hide();
}

CQPaletteAreaMgr::
~CQPaletteAreaMgr()
{
  for (Palettes::const_iterator p = palettes_.begin(); p != palettes_.end(); ++p)
    delete (*p).second;

  delete rubberBand_;
}

void
CQPaletteAreaMgr::
addPage(CQPaletteAreaPage *page, Qt::DockWidgetArea dockArea)
{
  CQPaletteWindow *window = palettes_[dockArea]->getWindow();

  if (! window)
    window = palettes_[dockArea]->addWindow();

  window->addPage(page);
}

void
CQPaletteAreaMgr::
removePage(CQPaletteAreaPage *page)
{
  CQPaletteGroup *group = page->group();
  assert(group);

  CQPaletteWindow *window = group->window();
  assert(window);

  window->removePage(page);
}

void
CQPaletteAreaMgr::
showExpandedPage(CQPaletteAreaPage *page)
{
  CQPaletteGroup *group = page->group();
  assert(group);

  CQPaletteWindow *window = group->window();
  assert(window);

  window->showPage(page);
}

void
CQPaletteAreaMgr::
hidePage(CQPaletteAreaPage *page)
{
  CQPaletteGroup *group = page->group();
  assert(group);

  CQPaletteWindow *window = group->window();
  assert(window);

  window->hidePage(page);
}

CQPaletteWindow *
CQPaletteAreaMgr::
addWindow(Qt::DockWidgetArea dockArea)
{
  return palettes_[dockArea]->addWindow();
}

void
CQPaletteAreaMgr::
removeWindow(CQPaletteWindow *window)
{
  CQPaletteArea *area = window->area();

  if (area)
    palettes_[area->dockArea()]->removeWindow(window);
}

CQPaletteArea *
CQPaletteAreaMgr::
getArea(Qt::DockWidgetArea area) const
{
  Palettes::const_iterator p = palettes_.find(area);
  assert(p != palettes_.end());

  return (*p).second;
}

CQPaletteArea *
CQPaletteAreaMgr::
getAreaAt(const QPoint &pos, Qt::DockWidgetAreas allowedAreas) const
{
  for (Palettes::const_iterator p = palettes_.begin(); p != palettes_.end(); ++p) {
    CQPaletteArea *area = (*p).second;

    if (! (area->dockArea() & allowedAreas))
      continue;

    QRect rect = area->getHighlightRect();

    if (rect.contains(pos))
      return area;
  }

  return 0;
}

void
CQPaletteAreaMgr::
swapAreas(CQPaletteArea *area1, CQPaletteArea *area2)
{
  Qt::DockWidgetArea dockArea1 = area1->dockArea();
  Qt::DockWidgetArea dockArea2 = area2->dockArea();

  QString name1 = area1->objectName();
  QString name2 = area2->objectName();

  area1->setDockArea(dockArea2);
  area2->setDockArea(dockArea1);

  area1->setObjectName(name2);
  area2->setObjectName(name1);

  palettes_[area1->dockArea()] = area1;
  palettes_[area2->dockArea()] = area2;

  window()->addDockWidget(area1->dockArea(), area1);
  window()->addDockWidget(area2->dockArea(), area2);

  area1->updateDockArea();
  area2->updateDockArea();
}

void
CQPaletteAreaMgr::
highlightArea(CQPaletteArea *area, const QPoint &p)
{
  QRect rect = area->getHighlightRectAtPos(p);

  rubberBand_->setGeometry(rect);

  rubberBand_->show();
}

void
CQPaletteAreaMgr::
clearHighlight()
{
  rubberBand_->hide();
}

//------

CQPaletteArea::
CQPaletteArea(CQPaletteAreaMgr *mgr, Qt::DockWidgetArea dockArea) :
 CQDockArea(mgr->window()), mgr_(mgr), expanded_(true),
 pinned_(true), floating_(false), detached_(false)
{
  setObjectName("area");

  setDockArea(dockArea);

  // remove title bar and lock in place
  title_ = new CQPaletteAreaTitle(this);

  title_->updateDockArea();

  setTitleBarWidget(title_);

  setAllowedAreas(dockArea);

  if      (isVerticalDockArea())
    setFeatures(0);
  else if (isHorizontalDockArea())
    setFeatures(QDockWidget::DockWidgetVerticalTitleBar);

  // add to main window
  mgr_->window()->addDockWidget(dockArea, this);

  // add splitter
  splitter_ = new QSplitter;

  splitter_->setObjectName("splitter");

  if      (isVerticalDockArea())
    splitter_->setOrientation(Qt::Vertical);
  else if (isHorizontalDockArea())
    splitter_->setOrientation(Qt::Horizontal);

  setWidget(splitter_);

  resizer_ = new CQWidgetResizer(this);

  resizer_->setMovingEnabled(false);
  resizer_->setActive(false);

  previewHandler_ = new CQPalettePreview;

  connect(previewHandler_, SIGNAL(stopPreview()), this, SLOT(collapseSlot()));

  // attach to signals to monitor when dock area changed, floated and shown/hidden
  connect(this, SIGNAL(dockLocationChanged(Qt::DockWidgetArea)),
          this, SLOT(updateDockLocation(Qt::DockWidgetArea)));
  connect(this, SIGNAL(topLevelChanged(bool)), this, SLOT(updateFloating(bool)));
  connect(this, SIGNAL(visibilityChanged(bool)), this, SLOT(updateVisibility(bool)));

  connect(this, SIGNAL(resizeFinished()), this, SLOT(updateSizeConstraints()));
}

CQPaletteArea::
~CQPaletteArea()
{
  delete previewHandler_;
}

CQPaletteWindow *
CQPaletteArea::
getWindow(int i)
{
  if (i < 0 || i >= int(windows_.size()))
    return 0;

  return windows_[i];
}

CQPaletteWindow *
CQPaletteArea::
addWindow()
{
  CQPaletteWindow *window = new CQPaletteWindow(this);

  addWindow(window);

  return window;
}

void
CQPaletteArea::
addWindow(CQPaletteWindow *window)
{
  window->setArea(this);

  window->show();

  splitter_->addWidget(window);

  updateSplitterSizes();

  for (uint i = 0; i < windows_.size(); ++i)
    if (windows_[i] == window)
      assert(false);

  windows_.push_back(window);

  show();

  updateSize();
}

void
CQPaletteArea::
addWindowAtPos(CQPaletteWindow *window, const QPoint &gpos)
{
  int tol = Constants::splitter_tol;

  window->setArea(this);

  int pos = -1;

  int n = splitter_->count();

  if      (isVerticalDockArea()) {
    for (int i = 0; i < n; ++i) {
      QWidget *widget = splitter_->widget(i);

      int y = widget->mapFromGlobal(gpos).y();
      int h = widget->height();

      if      (y > -tol && y < tol) {
        pos = i;
        break;;
      }
      else if (y > h - tol && y < h + tol) {
        pos = i + 1;
        break;
      }
      else if (y >= tol && y <= h - tol) {
        CQPaletteWindow *window1 = qobject_cast<CQPaletteWindow *>(widget);

        int numPages = window->group()->numPages();

        for (int j = 0; j < numPages; ++j)
          window1->addPage(window->group()->getPage(j));

        window->deleteLater();

        return;
      }
    }
  }
  else if (isHorizontalDockArea()) {
    for (int i = 0; i < n; ++i) {
      QWidget *widget = splitter_->widget(i);

      int x = widget->mapFromGlobal(gpos).x();
      int w = widget->width();

      if      (x > -tol && x < tol) {
        pos = i;
        break;
      }
      else if (x > w - tol && x < w + tol) {
        pos = i + 1;
        break;
      }
      else if (x >= tol && x <= w - tol) {
        CQPaletteWindow *window1 = qobject_cast<CQPaletteWindow *>(widget);

        int numPages = window->group()->numPages();

        for (int j = 0; j < numPages; ++j)
          window1->addPage(window->group()->getPage(j));

        window->deleteLater();

        return;
      }
    }
  }

  window->show();

  if (pos != -1)
    splitter_->insertWidget(pos, window);
  else
    splitter_->addWidget(window);

  updateSplitterSizes();

  windows_.push_back(window);

  show();

  updateSize();
}

void
CQPaletteArea::
removeWindow(CQPaletteWindow *window)
{
  int ind = -1;

  int nw = windows_.size();

  for (int i = 0; i < nw; ++i) {
    if (ind < 0)
      ind = (windows_[i] == window ? i : -1);
    else
      windows_[i - 1] = windows_[i];
  }

  windows_.pop_back();

  // remove from splitter
  window->hide();

  window->setParent(0);

  window->setArea(0);

  updateSize();
}

void
CQPaletteArea::
updateSplitterSizes()
{
  QList<int> sizes;

  int n = splitter_->count();

  if      (isVerticalDockArea()) {
    int h = splitter_->height();

    for (int i = 0; i < n; ++ i) {
      int h1 = h/(n - i);

      sizes.push_back(h1);

      h -= h1;
    }
  }
  else if (isHorizontalDockArea()) {
    int w = splitter_->width();

    for (int i = 0; i < n; ++ i) {
      int w1 = w/(n - i);

      sizes.push_back(w1);

      w -= w1;
    }
  }

  splitter_->setSizes(sizes);
}

void
CQPaletteArea::
updateDockLocation(Qt::DockWidgetArea area)
{
  if (area != dockArea())
    std::cerr << "CQPaletteArea::updateDockLocation" << std::endl;
}

void
CQPaletteArea::
updateFloating(bool /*floating*/)
{
  title_->updateState();

  updatePreviewState();
}

// called when visibility changes (TODO: needed)
void
CQPaletteArea::
updateVisibility(bool)
{
  updatePreviewState();
}

void
CQPaletteArea::
updateDockArea()
{
  setAllowedAreas(dockArea_);

  if      (isVerticalDockArea())
    setFeatures(0);
  else if (isHorizontalDockArea())
    setFeatures(QDockWidget::DockWidgetVerticalTitleBar);

  title_->updateDockArea();

  if      (isVerticalDockArea())
    splitter_->setOrientation(Qt::Vertical);
  else if (isHorizontalDockArea())
    splitter_->setOrientation(Qt::Horizontal);

  for (Windows::iterator p = windows_.begin(); p != windows_.end(); ++p) {
    CQPaletteWindow *window = *p;

    window->updateDockArea();
    window->updateLayout();
  }
}

void
CQPaletteArea::
expandSlot()
{
  if (expanded_) return;

  if      (isVerticalDockArea()) {
    int min_w, max_w;

    getDockMinMaxWidth(min_w, max_w);

    bool fixed = (min_w == max_w);

    if (! fixed)
      setDockWidth(dockWidth(), false);
    else
      setDockWidth(min_w, true);
  }
  else if (isHorizontalDockArea()) {
    int min_h, max_h;

    getDockMinMaxHeight(min_h, max_h);

    bool fixed = (min_h == max_h);

    if (! fixed)
      setDockHeight(dockHeight(), false);
    else
      setDockHeight(min_h, true);
  }

  expanded_ = true;

  title_->updateState();

  updatePreviewState();
}

void
CQPaletteArea::
collapseSlot()
{
  if (! expanded_) return;

  setCollapsedSize();

  expanded_ = false;

  title_->updateState();

  updatePreviewState();
}

void
CQPaletteArea::
setCollapsedSize()
{
  int w = 1;

  for (Windows::iterator p = windows_.begin(); p != windows_.end(); ++p) {
    CQPaletteWindow *window = *p;

    if      (isVerticalDockArea())
      w = std::max(w, window->dockWidth());
    else if (isHorizontalDockArea())
      w = std::max(w, window->dockHeight());
  }

  if      (isVerticalDockArea())
    setDockWidth(w, true);
  else if (isHorizontalDockArea())
    setDockHeight(w, true);
}

void
CQPaletteArea::
pinSlot()
{
  if (pinned_) return;

  pinned_ = true;

  updatePreviewState();
}

void
CQPaletteArea::
unpinSlot()
{
  if (! pinned_) return;

  pinned_ = false;

  updatePreviewState();
}

void
CQPaletteArea::
attachSlot()
{
  if (! detached_) return;

  mgr_->window()->addDockWidget(dockArea(), this);

  setDetached(false);
}

void
CQPaletteArea::
detachSlot()
{
  if (detached_) return;

  setDetached(true);

  setParent(0, Constants::detachedFlags);

  int detachPos = getDetachPos(width(), height());

  move(detachPos, detachPos);

  show();
}

void
CQPaletteArea::
updatePreviewState()
{
  previewHandler_->setActive(expanded_ && ! pinned_ && ! isFloating() && ! isDetached());

  updatePreview();
}

void
CQPaletteArea::
updatePreview()
{
  // clear existing data
  previewHandler_->clear();

  // add preview widgets (for auto hide)
  for (Windows::iterator p = windows_.begin(); p != windows_.end(); ++p) {
    CQPaletteWindow *window = *p;

    CQPaletteAreaPage *page = window->currentPage();

    if (page)
      previewHandler_->addWidget(page->widget());
  }

  previewHandler_->addWidget(this);

  // add rectangle for whole palette and resize area
  QPoint p = mapToGlobal(rect().topLeft());

  //previewHandler_->addRect(QRect(p.x(), p.y(), width(), height()));

  int resizeSize = 7;

  if      (dockArea() == Qt::LeftDockWidgetArea)
    previewHandler_->addRect(QRect(p.x() + width() - 1, p.y(), resizeSize, height()));
  else if (dockArea() == Qt::RightDockWidgetArea)
    previewHandler_->addRect(QRect(p.x() - resizeSize, p.y(), resizeSize, height()));
  else if (dockArea() == Qt::BottomDockWidgetArea)
    previewHandler_->addRect(QRect(p.x(), p.y() - resizeSize, width(), resizeSize));
  else if (dockArea() == Qt::TopDockWidgetArea)
    previewHandler_->addRect(QRect(p.x(), p.y() + height() - 1, width(), resizeSize));
}

CQPaletteArea::Pages
CQPaletteArea::
getPages() const
{
  Pages pages;

  for (Windows::const_iterator p = windows_.begin(); p != windows_.end(); ++p) {
    CQPaletteWindow *window = *p;

    CQPaletteWindow::Pages pages1 = window->getPages();

    std::copy(pages1.begin(), pages1.end(), std::back_inserter(pages));
  }

  return pages;
}

// update detached state
void
CQPaletteArea::
setDetached(bool detached)
{
  if (detached_ == detached)
    return;

  detached_ = detached;

  resizer_->setActive(detached_);

  title_->updateState();

  updatePreviewState();
}

// update floating state
void
CQPaletteArea::
setFloating(bool floating)
{
  if (floating_ == floating)
    return;

  floating_ = floating;

  updatePreviewState();
}

void
CQPaletteArea::
setFloated(bool floating, const QPoint &pos, bool /*dragAll*/)
{
  if (floating == floating_)
    return;

  if (floating) {
    QPoint lpos = mapFromGlobal(pos);

    setParent(0, Constants::floatingFlags);

    if (! pos.isNull())
      move(pos - lpos);
    else {
      int detachPos = getDetachPos(width(), height());

      move(detachPos, detachPos);
    }

    allowedAreas_ = calcAllowedAreas();
  }
  else
    mgr_->window()->addDockWidget(dockArea(), this);

  setFloating(floating);

  show();
}

Qt::DockWidgetAreas
CQPaletteArea::
calcAllowedAreas() const
{
  Qt::DockWidgetAreas allowedAreas = Qt::AllDockWidgetAreas;

  Pages pages = getPages();

  for (Pages::const_iterator p = pages.begin(); p != pages.end(); ++p) {
    CQPaletteAreaPage *page = *p;

    allowedAreas &= page->allowedAreas();
  }

  return allowedAreas;
}

void
CQPaletteArea::
cancelFloating()
{
  mgr_->window()->addDockWidget(dockArea(), this);

  setFloating(false);
  setDetached(false);
}

void
CQPaletteArea::
animateDrop(const QPoint &p)
{
  CQPaletteArea *area = mgr_->getAreaAt(p, allowedAreas_);

  if (area)
    mgr_->highlightArea(area, p);
  else
    clearDrop();
}

void
CQPaletteArea::
execDrop(const QPoint &gpos, bool floating)
{
  CQPaletteArea *area = mgr_->getAreaAt(gpos, allowedAreas());

  if (area && (area != this || floating)) {
    setFloated (false);
    setDetached(false);

    if (area != this) {
      if (area->windows_.empty()) {
        mgr_->swapAreas(this, area);
      }
      else {
        // TODO: keep splitters ...
        Windows windows = windows_;

        for (Windows::iterator p = windows.begin(); p != windows.end(); ++p) {
          CQPaletteWindow *window = *p;

          this->removeWindow(window);

          area->addWindowAtPos(window, gpos);
        }

        mgr_->window()->addDockWidget(area->dockArea(), area);

        hide();
      }
    }
    else {
      mgr_->window()->addDockWidget(area->dockArea(), area);
    }
  }
  else {
    setFloating(false);
    setDetached(true);
  }

  clearDrop();
}

void
CQPaletteArea::
clearDrop()
{
  mgr_->clearHighlight();
}

QRect
CQPaletteArea::
getHighlightRectAtPos(const QPoint &gpos) const
{
  if (isFloating())
    return getHighlightRect();

  int tol = Constants::splitter_tol;

  QRect    rect;
  QWidget *widget = 0;

  for (int i = 0; i < splitter_->count(); ++i) {
    widget = splitter_->widget(i);

    int w = std::max(widget->width (), 2*tol);
    int h = std::max(widget->height(), 2*tol);

    if      (isVerticalDockArea()) {
      int y = widget->mapFromGlobal(gpos).y();

      if      (y >    - tol && y <      tol) { rect = QRect(0,   - tol, w,     2*tol); break;; }
      else if (y >  h - tol && y <  h + tol) { rect = QRect(0, h - tol, w,     2*tol); break; }
      else if (y >=     tol && y <= h - tol) { rect = QRect(0,     tol, w, h - 2*tol); break; }
    }
    else if (isHorizontalDockArea()) {
      int x = widget->mapFromGlobal(gpos).x();

      if      (x >    - tol && x <      tol) { rect = QRect(  - tol, 0,     2*tol, h); break; }
      else if (x >  w - tol && x <  w + tol) { rect = QRect(w - tol, 0,     2*tol, h); break; }
      else if (x >=     tol && x <= w - tol) { rect = QRect(tol    , 0, w - 2*tol, h); break; }
    }
  }

  if (! widget)
    return getHighlightRect();

  QPoint p = widget->mapToGlobal(QPoint(0, 0));

  rect.adjust(p.x(), p.y(), p.x(), p.y());

  return rect;
}

QRect
CQPaletteArea::
getHighlightRect() const
{
  int tol = Constants::splitter_tol;

  QRect wrect = mgr_->window()->geometry();

  int dx = wrect.left();
  int dy = wrect.top ();

  QRect crect = mgr_->window()->centralWidget()->geometry();

  QRect rect;

  if (! isFloating() && ! windows_.empty()) {
    rect = geometry();

    if (! isDetached())
      rect.adjust(dx, dy, dx, dy);

    if      (isVerticalDockArea()) {
      rect.setHeight(crect.height());

      if (rect.width() < 2*tol) rect.setWidth(2*tol);
    }
    else if (isHorizontalDockArea()) {
      rect.setWidth(crect.height());

      if (rect.height() < 2*tol) rect.setHeight(2*tol);
    }
  }
  else {
    // left/right are the height of the central widget and to the side of it
    if      (isVerticalDockArea()) {
      rect = crect;

      if (dockArea() == Qt::LeftDockWidgetArea)
        rect.setWidth(2*tol);
      else
        rect.adjust(rect.width() - 20, 0, 0, 0);

      rect.adjust(dx, dy, dx, dy);
    }
    // top/bottom are the width of the main window and to the side of the central widget
    else if (isHorizontalDockArea()) {
      rect = crect;

      rect.setX    (wrect.x());
      rect.setWidth(wrect.width());

      if (dockArea() == Qt::TopDockWidgetArea)
        rect.setHeight(2*tol);
      else
        rect.adjust(0, rect.height() - 20, 0, 0);

      rect.adjust(0, dy, 0, dy);
    }
  }

  return rect;
}

void
CQPaletteArea::
dockAt(Qt::DockWidgetArea dockArea)
{
  CQPaletteArea *area = mgr_->getArea(dockArea);

  if (area->windows_.empty()) {
    mgr_->swapAreas(this, area);
  }
  else {
    Windows windows = windows_;

    for (Windows::iterator p = windows.begin(); p != windows.end(); ++p) {
      CQPaletteWindow *window = *p;

      removeWindow(window);

      area->addWindow(window);
    }

    mgr_->window()->addDockWidget(area->dockArea(), area);

    hide();
  }
}

// get position of detached area
int
CQPaletteArea::
getDetachPos(int w, int h) const
{
  static int detachPos = 16;

  const QRect &screenRect = QApplication::desktop()->availableGeometry();

  if (detachPos + w >= screenRect.right () ||
      detachPos + h >= screenRect.bottom())
    detachPos = 16;

  int pos = detachPos;

  detachPos += 16;

  return pos;
}

void
CQPaletteArea::
resizeEvent(QResizeEvent *e)
{
  CQDockArea::resizeEvent(e);

  updatePreviewState();
}

void
CQPaletteArea::
updateSize()
{
  if (windows_.empty())
    hide();
  else {
    QSize s = sizeHint();

    if      (isVerticalDockArea())
      setDockWidth (s.width ());
    else if (isHorizontalDockArea())
      setDockHeight(s.height());
  }
}

void
CQPaletteArea::
setSizeConstraints()
{
  if      (isVerticalDockArea()) {
    int min_w, max_w;

    getDockMinMaxWidth(min_w, max_w);

    bool fixed = (min_w == max_w);

    if (fixed)
      setDockWidth(min_w, true);
    else
      setDockWidth(dockWidth(), false);
  }
  else if (isHorizontalDockArea()) {
    int min_h, max_h;

    getDockMinMaxHeight(min_h, max_h);

    bool fixed = (min_h == max_h);

    if (fixed)
      setDockHeight(min_h, true);
    else
      setDockHeight(dockHeight(), false);
  }
}

void
CQPaletteArea::
updateSizeConstraints()
{
  if (isFixed()) return;

  assert(isExpanded());

  if      (isVerticalDockArea()) {
    int min_w, max_w;

    getDockMinMaxWidth(min_w, max_w);

    CQWidgetUtil::setWidgetMinMaxWidth(this, min_w, max_w);
  }
  else if (isHorizontalDockArea()) {
    int min_h, max_h;

    getDockMinMaxHeight(min_h, max_h);

    CQWidgetUtil::setWidgetMinMaxHeight(this, min_h, max_h);
  }
}

void
CQPaletteArea::
getDockMinMaxWidth(int &min_w, int &max_w) const
{
  min_w = 0;
  max_w = QWIDGETSIZE_MAX;

  for (Windows::const_iterator p = windows_.begin(); p != windows_.end(); ++p) {
    CQPaletteWindow *window = *p;

    CQPaletteAreaPage *page = window->currentPage();
    if (! page) continue;

    int min_w1, max_w1;

    page->getMinMaxWidth(min_w1, max_w1);

    if (! page->resizable()) {
      min_w = min_w1;
      max_w = max_w1;

      break;
    }

    min_w = std::max(min_w, min_w1);
    max_w = std::min(max_w, max_w1);
  }
}

void
CQPaletteArea::
getDockMinMaxHeight(int &min_h, int &max_h) const
{
  min_h = 0;
  max_h = QWIDGETSIZE_MAX;

  for (Windows::const_iterator p = windows_.begin(); p != windows_.end(); ++p) {
    CQPaletteWindow *window = *p;

    CQPaletteAreaPage *page = window->currentPage();
    if (! page) continue;

    int min_h1, max_h1;

    page->getMinMaxHeight(min_h1, max_h1);

    if (! page->resizable()) {
      min_h = min_h1;
      max_h = max_h1;

      break;
    }

    min_h = std::max(min_h, min_h1);
    max_h = std::min(max_h, max_h1);
  }
}

QSize
CQPaletteArea::
sizeHint() const
{
  int w = 0;
  int h = 0;

  for (Windows::const_iterator p = windows_.begin(); p != windows_.end(); ++p) {
    CQPaletteWindow *window = *p;

    QSize s = window->sizeHint();

    if      (isVerticalDockArea()) {
      w  = std::max(w, s.width());
      h += s.height();
    }
    else if (isHorizontalDockArea()) {
      h  = std::max(h, s.height());
      w += s.width();
    }
  }

  return QSize(w, h);
}

//------

CQPaletteWindow::
CQPaletteWindow(CQPaletteArea *area) :
 mgr_(area->mgr()), area_(area), parent_(0), floating_(false), detached_(false)
{
  setObjectName("window");

  QGridLayout *layout = new QGridLayout(this);
  layout->setMargin(0); layout->setSpacing(0);

  title_ = new CQPaletteWindowTitle(this);
  group_ = new CQPaletteGroup(this);

  layout->addWidget(title_, 0, 0);
  layout->addWidget(group_, 1, 0);

  connect(group_, SIGNAL(currentPageChanged(CQPaletteAreaPage *)),
          this, SLOT(pageChangedSlot(CQPaletteAreaPage *)));

  resizer_ = new CQWidgetResizer(this);

  resizer_->setMovingEnabled(false);
  resizer_->setActive(false);
}

void
CQPaletteWindow::
setArea(CQPaletteArea *area)
{
  setParent(area);

  area_ = area;

  if (! area_) return;

  updateLayout();

  updateDockArea();
}

void
CQPaletteWindow::
addPage(CQPaletteAreaPage *page)
{
  group_->addPage(page);
}

void
CQPaletteWindow::
insertPage(int ind, CQPaletteAreaPage *page)
{
  group_->insertPage(ind, page);
}

void
CQPaletteWindow::
removePage(CQPaletteAreaPage *page)
{
  group_->removePage(page);

  if (! group_->numPages()) {
    area_->removeWindow(this);

    this->deleteLater();
  }
}

void
CQPaletteWindow::
showPage(CQPaletteAreaPage *page)
{
  group_->showPage(page);

  if (group_->numPages() > 0)
    setVisible(true);
}

void
CQPaletteWindow::
hidePage(CQPaletteAreaPage *page)
{
  group_->hidePage(page);

  if (group_->numPages() == 0)
    setVisible(false);
}

CQPaletteAreaPage *
CQPaletteWindow::
currentPage() const
{
  return group_->currentPage();
}

void
CQPaletteWindow::
setCurrentPage(CQPaletteAreaPage *page)
{
  group_->setCurrentPage(page);
}

void
CQPaletteWindow::
updateLayout()
{
  QGridLayout *l = qobject_cast<QGridLayout *>(layout());

  QLayoutItem *child;

  while ((child = l->takeAt(0)) != 0)
    delete child;

  if      (area_->isVerticalDockArea()) {
    l->addWidget(title_, 0, 0);
    l->addWidget(group_, 1, 0);
  }
  else if (area_->isHorizontalDockArea()) {
    l->addWidget(title_, 0, 0);
    l->addWidget(group_, 0, 1);
  }
}

void
CQPaletteWindow::
updateDockArea()
{
  title_->updateDockArea();
  group_->updateDockArea();
}

int
CQPaletteWindow::
dockWidth() const
{
  return group_->tabbar()->width();
}

int
CQPaletteWindow::
dockHeight() const
{
  return group_->tabbar()->height();
}

void
CQPaletteWindow::
expand()
{
}

void
CQPaletteWindow::
collapse()
{
}

QString
CQPaletteWindow::
getTitle() const
{
  CQPaletteAreaPage *page = currentPage();

  return (page ? page->title() : "");
}

QIcon
CQPaletteWindow::
getIcon() const
{
  CQPaletteAreaPage *page = currentPage();

  return (page ? page->icon() : QIcon());
}

void
CQPaletteWindow::
pageChangedSlot(CQPaletteAreaPage *)
{
  title_->update();

  if (area_->isExpanded())
    area_->setSizeConstraints();
}

CQPaletteWindow::Pages
CQPaletteWindow::
getPages() const
{
  CQPaletteGroup::PageArray pages;

  group()->getPages(pages);

  return pages;
}

uint
CQPaletteWindow::
numPages() const
{
  return group()->numPages();
}

// update detached state
void
CQPaletteWindow::
setDetached(bool detached)
{
  if (detached_ == detached)
    return;

  detached_ = detached;

  resizer_->setActive(detached_);
}

// update floating state
void
CQPaletteWindow::
setFloating(bool floating)
{
  if (floating_ == floating)
    return;

  floating_ = floating;
}

void
CQPaletteWindow::
setFloated(bool floating, const QPoint &pos, bool dragAll)
{
  if (floating == floating_)
    return;

  if (floating) {
    CQPaletteGroup::PageArray pages = getPages();

    CQPaletteAreaPage *currentPage = this->currentPage();

    if (currentPage)
      allowedAreas_ = currentPage->allowedAreas();

    if (! dragAll && pages.size() > 1) {
      newWindow_ = mgr_->addWindow(area_->dockArea());

      parentPos_ = group_->currentIndex();

      for (uint i = 0; i < pages.size(); ++i) {
        if (pages[i] == currentPage) continue;

        removePage(pages[i]);

        newWindow_->addPage(pages[i]);
      }
    }
    else {
      newWindow_ = 0;
      parentPos_ = area_->splitter()->indexOf(this); // whole window
    }

    parent_ = parentWidget();

    QPoint lpos = mapFromGlobal(pos);

    setParent(0, Constants::floatingFlags);

    if (! pos.isNull())
      move(pos - lpos);
    else {
      int detachPos = area_->getDetachPos(width(), height());

      move(detachPos, detachPos);
    }
  }
  else
    setParent(parent_, Constants::normalFlags);

  setFloating(floating);
  setDetached(false);

  show();
}

void
CQPaletteWindow::
cancelFloating()
{
  if (! newWindow_)
    area_->splitter()->insertWidget(parentPos_, this);
  else {
    CQPaletteAreaPage *currentPage = this->currentPage();

    removePage(currentPage);

    newWindow_->insertPage(parentPos_, currentPage);

    newWindow_->setCurrentPage(currentPage);

    mgr_->removeWindow(this);

    this->deleteLater();
  }

  setFloating(false);
  setDetached(false);
}

void
CQPaletteWindow::
animateDrop(const QPoint &p)
{
  CQPaletteArea *area = mgr_->getAreaAt(p, allowedAreas());

  if (area)
    mgr_->highlightArea(area, p);
  else
    clearDrop();
}

void
CQPaletteWindow::
execDrop(const QPoint &gpos, bool floating)
{
  CQPaletteArea *area = mgr_->getAreaAt(gpos, allowedAreas());

  if (area) {
    setFloated(false);

    area_->removeWindow(this);

    area->addWindowAtPos(this, gpos);

    if (! area->isDetached())
      mgr_->window()->addDockWidget(area->dockArea(), area);
  }
  else {
    setFloating(false);
    setDetached(true);

    setParent(0, Constants::detachedFlags);

    show();
  }

  clearDrop();
}

void
CQPaletteWindow::
clearDrop()
{
  mgr_->clearHighlight();
}

void
CQPaletteWindow::
dockLeftSlot()
{
  //dockAt(Qt::LeftDockWidgetArea);
}

void
CQPaletteWindow::
dockRightSlot()
{
  //dockAt(Qt::RightDockWidgetArea);
}

void
CQPaletteWindow::
dockTopSlot()
{
  //dockAt(Qt::TopDockWidgetArea);
}

void
CQPaletteWindow::
dockBottomSlot()
{
  //dockAt(Qt::BottomDockWidgetArea);
}

void
CQPaletteWindow::
attachSlot()
{
  if (! detached_) return;

  area_->splitter()->addWidget(this);

  setDetached(false);
}

void
CQPaletteWindow::
detachSlot()
{
  if (detached_) return;

  setDetached(true);

  setParent(0, Constants::detachedFlags);

  int detachPos = area_->getDetachPos(width(), height());

  move(detachPos, detachPos);

  show();
}

void
CQPaletteWindow::
splitSlot()
{
  CQPaletteGroup::PageArray pages = getPages();

  if (pages.size() == 1) return;

  CQPaletteAreaPage *currentPage = this->currentPage();

  if (! currentPage) return;

  CQPaletteWindow *newWindow = mgr_->addWindow(area_->dockArea());

  removePage(currentPage);

  newWindow->addPage(currentPage);
}

void
CQPaletteWindow::
joinSlot()
{
  const CQPaletteArea::Windows &windows = area_->getWindows();

  if (windows.size() <= 1) return;

  CQPaletteWindow *joinWindow = 0;

  for (CQPaletteArea::Windows::const_iterator p = windows.begin(); p != windows.end(); ++p) {
    CQPaletteWindow *window = *p;

    if (window == this) continue;

    if (! joinWindow || window->numPages() > joinWindow->numPages())
      joinWindow = window;
  }

  if (! joinWindow) return;

  CQPaletteGroup::PageArray pages = getPages();

  for (uint i = 0; i < pages.size(); ++i) {
    CQPaletteAreaPage *page = pages[i];

    removePage(page);

    joinWindow->addPage(page);
  }

  this->deleteLater();
}

void
CQPaletteWindow::
closeSlot()
{
  CQPaletteAreaPage *page = currentPage();

  removePage(page);
}

QSize
CQPaletteWindow::
sizeHint() const
{
  int w = 0;
  int h = 0;

  QSize ts = title_->sizeHint();
  QSize gs = group_->sizeHint();

  if      (area_->isVerticalDockArea()) {
    w = std::max(std::max(w, ts.width()), gs.width());
    h = gs.height() + ts.height();
  }
  else if (area_->isHorizontalDockArea()) {
    h = std::max(std::max(h, ts.height()), gs.height());
    w = gs.width() + ts.width();
  }

  return QSize(w, h);
}

//------

CQPaletteAreaTitle::
CQPaletteAreaTitle(CQPaletteArea *area) :
 area_(area), contextMenu_(0)
{
  pinButton_    = addButton(QPixmap(pin_data));
  expandButton_ = addButton(style()->standardIcon(QStyle::SP_TitleBarShadeButton, 0, this));

  connect(pinButton_   , SIGNAL(clicked()), this, SLOT(pinSlot()));
  connect(expandButton_, SIGNAL(clicked()), this, SLOT(expandSlot()));

  setAttribute(Qt::WA_Hover);

  setFocusPolicy(Qt::NoFocus);

  setContextMenuPolicy(Qt::DefaultContextMenu);

  updateState();
}

void
CQPaletteAreaTitle::
updateDockArea()
{
  if      (area_->isVerticalDockArea())
    setOrientation(Qt::Horizontal);
  else if (area_->isHorizontalDockArea())
    setOrientation(Qt::Vertical);
}

QString
CQPaletteAreaTitle::
title() const
{
  return "";
}

QIcon
CQPaletteAreaTitle::
icon() const
{
  return QIcon();
}

void
CQPaletteAreaTitle::
attachSlot()
{
  if (area_->isDetached())
    area_->attachSlot();
  else
    area_->detachSlot();

  updateState();
}

void
CQPaletteAreaTitle::
pinSlot()
{
  if (area_->isPinned())
    area_->unpinSlot();
  else
    area_->pinSlot();

  updateState();
}

void
CQPaletteAreaTitle::
expandSlot()
{
  if (area_->isExpanded())
    area_->collapseSlot();
  else
    area_->expandSlot();

  updateState();
}

void
CQPaletteAreaTitle::
dockLeftSlot()
{
  area_->dockAt(Qt::LeftDockWidgetArea);
}

void
CQPaletteAreaTitle::
dockRightSlot()
{
  area_->dockAt(Qt::RightDockWidgetArea);
}

void
CQPaletteAreaTitle::
dockTopSlot()
{
  area_->dockAt(Qt::TopDockWidgetArea);
}

void
CQPaletteAreaTitle::
dockBottomSlot()
{
  area_->dockAt(Qt::BottomDockWidgetArea);
}

void
CQPaletteAreaTitle::
updateState()
{
  if (area_->isExpanded()) {
    pinButton_->setVisible(true);

    if (area_->isPinned()) {
      pinButton_->setIcon(QPixmap(unpin_data));

      pinButton_->setToolTip("Unpin");
    }
    else {
      pinButton_->setIcon(QPixmap(pin_data));

      pinButton_->setToolTip("Pin");
    }
  }
  else
    pinButton_->setVisible(false);

  pinButton_->setEnabled(! area_->isFloating() && ! area_->isDetached());

  if (area_->isExpanded()) {
    expandButton_->setIcon(style()->standardIcon(QStyle::SP_TitleBarUnshadeButton, 0, this));

    expandButton_->setToolTip("Collapse");
  }
  else {
    expandButton_->setIcon(style()->standardIcon(QStyle::SP_TitleBarShadeButton, 0, this));

    expandButton_->setToolTip("Expand");
  }

  updateLayout();
}

void
CQPaletteAreaTitle::
contextMenuEvent(QContextMenuEvent *e)
{
  if (! contextMenu_) {
    contextMenu_ = new QMenu(this);

    QMenu *dockMenu = contextMenu_->addMenu("Dock");

    QAction *dockLeftItem   = dockMenu->addAction("Left");
    QAction *dockRightItem  = dockMenu->addAction("Right");
    QAction *dockTopItem    = dockMenu->addAction("Top");
    QAction *dockBottomItem = dockMenu->addAction("Bottom");

    connect(dockLeftItem  , SIGNAL(triggered()), this, SLOT(dockLeftSlot()));
    connect(dockRightItem , SIGNAL(triggered()), this, SLOT(dockRightSlot()));
    connect(dockTopItem   , SIGNAL(triggered()), this, SLOT(dockTopSlot()));
    connect(dockBottomItem, SIGNAL(triggered()), this, SLOT(dockBottomSlot()));

    QAction *attachAction = contextMenu_->addAction("Attach");
    QAction *pinAction    = contextMenu_->addAction("Pin");
    QAction *expandAction = contextMenu_->addAction("Expand");

    connect(attachAction, SIGNAL(triggered()), this, SLOT(attachSlot()));
    connect(pinAction   , SIGNAL(triggered()), this, SLOT(pinSlot()));
    connect(expandAction, SIGNAL(triggered()), this, SLOT(expandSlot()));
  }

  //----

  QList<QAction *> actions = contextMenu_->actions();

  for (int i = 0; i < actions.size(); ++i) {
    QAction *action = actions.at(i);

    const QString &text = action->text();

    if      (text == "Dock") {
      Qt::DockWidgetAreas allowedAreas = area_->calcAllowedAreas();

      allowedAreas &= ~area_->dockArea();

      QMenu *menu = action->menu();

      QList<QAction *> actions1 = menu->actions();

      for (int j = 0; j < actions.size(); ++j) {
        QAction *action1 = actions1.at(j);

        const QString &text1 = action1->text();

        if      (text1 == "Left") {
          action1->setEnabled(allowedAreas & Qt::LeftDockWidgetArea);
        }
        else if (text1 == "Right") {
          action1->setEnabled(allowedAreas & Qt::RightDockWidgetArea);
        }
        else if (text1 == "Top") {
          action1->setEnabled(allowedAreas & Qt::TopDockWidgetArea);
        }
        else if (text1 == "Bottom") {
          action1->setEnabled(allowedAreas & Qt::BottomDockWidgetArea);
        }
      }
    }
    else if (text == "Attach" || text == "Detach") {
      if (area_->isDetached())
        action->setText("Attach");
      else
        action->setText("Detach");
    }
    else if (text == "Pin" || text == "Unpin") {
      if (area_->isPinned())
        action->setText("Unpin");
      else
        action->setText("Pin");
    }
    else if (text == "Expand" || text == "Collapse") {
      if (area_->isExpanded())
        action->setText("Collapse");
      else
        action->setText("Expand");
    }
  }

  contextMenu_->popup(e->globalPos());

  e->accept();
}

void
CQPaletteAreaTitle::
mousePressEvent(QMouseEvent *e)
{
  mouseState_.reset();

  mouseState_.pressed  = true;
  mouseState_.pressPos = e->globalPos();
  mouseState_.floating = area_->isFloating();
  mouseState_.dragAll  = (e->modifiers() & Qt::ShiftModifier);

  setFocusPolicy(Qt::StrongFocus);
}

void
CQPaletteAreaTitle::
mouseMoveEvent(QMouseEvent *e)
{
  if (! mouseState_.pressed || mouseState_.escapePress) return;

  if (! mouseState_.moving &&
      (e->globalPos() - mouseState_.pressPos).manhattanLength() < QApplication::startDragDistance())
    return;

  mouseState_.moving = true;

  if (! area_->isFloating())
    area_->setFloated(true, e->globalPos(), mouseState_.dragAll);

  int dx = e->globalPos().x() - mouseState_.pressPos.x();
  int dy = e->globalPos().y() - mouseState_.pressPos.y();

  area_->move(area_->pos() + QPoint(dx, dy));

  area_->animateDrop(e->globalPos());

  mouseState_.pressPos = e->globalPos();
}

void
CQPaletteAreaTitle::
mouseReleaseEvent(QMouseEvent *e)
{
  if (! mouseState_.moving || mouseState_.escapePress)
    return;

  area_->execDrop(e->globalPos(), mouseState_.floating);

  mouseState_.reset();

  setFocusPolicy(Qt::NoFocus);
}

void
CQPaletteAreaTitle::
keyPressEvent(QKeyEvent *e)
{
  if (e->key() == Qt::Key_Escape && ! mouseState_.escapePress) {
    mouseState_.escapePress = true;

    area_->cancelFloating();

    area_->clearDrop();
  }
}

bool
CQPaletteAreaTitle::
event(QEvent *e)
{
  switch (e->type()) {
    case QEvent::HoverEnter:
      setCursor(Qt::SizeAllCursor);
      break;
    case QEvent::HoverLeave:
      unsetCursor();
      break;
    default:
      break;
  }

  return QWidget::event(e);
}

//------

CQPaletteWindowTitle::
CQPaletteWindowTitle(CQPaletteWindow *window) :
 window_(window), contextMenu_(0)
{
  closeButton_ = addButton(style()->standardIcon(QStyle::SP_TitleBarCloseButton, 0, this));

  connect(closeButton_, SIGNAL(clicked()), window_, SLOT(closeSlot()));

  closeButton_->setToolTip("Close");

  setAttribute(Qt::WA_Hover);

  setFocusPolicy(Qt::NoFocus);

  setContextMenuPolicy(Qt::DefaultContextMenu);
}

void
CQPaletteWindowTitle::
updateDockArea()
{
  if      (window_->area()->isVerticalDockArea())
    setOrientation(Qt::Horizontal);
  else if (window_->area()->isHorizontalDockArea())
    setOrientation(Qt::Vertical);
}

QString
CQPaletteWindowTitle::
title() const
{
  return window_->getTitle();
}

QIcon
CQPaletteWindowTitle::
icon() const
{
  return window_->getIcon();
}

void
CQPaletteWindowTitle::
contextMenuEvent(QContextMenuEvent *e)
{
  if (! contextMenu_) {
    contextMenu_ = new QMenu(this);

    // TODO: add pages

    QMenu *dockMenu = contextMenu_->addMenu("Dock");

    QAction *dockLeftItem   = dockMenu->addAction("Left");
    QAction *dockRightItem  = dockMenu->addAction("Right");
    QAction *dockTopItem    = dockMenu->addAction("Top");
    QAction *dockBottomItem = dockMenu->addAction("Bottom");

    connect(dockLeftItem  , SIGNAL(triggered()), window_, SLOT(dockLeftSlot()));
    connect(dockRightItem , SIGNAL(triggered()), window_, SLOT(dockRightSlot()));
    connect(dockTopItem   , SIGNAL(triggered()), window_, SLOT(dockTopSlot()));
    connect(dockBottomItem, SIGNAL(triggered()), window_, SLOT(dockBottomSlot()));

    QAction *attachAction = contextMenu_->addAction("Attach");
    QAction *detachAction = contextMenu_->addAction("Detach");
    QAction *splitAction  = contextMenu_->addAction("Split");
    QAction *joinAction   = contextMenu_->addAction("Join" );
    QAction *closeAction  = contextMenu_->addAction("Close");

    connect(attachAction, SIGNAL(triggered()), window_, SLOT(attachSlot()));
    connect(detachAction, SIGNAL(triggered()), window_, SLOT(detachSlot()));
    connect(splitAction , SIGNAL(triggered()), window_, SLOT(splitSlot()));
    connect(joinAction  , SIGNAL(triggered()), window_, SLOT(joinSlot()));
    connect(closeAction , SIGNAL(triggered()), window_, SLOT(closeSlot()));
  }

  contextMenu_->popup(e->globalPos());

  e->accept();
}

void
CQPaletteWindowTitle::
mousePressEvent(QMouseEvent *e)
{
  mouseState_.reset();

  mouseState_.pressed  = true;
  mouseState_.pressPos = e->globalPos();
  mouseState_.floating = window_->area()->isFloating();
  mouseState_.dragAll  = (e->modifiers() & Qt::ShiftModifier);

  if (window_->isDetached())
    mouseState_.dragAll = true;

  setFocusPolicy(Qt::StrongFocus);
}

void
CQPaletteWindowTitle::
mouseMoveEvent(QMouseEvent *e)
{
  if (! mouseState_.pressed || mouseState_.escapePress) return;

  if (! mouseState_.moving &&
      (e->globalPos() - mouseState_.pressPos).manhattanLength() < QApplication::startDragDistance())
    return;

  mouseState_.moving = true;

  if (! window_->isFloating())
    window_->setFloated(true, e->globalPos(), mouseState_.dragAll);

  int dx = e->globalPos().x() - mouseState_.pressPos.x();
  int dy = e->globalPos().y() - mouseState_.pressPos.y();

  window_->move(window_->pos() + QPoint(dx, dy));

  window_->animateDrop(e->globalPos());

  mouseState_.pressPos = e->globalPos();
}

void
CQPaletteWindowTitle::
mouseReleaseEvent(QMouseEvent *e)
{
  if (! mouseState_.moving || mouseState_.escapePress)
    return;

  window_->execDrop(e->globalPos(), mouseState_.floating);

  mouseState_.reset();

  setFocusPolicy(Qt::NoFocus);
}

void
CQPaletteWindowTitle::
keyPressEvent(QKeyEvent *e)
{
  if (e->key() == Qt::Key_Escape && ! mouseState_.escapePress) {
    mouseState_.escapePress = true;

    window_->cancelFloating();

    window_->clearDrop();
  }
}

bool
CQPaletteWindowTitle::
event(QEvent *e)
{
  switch (e->type()) {
    case QEvent::HoverEnter:
      setCursor(Qt::SizeAllCursor);
      break;
    case QEvent::HoverLeave:
      unsetCursor();
      break;
    default:
      break;
  }

  return QWidget::event(e);
}
