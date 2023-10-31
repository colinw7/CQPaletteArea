#include <CQPaletteArea.h>
#include <CQPaletteGroup.h>
#include <CQPalettePreview.h>

#include <CQSplitterArea.h>
#include <CQWidgetResizer.h>
#include <CQRubberBand.h>
#include <CQWidgetUtil.h>

#include <QApplication>
#include <QDesktopWidget>
#include <QMainWindow>
#include <QSplitter>
#include <QScrollArea>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QStylePainter>
#include <QStyleOption>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QTimer>
#include <QScreen>

#include <cassert>
#include <iostream>

#include <pin.xpm>
#include <unpin.xpm>
#include <left_triangle.xpm>
#include <right_triangle.xpm>

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

  for (int i = 0; i < 4; ++i) {
    Qt::DockWidgetArea dockArea = dockAreas[i];

    (void) createArea(dockArea);
  }

  rubberBand_ = new CQRubberBand;

  rubberBand_->hide();
}

CQPaletteAreaMgr::
~CQPaletteAreaMgr()
{
  for (Palettes::iterator p = palettes_.begin(); p != palettes_.end(); ++p) {
    Areas &areas = (*p).second;

    for (Areas::iterator pa = areas.begin(); pa != areas.end(); ++pa)
      delete *pa;
  }

  delete rubberBand_;
}

QString
CQPaletteAreaMgr::
dockAreaName(Qt::DockWidgetArea area) const
{
  switch (area) {
    case Qt::LeftDockWidgetArea  : return "leftArea";
    case Qt::RightDockWidgetArea : return "rightArea";
    case Qt::TopDockWidgetArea   : return "topArea";
    case Qt::BottomDockWidgetArea: return "bottomArea";
    default                      : return "noArea";
  }
}

CQPaletteArea *
CQPaletteAreaMgr::
getArea(Qt::DockWidgetArea dockArea)
{
  Areas &areas = palettes_[dockArea];

  for (Areas::iterator pa = areas.begin(); pa != areas.end(); ++pa) {
    if ((*pa)->isDetached()) continue;

    return *pa;
  }

  return createArea(dockArea);
}

CQPaletteArea *
CQPaletteAreaMgr::
createArea(Qt::DockWidgetArea dockArea)
{
  CQPaletteArea *area = new CQPaletteArea(this, dockArea);

  area->setObjectName(dockAreaName(dockArea));

  palettes_[dockArea].push_back(area);

  area->setVisible(false);

  return area;
}

void
CQPaletteAreaMgr::
deleteArea(CQPaletteArea *area)
{
  Areas &areas = palettes_[area->dockArea()];

  uint pos = 0;

  for ( ; pos < areas.size(); ++pos)
    if (areas[pos] == area)
      break;

  assert(pos < areas.size());

  for (uint i = pos + 1; i < areas.size(); ++i)
    areas[i - 1] = areas[i];

  areas.pop_back();

  area->setVisible(false);

  area->deleteLater();
}

void
CQPaletteAreaMgr::
addPage(CQPaletteAreaPage *page, Qt::DockWidgetArea dockArea)
{
  CQPaletteArea *area = getArea(dockArea);

  area->addPage(page);
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

  if (! group) {
    addPage(page, page->dockArea());

    group = page->group();
  }

  CQPaletteWindow *window = group->window();
  assert(window);

  window->showPage(page);

  window->setCurrentPage(page);
}

void
CQPaletteAreaMgr::
hidePage(CQPaletteAreaPage *page)
{
  CQPaletteGroup *group = page->group();

  if (! group) {
    addPage(page, page->dockArea());

    group = page->group();
  }

  CQPaletteWindow *window = group->window();
  assert(window);

  window->hidePage(page);
}

CQPaletteWindow *
CQPaletteAreaMgr::
addWindow(Qt::DockWidgetArea dockArea)
{
  CQPaletteArea *area = getArea(dockArea);

  return area->addWindow();
}

void
CQPaletteAreaMgr::
removeWindow(CQPaletteWindow *window)
{
  CQPaletteArea *area = window->area();

  if (area)
    area->removeWindow(window);
}

CQPaletteArea *
CQPaletteAreaMgr::
getAreaAt(const QPoint &pos, Qt::DockWidgetAreas allowedAreas) const
{
  for (Palettes::const_iterator p = palettes_.begin(); p != palettes_.end(); ++p) {
    Qt::DockWidgetArea  dockArea = (*p).first;
    const Areas        &areas    = (*p).second;

    // ensure we have an attach dock area
    bool hasAttached = false;

    for (Areas::const_iterator pa = areas.begin(); pa != areas.end(); ++pa) {
      CQPaletteArea *area = *pa;

      if (! area->isDetached()) {
        hasAttached = true;
        break;
      }
    }

    if (! hasAttached) {
      CQPaletteAreaMgr *th = const_cast<CQPaletteAreaMgr *>(this);

      CQPaletteArea *area = new CQPaletteArea(th, dockArea);

      area->setObjectName(dockAreaName(dockArea));

      th->palettes_[dockArea].push_back(area);

      area->setVisible(false);
    }

    // check all areas
    for (Areas::const_iterator pa = areas.begin(); pa != areas.end(); ++pa) {
      CQPaletteArea *area = *pa;

      if (area->isDetached() && ! area->isVisible()) continue;

      if (! (area->dockArea() & allowedAreas))
        continue;

      QRect rect = area->getHighlightRect();

      if (rect.contains(pos))
        return area;
    }
  }

  return nullptr;
}

