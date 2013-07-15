#include <CQPaletteArea.h>
#include <CQPaletteGroup.h>

#include <QApplication>
#include <QMainWindow>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QSplitter>
#include <QStylePainter>
#include <QStyleOption>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QRubberBand>
#include <QMenu>
#include <cassert>
#include <iostream>

#include <pin.xpm>
#include <unpin.xpm>

namespace Constants {
  int splitter_tol = 8;
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

  rubberBand_ = new QRubberBand(QRubberBand::Rectangle);

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
 CQDockArea(mgr->window()), mgr_(mgr), dockArea_(dockArea), expanded_(true),
 pinned_(true), floating_(false)
{
  setObjectName("area");

  // remove title bar and lock in place
  title_ = new CQPaletteAreaTitle(this);

  title_->updateDockArea();

  setTitleBarWidget(title_);

  setAllowedAreas(dockArea);

  if (dockArea_ == Qt::LeftDockWidgetArea || dockArea_ == Qt::RightDockWidgetArea)
    setFeatures(0);
  else
    setFeatures(QDockWidget::DockWidgetVerticalTitleBar);

  // add to main window
  mgr_->window()->addDockWidget(dockArea, this);

  // add splitter
  splitter_ = new QSplitter;

  splitter_->setObjectName("splitter");

  if (dockArea_ == Qt::LeftDockWidgetArea || dockArea_ == Qt::RightDockWidgetArea)
    splitter_->setOrientation(Qt::Vertical);
  else
    splitter_->setOrientation(Qt::Horizontal);

  setWidget(splitter_);
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

  if (dockArea() == Qt::LeftDockWidgetArea || dockArea() == Qt::RightDockWidgetArea) {
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
  else {
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

  if (dockArea_ == Qt::LeftDockWidgetArea || dockArea_ == Qt::RightDockWidgetArea) {
    int h = splitter_->height();

    for (int i = 0; i < n; ++ i) {
      int h1 = h/(n - i);

      sizes.push_back(h1);

      h -= h1;
    }
  }
  else {
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
updateDockArea()
{
  setAllowedAreas(dockArea_);

  if (dockArea_ == Qt::LeftDockWidgetArea || dockArea_ == Qt::RightDockWidgetArea)
    setFeatures(0);
  else
    setFeatures(QDockWidget::DockWidgetVerticalTitleBar);

  title_->updateDockArea();

  if (dockArea_ == Qt::LeftDockWidgetArea || dockArea_ == Qt::RightDockWidgetArea)
    splitter_->setOrientation(Qt::Vertical);
  else
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

  if (dockArea() == Qt::LeftDockWidgetArea || dockArea() == Qt::RightDockWidgetArea)
    setDockWidth(dockWidth(), false);
  else
    setDockHeight(dockHeight(), false);

  title_->updateState();

  expanded_ = true;
}

void
CQPaletteArea::
collapseSlot()
{
  if (! expanded_) return;

  int w = 1;

  for (Windows::iterator p = windows_.begin(); p != windows_.end(); ++p) {
    CQPaletteWindow *window = *p;

    if (dockArea() == Qt::LeftDockWidgetArea || dockArea() == Qt::RightDockWidgetArea)
      w = std::max(w, window->dockWidth());
    else
      w = std::max(w, window->dockHeight());
  }

  if (dockArea() == Qt::LeftDockWidgetArea || dockArea() == Qt::RightDockWidgetArea)
    setDockWidth(w, true);
  else
    setDockHeight(w, true);

  title_->updateState();

  expanded_ = false;
}

void
CQPaletteArea::
pinSlot()
{
  if (pinned_) return;

  pinned_ = true;
}

void
CQPaletteArea::
unpinSlot()
{
  if (! pinned_) return;

  pinned_ = false;
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

void
CQPaletteArea::
setFloating(bool floating, const QPoint &pos)
{
  if (floating == floating_)
    return;

  if (floating) {
    QPoint lpos = mapFromGlobal(pos);

    setParent(0, Qt::FramelessWindowHint);

    move(pos - lpos);

    allowedAreas_ = Qt::AllDockWidgetAreas;

    Pages pages = getPages();

    for (Pages::const_iterator p = pages.begin(); p != pages.end(); ++p) {
      CQPaletteAreaPage *page = *p;

      allowedAreas_ &= page->allowedAreas();
    }
  }
  else
    mgr_->window()->addDockWidget(dockArea(), this);

  floating_ = floating;

  show();
}

void
CQPaletteArea::
cancelFloating()
{
  mgr_->window()->addDockWidget(dockArea(), this);

  floating_ = false;
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
execDrop(const QPoint &gpos)
{
  CQPaletteArea *area = mgr_->getAreaAt(gpos, allowedAreas_);

  if (area && area != this) {
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

    if (dockArea() == Qt::LeftDockWidgetArea || dockArea() == Qt::RightDockWidgetArea) {
      int y = widget->mapFromGlobal(gpos).y();

      if      (y >    - tol && y <      tol) { rect = QRect(0,   - tol, w,     2*tol); break;; }
      else if (y >  h - tol && y <  h + tol) { rect = QRect(0, h - tol, w,     2*tol); break; }
      else if (y >=     tol && y <= h - tol) { rect = QRect(0,     tol, w, h - 2*tol); break; }
    }
    else {
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

  if (! floating_ && ! windows_.empty()) {
    rect = geometry();

    rect.adjust(dx, dy, dx, dy);

    if (dockArea() == Qt::LeftDockWidgetArea || dockArea() == Qt::RightDockWidgetArea) {
      rect.setHeight(crect.height());

      if (rect.width() < 2*tol) rect.setWidth(2*tol);
    }
    else {
      rect.setWidth(crect.height());

      if (rect.height() < 2*tol) rect.setHeight(2*tol);
    }
  }
  else {
    // left/right are the height of the central widget and to the side of it
    if (dockArea() == Qt::LeftDockWidgetArea || dockArea() == Qt::RightDockWidgetArea) {
      rect = crect;

      if (dockArea() == Qt::LeftDockWidgetArea)
        rect.setWidth(2*tol);
      else
        rect.adjust(rect.width() - 20, 0, 0, 0);

      rect.adjust(dx, dy, dx, dy);
    }
    // top/bottom are the width of the main window and to the side of the central widget
    else {
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
updateSize()
{
  if (windows_.empty())
    hide();
  else {
    QSize s = sizeHint();

    if (dockArea() == Qt::LeftDockWidgetArea || dockArea() == Qt::RightDockWidgetArea)
      setDockWidth (s.width ());
    else
      setDockHeight(s.height());
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

    if (dockArea() == Qt::LeftDockWidgetArea || dockArea() == Qt::RightDockWidgetArea) {
      w  = std::max(w, s.width());
      h += s.height();
    }
    else {
      h  = std::max(h, s.height());
      w += s.width();
    }
  }

  return QSize(w, h);
}

//------

CQPaletteWindow::
CQPaletteWindow(CQPaletteArea *area) :
 mgr_(area->mgr()), area_(area), parent_(0), floating_(false)
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

  if (area_->dockArea() == Qt::LeftDockWidgetArea ||
      area_->dockArea() == Qt::RightDockWidgetArea) {
    l->addWidget(title_, 0, 0);
    l->addWidget(group_, 1, 0);
  }
  else {
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
  CQPaletteAreaPage *page = group_->currentPage();

  return (page ? page->title() : "");
}

QIcon
CQPaletteWindow::
getIcon() const
{
  CQPaletteAreaPage *page = group_->currentPage();

  return (page ? page->icon() : QIcon());
}

void
CQPaletteWindow::
pageChangedSlot(CQPaletteAreaPage *)
{
  title_->update();
}

CQPaletteWindow::Pages
CQPaletteWindow::
getPages() const
{
  CQPaletteGroup::PageArray pages;

  group()->getPages(pages);

  return pages;
}

void
CQPaletteWindow::
setFloating(bool floating, const QPoint &pos)
{
  if (floating == floating_)
    return;

  if (floating) {
    CQPaletteGroup::PageArray pages = getPages();

    CQPaletteAreaPage *currentPage = group_->currentPage();

    allowedAreas_ = currentPage->allowedAreas();

    if (pages.size() > 1) {
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

    setParent(0, Qt::FramelessWindowHint);

    move(pos - lpos);
  }
  else
    setParent(parent_);

  floating_ = floating;

  show();
}

void
CQPaletteWindow::
cancelFloating()
{
  if (! newWindow_)
    area_->splitter()->insertWidget(parentPos_, this);
  else {
    CQPaletteAreaPage *currentPage = group_->currentPage();

    removePage(currentPage);

    newWindow_->insertPage(parentPos_, currentPage);

    newWindow_->setCurrentPage(currentPage);

    mgr_->removeWindow(this);

    this->deleteLater();
  }
}

void
CQPaletteWindow::
animateDrop(const QPoint &p)
{
  CQPaletteArea *area = mgr_->getAreaAt(p, allowedAreas_);

  if (area)
    mgr_->highlightArea(area, p);
  else
    clearDrop();
}

void
CQPaletteWindow::
execDrop(const QPoint &gpos)
{
  CQPaletteArea *area = mgr_->getAreaAt(gpos, allowedAreas_);

  if (area) {
    area_->removeWindow(this);

    area->addWindowAtPos(this, gpos);

    mgr_->window()->addDockWidget(area->dockArea(), area);
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
closeSlot()
{
  CQPaletteAreaPage *page = group_->currentPage();

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

  if (area_->dockArea() == Qt::LeftDockWidgetArea ||
      area_->dockArea() == Qt::RightDockWidgetArea) {
    w = std::max(std::max(w, ts.width()), gs.width());
    h = gs.height() + ts.height();
  }
  else {
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
  if (area_->dockArea() == Qt::LeftDockWidgetArea ||
      area_->dockArea() == Qt::RightDockWidgetArea)
    setOrientation(Qt::Horizontal);
  else
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
updateState()
{
  if (area_->isExpanded()) {
    pinButton_->setVisible(true);

    if (area_->isPinned())
      pinButton_->setIcon(QPixmap(unpin_data));
    else
      pinButton_->setIcon(QPixmap(pin_data));
  }
  else
    pinButton_->setVisible(false);

  if (area_->isExpanded())
    expandButton_->setIcon(style()->standardIcon(QStyle::SP_TitleBarUnshadeButton, 0, this));
  else
    expandButton_->setIcon(style()->standardIcon(QStyle::SP_TitleBarShadeButton, 0, this));

  updateLayout();
}

void
CQPaletteAreaTitle::
contextMenuEvent(QContextMenuEvent *e)
{
  if (! contextMenu_) {
    contextMenu_ = new QMenu(this);

    QAction *pinAction    = contextMenu_->addAction("Pin");
    QAction *expandAction = contextMenu_->addAction("Expand");

    connect(pinAction   , SIGNAL(triggered()), this, SLOT(pinSlot()));
    connect(expandAction, SIGNAL(triggered()), this, SLOT(expandSlot()));
  }

  QList<QAction *> actions = contextMenu_->actions();

  for (int i = 0; i < actions.size(); ++i) {
    QAction *action = actions.at(i);

    const QString &text = action->text();

    if      (text == "Pin" || text == "Unpin") {
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
    area_->setFloating(true, e->globalPos());

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

  area_->setFloating(false);

  area_->execDrop(e->globalPos());

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
  if (window_->area()->dockArea() == Qt::LeftDockWidgetArea ||
      window_->area()->dockArea() == Qt::RightDockWidgetArea)
    setOrientation(Qt::Horizontal);
  else
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

    QAction *closeAction = contextMenu_->addAction("Close");

    connect(closeAction, SIGNAL(triggered()), window_, SLOT(closeSlot()));
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
    window_->setFloating(true, e->globalPos());

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

  window_->setFloating(false);

  window_->execDrop(e->globalPos());

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
