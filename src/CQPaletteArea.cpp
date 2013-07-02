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
#include <cassert>
#include <iostream>

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
getAreaAt(const QPoint &pos) const
{
  for (Palettes::const_iterator p = palettes_.begin(); p != palettes_.end(); ++p) {
    CQPaletteArea *area = (*p).second;

    QRect rect = area->getHighlightRect();

    if (rect.contains(pos))
      return area;
  }

  return 0;
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
 CQDockArea(mgr->window()), mgr_(mgr), dockArea_(dockArea)
{
  setObjectName("area");

  // remove title bar and lock in place
  setTitleBarWidget(new QWidget(this));

  setAllowedAreas(dockArea);

  setFeatures(0);

  // add to main window
  mgr_->window()->addDockWidget(dockArea, this);

  // add splitter
  splitter_ = new QSplitter;

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

QRect
CQPaletteArea::
getHighlightRectAtPos(const QPoint &gpos) const
{
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

  if (! windows_.empty()) {
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

  title_->updateDockArea();
  group_->updateDockArea();
}

void
CQPaletteWindow::
addPage(CQPaletteAreaPage *page)
{
  group_->addPage(page);
}

void
CQPaletteWindow::
removePage(CQPaletteAreaPage *page)
{
  group_->removePage(page);
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

void
CQPaletteWindow::
setFloating(bool floating, const QPoint &pos)
{
  if (floating == floating_)
    return;

  if (floating) {
    CQPaletteGroup::PageArray pages;

    group()->getPages(pages);

    if (pages.size() > 1) {
      CQPaletteWindow *newWindow = mgr_->addWindow(area_->dockArea());

      CQPaletteAreaPage *currentPage = group_->currentPage();

      for (uint i = 0; i < pages.size(); ++i) {
        if (pages[i] == currentPage) continue;

        group_->removePage(pages[i]);

        newWindow->addPage(pages[i]);
      }
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
animateDrop(const QPoint &p)
{
  CQPaletteArea *area = mgr_->getAreaAt(p);

  if (area)
    mgr_->highlightArea(area, p);
  else
    clearDrop();
}

void
CQPaletteWindow::
execDrop(const QPoint &p)
{
  CQPaletteArea *area = mgr_->getAreaAt(p);

  if (area) {
    area_->removeWindow(this);

    area->addWindowAtPos(this, p);
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

CQPaletteWindowTitle::
CQPaletteWindowTitle(CQPaletteWindow *window) :
 window_(window)
{
  closeButton_ = addButton(style()->standardIcon(QStyle::SP_TitleBarCloseButton, 0, this));

  connect(closeButton_, SIGNAL(clicked()), window, SLOT(closeSlot()));

  setAttribute(Qt::WA_Hover);
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
mousePressEvent(QMouseEvent *e)
{
  mouseState_.pressed  = true;
  mouseState_.moving   = false;
  mouseState_.pressPos = e->globalPos();
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
  if (! mouseState_.moving)
    return;

  window_->setFloating(false);

  window_->execDrop(e->globalPos());

  mouseState_.reset();
}

void
CQPaletteWindowTitle::
keyPressEvent(QKeyEvent *e)
{
  if (e->key() == Qt::Key_Escape)
    mouseState_.escapePress = true;
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