void
CQPaletteAreaMgr::
swapAreas(CQPaletteArea *area1, CQPaletteArea *area2)
{
  // get dock area, name, and array position for each area
  Qt::DockWidgetArea dockArea1 = area1->dockArea();
  Qt::DockWidgetArea dockArea2 = area2->dockArea();

  QString name1 = area1->objectName();
  QString name2 = area2->objectName();

  Areas &areas1 = palettes_[dockArea1];
  Areas &areas2 = palettes_[dockArea2];

  uint pos1 = 0, pos2 = 0;

  for ( ; pos1 < areas1.size(); ++pos1)
    if (areas1[pos1] == area1)
      break;

  for ( ; pos2 < areas2.size(); ++pos2)
    if (areas2[pos2] == area2)
      break;

  assert(pos1 < areas1.size() && pos2 < areas2.size());

  //---

  // swap values
  area1->setDockArea(dockArea2);
  area2->setDockArea(dockArea1);

  area1->setObjectName(name2);
  area2->setObjectName(name1);

  palettes_[dockArea1][pos1] = area2;
  palettes_[dockArea2][pos2] = area1;

  //---

  // re-add to main window to make visible
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

int CQPaletteArea::windowId_ = 1;

CQPaletteArea::
CQPaletteArea(CQPaletteAreaMgr *mgr, Qt::DockWidgetArea dockArea) :
 CQDockArea(mgr->window()), mgr_(mgr), windowState_(NormalState), hideTitle_(true),
 visible_(true), expanded_(true), pinned_(true), floating_(false), detached_(false)
{
  setObjectName(mgr->dockAreaName(dockArea));

  setDockArea(dockArea);

  // remove title bar and lock in place
  title_ = new CQPaletteAreaTitle(this);

  title_->updateDockArea();

  setTitleBarWidget(title_);

  noTitle_ = new CQPaletteAreaNoTitle(this);

  setAllowedAreas(dockArea);

  if      (isVerticalDockArea())
    setFeatures(QDockWidget::DockWidgetFeatures());
  else if (isHorizontalDockArea())
    setFeatures(QDockWidget::DockWidgetVerticalTitleBar);

  // add to main window
  mgr_->window()->addDockWidget(dockArea, this);

  // add splitter
  splitter_ = new CQSplitterArea(this);

  splitter_->setObjectName("splitter_area");
  splitter_->setDockArea(this->dockArea());

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

  connect(this, SIGNAL(centralWidgetResized()), this, SLOT(updateSplitter()));

  updateTitle();
}

CQPaletteArea::
~CQPaletteArea()
{
  delete previewHandler_;
  delete noTitle_;
}

CQPaletteWindow *
CQPaletteArea::
getDockedWindow()
{
  for (uint i = 0; i < numWindows(); ++i)
    if (! windows_[i]->isDetached())
      return windows_[i];

  return nullptr;
}

CQPaletteWindow *
CQPaletteArea::
addWindow()
{
  auto *window = new CQPaletteWindow(this, uint(windowId_++));

  addWindow(window);

  return window;
}

void
CQPaletteArea::
addWindow(CQPaletteWindow *window)
{
  window->setArea(this);

  window->setVisible(true);

  if (! window->detachToArea())
    window->setDetached(false);
  else
    window->setWindowState(CQPaletteWindow::NormalState);

  window->setFloating(false);

  splitter()->splitter()->addWidget(window);

  updateSplitterSizes();

  for (uint i = 0; i < numWindows(); ++i)
    if (windows_[i] == window)
      assert(false);

  windows_.push_back(window);

  setVisible(true);

  updateTitle();

  updateSize();
}

void
CQPaletteArea::
addWindowAtPos(CQPaletteWindow *window, const QPoint &gpos)
{
  int tol = Constants::splitter_tol;

  window->setArea(this);

  int pos = -1;

  int n = splitter()->splitter()->count();

  if      (isVerticalDockArea()) {
    for (int i = 0; i < n; ++i) {
      QWidget *widget = splitter()->splitter()->widget(i);

      int y = widget->mapFromGlobal(gpos).y();
      int h = widget->height();

      if      (y > -tol && y < tol) {
        pos = i;
        break;
      }
      else if (y > h - tol && y < h + tol) {
        pos = i + 1;
        break;
      }
      else if (y >= tol && y <= h - tol) {
        CQPaletteWindow *window1 = qobject_cast<CQPaletteWindow *>(widget);

        auto numPages = window->group()->numPages();

        for (uint j = 0; j < numPages; ++j)
          window1->addPage(window->group()->getPage(int(j)));

        window1->setCurrentPage(window->group()->getPage(0));

        window->deleteLater();

        return;
      }
    }
  }
  else if (isHorizontalDockArea()) {
    for (int i = 0; i < n; ++i) {
      QWidget *widget = splitter()->splitter()->widget(i);

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

        auto numPages = window->group()->numPages();

        for (uint j = 0; j < numPages; ++j)
          window1->addPage(window->group()->getPage(int(j)));

        window->deleteLater();

        return;
      }
    }
  }

  window->setVisible(true);

  if (! window->detachToArea())
    window->setDetached(false);
  else
    window->setWindowState(CQPaletteWindow::NormalState);

  window->setFloating(false);

  if (pos != -1)
    splitter()->splitter()->insertWidget(pos, window);
  else
    splitter()->splitter()->addWidget(window);

  updateSplitterSizes();

  windows_.push_back(window);

  setVisible(true);

  updateTitle();

  updateSize();
}

void
CQPaletteArea::
removeWindow(CQPaletteWindow *window)
{
  int ind = -1;

  auto nw = numWindows();

  for (uint i = 0; i < nw; ++i) {
    if (ind < 0)
      ind = (windows_[i] == window ? int(i) : -1);
    else
      windows_[i - 1] = windows_[i];
  }

  assert(ind >= 0);

  windows_.pop_back();

  // remove from splitter
  window->setVisible(false);

  window->setParent(nullptr);

  window->setArea(nullptr);

  updateTitle();

  updateSize();
}

uint
CQPaletteArea::
numVisibleWindows() const
{
  uint num = 0;

  for (Windows::const_iterator p = windows_.begin(); p != windows_.end(); ++p) {
    CQPaletteWindow *window = *p;

    if (! window->isVisible() || window->isDetached())
      continue;

    ++num;
  }

  return num;
}

bool
CQPaletteArea::
isFirstWindow(const CQPaletteWindow *window) const
{
  int pos = splitter()->splitter()->indexOf(const_cast<CQPaletteWindow *>(window));

  return (pos == 0);
}

void
CQPaletteArea::
setVisible(bool visible)
{
  if (visible_ == visible)
    return;

  visible_ = visible;

  bool oldIgnoreSize = setIgnoreSize(true);

  CQDockArea::setVisible(visible);

  setIgnoreSize(oldIgnoreSize);
}

void
CQPaletteArea::
addPage(CQPaletteAreaPage *page, bool current)
{
  CQPaletteWindow *window = getDockedWindow();

  if (! window)
    window = addWindow();

  window->addPage(page);

  if (current)
    window->setCurrentPage(page);
}

void
CQPaletteArea::
updateSplitterSizes()
{
  QList<int> sizes;

  int n = splitter()->splitter()->count();

  if      (isVerticalDockArea()) {
    int h = splitter()->height();

    for (int i = 0; i < n; ++ i) {
      int h1 = h/(n - i);

      sizes.push_back(h1);

      h -= h1;
    }
  }
  else if (isHorizontalDockArea()) {
    int w = splitter()->width();

    for (int i = 0; i < n; ++ i) {
      int w1 = w/(n - i);

      sizes.push_back(w1);

      w -= w1;
    }
  }

  splitter()->splitter()->setSizes(sizes);
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
updateFloating(bool floating)
{
  splitter_->setFloating(floating);

  updateTitle();

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
    setFeatures(QDockWidget::DockWidgetFeatures());
  else if (isHorizontalDockArea())
    setFeatures(QDockWidget::DockWidgetVerticalTitleBar);

  title_->updateDockArea();

  splitter()->setDockArea(dockArea());

  for (Windows::iterator p = windows_.begin(); p != windows_.end(); ++p) {
    CQPaletteWindow *window = *p;

    window->updateDockArea();
    window->updateLayout();
  }

  splitter()->updateLayout();
}

void
CQPaletteArea::
expandSlot()
{
  if (expanded_) return;

  bool fixed = false;

  if      (isVerticalDockArea()) {
    int min_w, max_w;

    getDockMinMaxWidth(min_w, max_w);

    fixed = (min_w == max_w);

    if (! fixed)
      applyDockWidth(dockWidth(), false);
    else
      applyDockWidth(min_w, true);
  }
  else if (isHorizontalDockArea()) {
    int min_h, max_h;

    getDockMinMaxHeight(min_h, max_h);

    fixed = (min_h == max_h);

    if (! fixed)
      applyDockHeight(dockHeight(), false);
    else
      applyDockHeight(min_h, true);
  }

  splitter_->setResizable(! fixed);

  expanded_ = true;

  updateSizeConstraints();

  updateTitle();

  updatePreviewState();
}

void
CQPaletteArea::
collapseSlot()
{
  if (! expanded_) return;

  setCollapsedSize();

  expanded_ = false;

  updateSizeConstraints();

  updateTitle();

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
    applyDockWidth(w, true);
  else if (isHorizontalDockArea())
    applyDockHeight(w, true);

  splitter_->setResizable(false);
}

void
CQPaletteArea::
pinSlot()
{
  if (pinned_) return;

  pinned_ = true;

  updateTitle();

  updatePreviewState();
}

void
CQPaletteArea::
unpinSlot()
{
  if (! pinned_) return;

  pinned_ = false;

  updateTitle();

  updatePreviewState();
}

void
CQPaletteArea::
attachSlot()
{
  if (! detached_) return;

  setDetached(false);

  mgr_->window()->addDockWidget(dockArea(), this);
}

void
CQPaletteArea::
detachSlot()
{
  if (detached_) return;

  setDetached(true);

  int detachPos = getDetachPos(width(), height());

  move(detachPos, detachPos);
}

void
CQPaletteArea::
updateSizeConstraints()
{
  if (isFloating() || isDetached()) {
    int min_w, max_w;
    int min_h, max_h;

    getDockMinMaxWidth(min_w, max_w);
    getDockMinMaxHeight(min_h, max_h);

    if (isExpanded()) {
      CQWidgetUtil::setWidgetMinMaxWidth (this, min_w, max_w);
      CQWidgetUtil::setWidgetMinMaxHeight(this, min_h, max_h);
    }
    else {
      if (isVerticalDockArea()) {
        CQWidgetUtil::setWidgetFixedWidth  (this, dockWidth ());
        CQWidgetUtil::setWidgetMinMaxHeight(this, min_h, max_h);
      }
      else {
        CQWidgetUtil::setWidgetFixedHeight (this, dockHeight());
        CQWidgetUtil::setWidgetMinMaxWidth (this, min_w, max_w);
      }
    }
  }
  else {
    CQWidgetUtil::resetWidgetMinMaxWidth (this);
    CQWidgetUtil::resetWidgetMinMaxHeight(this);
  }
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

  if (detached_)
    setWindowState(DetachedState);
  else
    setWindowState(NormalState);

  setVisible(true);

  if (! detached_)
    setFloating(false);

  resizer_->setActive(detached_);

  updateSizeConstraints();

  updateTitle();

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

  updateSizeConstraints();

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

    setWindowState(FloatingState);

    if (! pos.isNull())
      move(pos - lpos);
    else {
      int detachPos = getDetachPos(width(), height());

      move(detachPos, detachPos);
    }

    allowedAreas_ = calcAllowedAreas();
  }
  else {
    setWindowState(NormalState);

    mgr_->window()->addDockWidget(dockArea(), this);
  }

  setFloating(floating);

  setVisible(true);
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
  if (! isDetached()) {
    setWindowState(NormalState);

    mgr_->window()->addDockWidget(dockArea(), this);

    setFloating(false);
  }
}

void
CQPaletteArea::
animateDrop(const QPoint &p)
{
  CQPaletteArea *area = mgr_->getAreaAt(p, allowedAreas());

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

        if (! isDetached())
          setVisible(false);
        else
          mgr_->deleteArea(this);
      }
    }
    else
      mgr_->window()->addDockWidget(area->dockArea(), area);
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

void
CQPaletteArea::
setWindowState(WindowState state)
{
  if (windowState_ == state)
    return;

  windowState_ = state;

  if      (windowState_ == NormalState)
    setParent(nullptr, Constants::normalFlags);
  else if (windowState_ == FloatingState)
    setParent(nullptr, Constants::floatingFlags);
  else if (windowState_ == DetachedState)
    setParent(nullptr, Constants::detachedFlags);
}

QRect
CQPaletteArea::
getHighlightRectAtPos(const QPoint &gpos) const
{
  if (isFloating())
    return getHighlightRect();

  int tol = Constants::splitter_tol;

  QRect    rect;
  QWidget *widget = nullptr;

  for (int i = 0; i < splitter()->splitter()->count(); ++i) {
    widget = splitter()->splitter()->widget(i);

    int w = std::max(widget->width (), 2*tol);
    int h = std::max(widget->height(), 2*tol);

    if      (isVerticalDockArea()) {
      int y = widget->mapFromGlobal(gpos).y();

      if      (y >    - tol && y <      tol) { rect = QRect(0,   - tol, w,     2*tol); break; }
      else if (y >  h - tol && y <  h + tol) { rect = QRect(0, h - tol, w,     2*tol); break; }
      else if (y >=     tol && y <= h - tol) { rect = QRect(0,     tol, w, h - 2*tol); break; }
    }
    else if (isHorizontalDockArea()) {
      int x = widget->mapFromGlobal(gpos).x();

      if      (x >    - tol && x <      tol) { rect = QRect(  - tol, 0,     2*tol, h); break; }
      else if (x >  w - tol && x <  w + tol) { rect = QRect(w - tol, 0,     2*tol, h); break; }
      else if (x >=     tol && x <= w - tol) { rect = QRect(tol    , 0, w - 2*tol, h); break; }
    }

    widget = nullptr;
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

  // main window geometry
  QRect wrect = mgr_->window()->geometry();

  int dx = wrect.left();
  int dy = wrect.top ();

  // main window central widget geometry
  QRect crect = mgr_->window()->centralWidget()->geometry();

  QRect rect;

  // visible docked area
  if (! isFloating() && numVisibleWindows() != 0) {
    rect = geometry();

    if (! isDetached())
      rect.adjust(dx, dy, dx, dy);

    if      (isVerticalDockArea()) {
      rect.setHeight(crect.height());

      if (rect.width() < 2*tol) rect.setWidth(2*tol);
    }
    else if (isHorizontalDockArea()) {
      rect.setWidth(wrect.height());

      if (rect.height() < 2*tol) rect.setHeight(2*tol);
    }
  }
  // floating or empty area
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

      this->removeWindow(window);

      area->addWindow(window);
    }

    mgr_->window()->addDockWidget(area->dockArea(), area);

    if (! isDetached())
      setVisible(false);
    else
      mgr_->deleteArea(this);
  }

  updateTitle();
}

// get position of detached area
int
CQPaletteArea::
getDetachPos(int w, int h) const
{
  static int detachPos = 16;

  QRect screenRect = CQWidgetUtil::desktopAvailableGeometry();

  if (detachPos + w >= screenRect.right () ||
      detachPos + h >= screenRect.bottom())
    detachPos = 16;

  int pos = detachPos;

  detachPos += 16;

  return pos;
}

void
CQPaletteArea::
updateTitle()
{
  QWidget *titleWidget = title_;

  if (hideTitle())
    titleWidget = noTitle_;

  if (titleWidget != titleBarWidget())
    setTitleBarWidget(titleWidget);

  for (Windows::iterator p = windows_.begin(); p != windows_.end(); ++p)
    (*p)->updateTitle();

  title_->updateState();
}

void
CQPaletteArea::
resizeEvent(QResizeEvent *e)
{
  CQDockArea::resizeEvent(e);

  updateSplitter();

  updatePreviewState();
}

void
CQPaletteArea::
moveEvent(QMoveEvent *e)
{
  CQDockArea::moveEvent(e);

  updateSplitter();
}

void
CQPaletteArea::
updateSplitter()
{
  splitter_->updateLayout();
}

void
CQPaletteArea::
updateSize()
{
  if (numVisibleWindows() == 0)
    setVisible(false);
  else {
    setVisible(true);

    QSize s = sizeHint();

    bool fixed = false;

    if      (isVerticalDockArea()) {
      int min_w, max_w;

      getDockMinMaxWidth(min_w, max_w);

      fixed = (min_w == max_w);

      if (fixed)
        applyDockWidth(min_w, true);
      else
        applyDockWidth(s.width(), false);
    }
    else if (isHorizontalDockArea()) {
      int min_h, max_h;

      getDockMinMaxHeight(min_h, max_h);

      fixed = (min_h == max_h);

      if (fixed)
        applyDockHeight(min_h, true);
      else
        applyDockHeight(s.height(), false);
    }

    splitter_->setResizable(! fixed);
  }
}

void
CQPaletteArea::
setSizeConstraints()
{
  bool fixed = false;

  if      (isVerticalDockArea()) {
    int min_w, max_w;

    getDockMinMaxWidth(min_w, max_w);

    fixed = (min_w == max_w);

    if (fixed)
      applyDockWidth(min_w, true);
    else
      applyDockWidth(dockWidth(), false);
  }
  else if (isHorizontalDockArea()) {
    int min_h, max_h;

    getDockMinMaxHeight(min_h, max_h);

    fixed = (min_h == max_h);

    if (fixed)
      applyDockHeight(min_h, true);
    else
      applyDockHeight(dockHeight(), false);
  }

  splitter_->setResizable(! fixed);
}

bool
CQPaletteArea::
moveSplitter(int d)
{
  if      (isVerticalDockArea()) {
    int min_w, max_w;

    getDockMinMaxWidth(min_w, max_w);

    int w = this->width();

    if      (dockArea() == Qt::LeftDockWidgetArea)
      w += d;
    else if (dockArea() == Qt::RightDockWidgetArea)
      w -= d;

    if (w < min_w || w > max_w)
      return false;

    applyDockWidth(w, false);
  }
  else if (isHorizontalDockArea()) {
    int min_h, max_h;

    getDockMinMaxHeight(min_h, max_h);

    int h = this->height();

    if      (dockArea() == Qt::TopDockWidgetArea)
      h += d;
    else if (dockArea() == Qt::BottomDockWidgetArea)
      h -= d;

    if (h < min_h || h > max_h)
      return false;

    applyDockHeight(h, false);
  }

  return true;
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

    if (! page->widthResizable()) {
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

    if (! page->heightResizable()) {
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
CQPaletteWindow(CQPaletteArea *area, uint id) :
 mgr_(area->mgr()), area_(area), id_(id), title_(nullptr), group_(nullptr), resizer_(nullptr),
 windowState_(NormalState), newWindow_(nullptr), parent_(nullptr), parentPos_(-1),
 detachToArea_(true), visible_(true), expanded_(true), floating_(false), detached_(false),
 allowedAreas_(), detachWidth_(0), detachHeight_(0)
{
  setObjectName(QString("window_%1").arg(id_));

  setFrameStyle(uint(QFrame::NoFrame) | uint(QFrame::Plain));
  setLineWidth(2);

  QGridLayout *layout = new QGridLayout(this);
  layout->setMargin(0); layout->setSpacing(0);

  title_ = new CQPaletteWindowTitle(this);
  group_ = CQPaletteGroupMgrInst->createGroup(this);

  group_->setObjectName(QString("group_%1").arg(id_));
  group_->tabbar()->setObjectName(QString("tabbar_%1").arg(id_));

  layout->addWidget(title_, 0, 0);
  layout->addWidget(group_, 1, 0);

  connect(group_, SIGNAL(currentPageChanged(CQPaletteAreaPage *)),
          this, SLOT(pageChangedSlot(CQPaletteAreaPage *)));

  resizer_ = new CQWidgetResizer(this);

  resizer_->setMovingEnabled(false);
  resizer_->setActive(false);
}

CQPaletteWindow::
~CQPaletteWindow()
{
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

Qt::DockWidgetArea
CQPaletteWindow::
dockArea() const
{
  return area_->dockArea();
}

bool
CQPaletteWindow::
isFirstArea() const
{
  return area_->isFirstWindow(this);
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

  if (group_->numPages() > 0) {
    setVisible(true);

    area_->updateSize();
  }
}

void
CQPaletteWindow::
hidePage(CQPaletteAreaPage *page)
{
  group_->hidePage(page);

  if (group_->numPages() == 0) {
    setVisible(false);

    area_->updateSize();
  }
}

void
CQPaletteWindow::
movePage(CQPaletteAreaPage *page, CQPaletteWindow *newWindow)
{
  group_->removePage(page);

  newWindow->group_->addPage(page);

  newWindow->group_->setCurrentPage(page);

  if (! group_->numPages()) {
    area_->removeWindow(this);

    QTimer::singleShot(10, this, SLOT(deleteLaterSlot()));
  }
}

void
CQPaletteWindow::
deleteLaterSlot()
{
  this->deleteLater();
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
setVisible(bool visible)
{
  visible_ = visible;

  QFrame::setVisible(visible);
}

void
CQPaletteWindow::
updateLayout()
{
  auto *l = qobject_cast<QGridLayout *>(layout());

  QLayoutItem *child;

  while ((child = l->takeAt(0)) != nullptr)
    delete child;

  l->addWidget(title_, 0, 0);

  if      (area_->isVerticalDockArea())
    l->addWidget(group_, 1, 0);
  else if (area_->isHorizontalDockArea())
    l->addWidget(group_, 0, 1);
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
  assert(! detachToArea());

  if (detached_ == detached)
    return;

  detached_ = detached;

  if (detached_)
    setWindowState(DetachedState);
  else
    setWindowState(NormalState);

  setVisible(true);

  if (! detached)
    setFloating(false);

  if (detached_)
    setFrameStyle(uint(QFrame::Panel) | uint(QFrame::Raised));
  else
    setFrameStyle(uint(QFrame::NoFrame) | uint(QFrame::Plain));

  updateTitle();

  resizer_->setActive(detached_);

  updateDetachSize();
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
      QSize saveSize = this->size();

      newWindow_ = area_->addWindow();

      parentPos_ = group_->currentIndex();

      for (uint i = 0; i < pages.size(); ++i) {
        if (pages[i] == currentPage) continue;

        removePage(pages[i]);

        newWindow_->addPage(pages[i]);
      }

      area_->updateSize();

      resize(saveSize);
    }
    else {
      newWindow_ = nullptr;
      parentPos_ = area_->splitter()->splitter()->indexOf(this); // whole window
    }

    parent_ = parentWidget();

    QPoint lpos = mapFromGlobal(pos);

    setWindowState(FloatingState);

    if (! pos.isNull())
      move(pos - lpos);
    else {
      int detachPos = area_->getDetachPos(width(), height());

      move(detachPos, detachPos);
    }

    if (newWindow_ == nullptr)
      area_->updateSize();
  }
  else {
    if (! isDetached())
      setWindowState(NormalState);
  }

  setFloating(floating);

  setVisible(true);
}

void
CQPaletteWindow::
cancelFloating()
{
  if (! newWindow_) {
    area_->splitter()->splitter()->insertWidget(parentPos_, this);

    area_->updateTitle();

    area_->updateSize();
  }
  else {
    CQPaletteAreaPage *currentPage = this->currentPage();

    removePage(currentPage);

    newWindow_->insertPage(parentPos_, currentPage);

    newWindow_->setCurrentPage(currentPage);

    area_->removeWindow(this);

    this->deleteLater();
  }

  if (! isDetached())
    setWindowState(NormalState);

  setFloating(false);
}

void
CQPaletteWindow::
detachToNewArea()
{
  setWindowState(NormalState);

  QPoint pos  = this->pos();
  QSize  size = this->size();

  CQPaletteArea *area = mgr_->createArea(dockArea());

  area_->removeWindow(this);

  area->addWindow(this);

  area->setDetached(true);

  area->move(pos);
  area->resize(size);

  area->updateTitle();
  area->updateSize();

  area_->updateTitle();
  area_->updateSize();
}

void
CQPaletteWindow::
setWindowState(WindowState state)
{
  if (windowState_ == state)
    return;

  windowState_ = state;

  if      (windowState_ == NormalState)
    setParent(area_, Constants::normalFlags);
  else if (windowState_ == FloatingState)
    setParent(nullptr, Constants::floatingFlags);
  else if (windowState_ == DetachedState)
    setParent(nullptr, Constants::detachedFlags);
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
execDrop(const QPoint &gpos, bool /*floating*/)
{
  CQPaletteArea *area = mgr_->getAreaAt(gpos, allowedAreas());

  if (area) {
    if (! detachToArea())
      setDetached(false);
    else
      setWindowState(NormalState);

    setFloating(false);

    area_->removeWindow(this);

    area->addWindowAtPos(this, gpos);

    if (! area->isDetached())
      mgr_->window()->addDockWidget(area->dockArea(), area);

    area->updateTitle();

    area->updateSize();
  }
  else {
    setFloating(false);

    if (detachToArea())
      detachToNewArea();
    else
      setDetached(true);

    setVisible(true);
  }

  clearDrop();
}

void
CQPaletteWindow::
clearDrop()
{
  mgr_->clearHighlight();
}

Qt::DockWidgetAreas
CQPaletteWindow::
calcAllowedAreas() const
{
  Qt::DockWidgetAreas allowedAreas = Qt::AllDockWidgetAreas;

  CQPaletteGroup::PageArray pages = getPages();

  for (CQPaletteGroup::PageArray::const_iterator p = pages.begin(); p != pages.end(); ++p) {
    CQPaletteAreaPage *page = *p;

    allowedAreas &= page->allowedAreas();
  }

  return allowedAreas;
}

void
CQPaletteWindow::
dockAt(Qt::DockWidgetArea dockArea)
{
  CQPaletteArea *area = mgr_->getArea(dockArea);

  CQPaletteAreaPage *page = this->currentPage();
  if (! page) return;

  removePage(page);

  area->addPage(page, true);
}

void
CQPaletteWindow::
dockLeftSlot()
{
  dockAt(Qt::LeftDockWidgetArea);
}

void
CQPaletteWindow::
dockRightSlot()
{
  dockAt(Qt::RightDockWidgetArea);
}

void
CQPaletteWindow::
dockTopSlot()
{
  dockAt(Qt::TopDockWidgetArea);
}

void
CQPaletteWindow::
dockBottomSlot()
{
  dockAt(Qt::BottomDockWidgetArea);
}

void
CQPaletteWindow::
togglePinSlot()
{
  if (area()->isPinned())
    area()->unpinSlot();
  else
    area()->pinSlot();

  updateTitle();
}

void
CQPaletteWindow::
toggleExpandSlot()
{
  bool isAreaTitle = isFirstArea();

  if (isAreaTitle || ! isDetached()) {
    if (area()->isExpanded())
      area()->collapseSlot();
    else
      area()->expandSlot();
  }
  else {
    if (isExpanded())
      collapseSlot();
    else
      expandSlot();
  }
}

void
CQPaletteWindow::
expandSlot()
{
  if (expanded_) return;

  if      (isVerticalDockArea()) {
    //CQWidgetUtil::resetWidgetMinMaxWidth(this);

    resize(detachWidth_, height());
  }
  else if (isHorizontalDockArea()) {
    //CQWidgetUtil::resetWidgetMinMaxHeight(this);

    resize(width(), detachHeight_);
  }

  expanded_ = true;

  updateTitle();
}

void
CQPaletteWindow::
collapseSlot()
{
  if (! expanded_) return;

  expanded_ = false;

  if      (isVerticalDockArea())
    setFixedWidth(dockWidth());
  else if (isHorizontalDockArea())
    setFixedHeight(dockHeight());

  updateTitle();
}

void
CQPaletteWindow::
attachSlot()
{
  if (! detached_) return;

  if (! detachToArea())
    setDetached(false);
  else
    setWindowState(NormalState);

  setFloating(false);

  area_->splitter()->splitter()->addWidget(this);

  area_->updateSplitterSizes();

  setVisible(true);

  area_->updateTitle();

  area_->updateSize();
}

void
CQPaletteWindow::
detachSlot()
{
  if (detached_) return;

  CQPaletteAreaPage *page = this->currentPage();
  if (! page) return;

  CQPaletteArea *area = area_;

  CQPaletteWindow *newWindow = area->addWindow();

  removePage(page);

  newWindow->addPage(page);

  int detachPos = area->getDetachPos(width(), height());

  newWindow->move(detachPos, detachPos);

  if (newWindow->detachToArea())
    newWindow->detachToNewArea();
  else
    newWindow->setDetached(true);

  if (! group_->numPages()) {
    area->removeWindow(this);

    QTimer::singleShot(10, this, SLOT(deleteLaterSlot()));
  }
}

void
CQPaletteWindow::
splitSlot()
{
  CQPaletteGroup::PageArray pages = getPages();

  if (pages.size() == 1) return;

  CQPaletteAreaPage *page = this->currentPage();

  if (! page) return;

  CQPaletteWindow *newWindow = area_->addWindow();

  removePage(page);

  newWindow->addPage(page);
}

void
CQPaletteWindow::
joinSlot()
{
  const CQPaletteArea::Windows &windows = area_->windows();

  if (windows.size() <= 1) return;

  CQPaletteWindow *joinWindow = nullptr;

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

    joinWindow->setCurrentPage(page);
  }

  this->deleteLater();
}

bool
CQPaletteWindow::
isDetachedNoArea() const
{
  if (! isDetached())
    return false;

  return ! detachToArea();
}

void
CQPaletteWindow::
closeSlot()
{
  CQPaletteAreaPage *page = currentPage();

  removePage(page);
}

void
CQPaletteWindow::
updateTitle()
{
  title_->updateState();
}

void
CQPaletteWindow::
resizeEvent(QResizeEvent *)
{
  if (! isDetached() || ! expanded_)
    return;

  updateDetachSize();
}

void
CQPaletteWindow::
updateDetachSize()
{
  if (isVerticalDockArea())
    detachWidth_  = width();
  else
    detachHeight_ = height();
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
 area_(area), contextMenu_(nullptr)
{
  pinButton_    = addButton(QPixmap(pin_data));
  expandButton_ = addButton(QPixmap(left_triangle_data));

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
    if      (area_->dockArea() == Qt::LeftDockWidgetArea ||
             area_->dockArea() == Qt::BottomDockWidgetArea)
      expandButton_->setIcon(QPixmap(left_triangle_data));
    else if (area_->dockArea() == Qt::RightDockWidgetArea ||
             area_->dockArea() == Qt::TopDockWidgetArea)
      expandButton_->setIcon(QPixmap(right_triangle_data));

    expandButton_->setToolTip("Collapse");
  }
  else {
    if      (area_->dockArea() == Qt::LeftDockWidgetArea ||
             area_->dockArea() == Qt::BottomDockWidgetArea)
      expandButton_->setIcon(QPixmap(right_triangle_data));
    else if (area_->dockArea() == Qt::RightDockWidgetArea ||
             area_->dockArea() == Qt::TopDockWidgetArea)
      expandButton_->setIcon(QPixmap(left_triangle_data));

    expandButton_->setToolTip("Expand");
  }

  //---

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

      if (! area_->isDetached())
        allowedAreas &= ~area_->dockArea();

      QMenu *menu = action->menu();

      QList<QAction *> actions1 = menu->actions();

      for (int j = 0; j < actions1.size(); ++j) {
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

CQPaletteAreaNoTitle::
CQPaletteAreaNoTitle(QWidget *parent) :
 QWidget(parent)
{
}

QSize
CQPaletteAreaNoTitle::
sizeHint() const
{
  return QSize(1, 1);
}

QSize
CQPaletteAreaNoTitle::
minimumSizeHint() const
{
  return QSize(1, 1);
}

//------

CQPaletteWindowTitle::
CQPaletteWindowTitle(CQPaletteWindow *window) :
 window_(window), contextMenu_(nullptr)
{
  pinButton_    = addButton(QPixmap(pin_data));
  expandButton_ = addButton(QPixmap(left_triangle_data));
  closeButton_  = addButton(style()->standardIcon(QStyle::SP_TitleBarCloseButton, nullptr, this));

  connect(pinButton_   , SIGNAL(clicked()), window_, SLOT(togglePinSlot()));
  connect(expandButton_, SIGNAL(clicked()), window_, SLOT(toggleExpandSlot()));
  connect(closeButton_ , SIGNAL(clicked()), window_, SLOT(closeSlot()));

  pinButton_   ->setToolTip("Pin");
  expandButton_->setToolTip("Expand");
  closeButton_ ->setToolTip("Close");

  setAttribute(Qt::WA_Hover);

  setFocusPolicy(Qt::NoFocus);

  setContextMenuPolicy(Qt::DefaultContextMenu);

  updateState();
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
updateState()
{
  bool isAreaTitle = window_->isFirstArea();

  //---

  bool isExpanded = (isAreaTitle ? window_->area()->isExpanded() : window_->isExpanded());

  //---

  if (isExpanded) {
    pinButton_->setVisible(isAreaTitle);

    if (window_->area()->isPinned()) {
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

  pinButton_->setEnabled(! window_->area()->isFloating() && ! window_->area()->isDetached());

  //---

  expandButton_->setVisible(isAreaTitle || window_->isDetached());

  if (isExpanded) {
    if      (window_->dockArea() == Qt::LeftDockWidgetArea ||
             window_->dockArea() == Qt::BottomDockWidgetArea)
      expandButton_->setIcon(QPixmap(left_triangle_data));
    else if (window_->dockArea() == Qt::RightDockWidgetArea ||
             window_->dockArea() == Qt::TopDockWidgetArea)
      expandButton_->setIcon(QPixmap(right_triangle_data));

    expandButton_->setToolTip("Collapse");
  }
  else {
    if      (window_->dockArea() == Qt::LeftDockWidgetArea ||
             window_->dockArea() == Qt::BottomDockWidgetArea)
      expandButton_->setIcon(QPixmap(right_triangle_data));
    else if (window_->dockArea() == Qt::RightDockWidgetArea ||
             window_->dockArea() == Qt::TopDockWidgetArea)
      expandButton_->setIcon(QPixmap(left_triangle_data));

    expandButton_->setToolTip("Expand");
  }

  //---

  updateLayout();
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

    QAction *expandAction = contextMenu_->addAction("Expand");
    QAction *attachAction = contextMenu_->addAction("Attach");
    QAction *detachAction = contextMenu_->addAction("Detach");
    QAction *splitAction  = contextMenu_->addAction("Split");
    QAction *joinAction   = contextMenu_->addAction("Join" );
    QAction *closeAction  = contextMenu_->addAction("Close");

    connect(expandAction, SIGNAL(triggered()), window_, SLOT(toggleExpandSlot()));
    connect(attachAction, SIGNAL(triggered()), window_, SLOT(attachSlot()));
    connect(detachAction, SIGNAL(triggered()), window_, SLOT(detachSlot()));
    connect(splitAction , SIGNAL(triggered()), window_, SLOT(splitSlot()));
    connect(joinAction  , SIGNAL(triggered()), window_, SLOT(joinSlot()));
    connect(closeAction , SIGNAL(triggered()), window_, SLOT(closeSlot()));
  }

  //------

  CQPaletteAreaPage *page = window_->currentPage();

  QList<QAction *> actions = contextMenu_->actions();

  for (int i = 0; i < actions.size(); ++i) {
    QAction *action = actions.at(i);

    const QString &text = action->text();

    if      (text == "Dock") {
      Qt::DockWidgetAreas allowedAreas;

      if (page)
        allowedAreas = page->allowedAreas();
      else
        allowedAreas = window_->calcAllowedAreas();

      if (! window_->isDetached() && ! window_->area()->isDetached())
        allowedAreas &= ~window_->area()->dockArea();

      QMenu *menu = action->menu();

      QList<QAction *> actions1 = menu->actions();

      for (int j = 0; j < actions1.size(); ++j) {
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
    else if (text == "Expand" || text == "Collapse") {
      bool isAreaTitle = window_->isFirstArea();

      action->setVisible(isAreaTitle || window_->isDetached());

      if (window_->isExpanded())
        action->setText("Collapse");
      else
        action->setText("Expand");
    }
    else if (text == "Attach")
      action->setVisible(window_->isDetached());
    else if (text == "Detach")
      action->setVisible(! window_->isDetached());
    else if (text == "Split")
      action->setVisible(window_->numPages() > 1);
    else if (text == "Join")
      action->setVisible(window_->area()->numWindows() > 1);
  }

  //------

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
