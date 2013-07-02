#include <CQPaletteGroup.h>
#include <CQPaletteArea.h>
#include <QVariant>
#include <cassert>

CQPaletteGroup::
CQPaletteGroup(CQPaletteWindow *window) :
 window_(window)
{
  setObjectName("group");

  setFocusPolicy(Qt::NoFocus);

  tabbar_ = new CQPaletteGroupTabBar(this);
  stack_  = new CQPaletteGroupStack (this);

  connect(tabbar_, SIGNAL(currentChanged(int)), this, SLOT(setTabIndex(int)));

  updateLayout();
}

Qt::DockWidgetArea
CQPaletteGroup::
dockArea() const
{
  CQPaletteArea *area = window()->area();

  if (area)
    return area->dockArea();
  else
    return Qt::LeftDockWidgetArea;
}

void
CQPaletteGroup::
updateDockArea()
{
  tabbar_->updateDockArea();

  updateLayout();
}

void
CQPaletteGroup::
addPage(CQPaletteAreaPage *page)
{
  page->setGroup(this);

  tabbar_->addPage(page);
  stack_ ->addPage(page);

  pages_[page->id()] = page;
}

void
CQPaletteGroup::
removePage(CQPaletteAreaPage *page)
{
  page->setGroup(0);

  tabbar_->removePage(page);
  stack_ ->removePage(page);

  pages_.erase(page->id());
}

CQPaletteAreaPage *
CQPaletteGroup::
currentPage() const
{
  uint id = tabbar_->getPageId(tabbar_->currentIndex());

  Pages::const_iterator p = pages_.find(id);

  if (p == pages_.end())
    return 0;

  return (*p).second;
}

CQPaletteAreaPage *
CQPaletteGroup::
getPage(int i) const
{
  PageArray pages;

  getPages(pages);

  return pages[i];
}

void
CQPaletteGroup::
getPages(PageArray &pages) const
{
  for (Pages::const_iterator p = pages_.begin(); p != pages_.end(); ++p)
    pages.push_back((*p).second);
}

void
CQPaletteGroup::
setTabIndex(int ind)
{
  uint id = tabbar_->getPageId(ind);

  Pages::const_iterator p = pages_.find(id);
  if (p == pages_.end()) return;

  CQPaletteAreaPage *page = (*p).second;

  stack_->setPage(page);

  emit currentPageChanged(page);
}

void
CQPaletteGroup::
updateLayout()
{
  if (! isVisible()) return;

  int w = width ();
  int h = height();

  int tw = std::max(tabbar()->width (), tabbar()->minimumSizeHint().width ());
  int th = std::max(tabbar()->height(), tabbar()->minimumSizeHint().height());

  tabbar_->resize(tw, th);

  Qt::DockWidgetArea dockArea = this->dockArea();

  if      (dockArea == Qt::LeftDockWidgetArea) {
    tabbar()->move(0     , 0     ); tabbar()->resize(tw    , h     );
    stack ()->move(tw    , 0     ); stack ()->resize(w - tw, h     );
  }
  else if (dockArea == Qt::RightDockWidgetArea) {
    tabbar()->move(w - tw, 0     ); tabbar()->resize(tw    , h     );
    stack ()->move(0     , 0     ); stack ()->resize(w - tw, h     );
  }
  else if (dockArea == Qt::TopDockWidgetArea) {
    tabbar()->move(0     , 0     ); tabbar()->resize(w     , th    );
    stack ()->move(0     , th    ); stack ()->resize(w     , h - th);
  }
  else if (dockArea == Qt::BottomDockWidgetArea) {
    tabbar()->move(0     , h - th); tabbar()->resize(w     , th    );
    stack ()->move(0     , 0     ); stack ()->resize(w     , h - th);
  }
}

void
CQPaletteGroup::
showEvent(QShowEvent *)
{
  updateLayout();
}

void
CQPaletteGroup::
resizeEvent(QResizeEvent *)
{
  updateLayout();
}

QSize
CQPaletteGroup::
sizeHint() const
{
  int w = 0;
  int h = 0;

  Qt::DockWidgetArea dockArea = this->dockArea();

  if (dockArea == Qt::LeftDockWidgetArea || dockArea == Qt::RightDockWidgetArea) {
    w = tabbar_->width() + stack_->width();
    h = std::max(tabbar_->height(), stack_->height());
  }
  else {
    h = tabbar_->height() + stack_->height();
    w = std::max(tabbar_->width(), stack_->width());
  }

  return QSize(w, h);
}

//------

CQPaletteGroupTabBar::
CQPaletteGroupTabBar(CQPaletteGroup *group) :
 QTabBar(group), group_(group)
{
  setObjectName("tabbar");

  setFocusPolicy(Qt::NoFocus);

  updateDockArea();

  //setButtonStyle(Qt::ToolButtonIconOnly);
}

void
CQPaletteGroupTabBar::
updateDockArea()
{
  Qt::DockWidgetArea dockArea = group_->dockArea();

  if      (dockArea == Qt::LeftDockWidgetArea)
    setShape(QTabBar::RoundedWest);
  else if (dockArea == Qt::RightDockWidgetArea)
    setShape(QTabBar::RoundedEast);
  else if (dockArea == Qt::TopDockWidgetArea)
    setShape(QTabBar::RoundedNorth);
  else if (dockArea == Qt::BottomDockWidgetArea)
    setShape(QTabBar::RoundedSouth);
}

void
CQPaletteGroupTabBar::
addPage(CQPaletteAreaPage *page)
{
  int ind = addTab(page->icon(), page->title());

  setTabData(ind, page->id());
}

void
CQPaletteGroupTabBar::
removePage(CQPaletteAreaPage *page)
{
  int i = 0;

  for ( ; i < count(); ++i)
    if (getPageId(i) == page->id())
      break;

  assert(i < count());

  removeTab(i);
}

uint
CQPaletteGroupTabBar::
getPageId(int ind) const
{
  return tabData(ind).toUInt();
}

QSize
CQPaletteGroupTabBar::
sizeHint() const
{
  enum { TAB_BORDER=8, RESIZE_WIDTH=5 };

  QFontMetrics fm(font());

  int iw = iconSize().width();

  int w = iw;
  int h = qMax(iw, fm.height()) + TAB_BORDER + RESIZE_WIDTH;

  Qt::DockWidgetArea dockArea = group_->dockArea();

  if (dockArea == Qt::LeftDockWidgetArea || dockArea == Qt::RightDockWidgetArea)
    return QSize(h, w);
  else
    return QSize(w, h);
}

QSize
CQPaletteGroupTabBar::
minimumSizeHint() const
{
  QSize s = sizeHint();

  Qt::DockWidgetArea dockArea = group_->dockArea();

  if (dockArea == Qt::LeftDockWidgetArea || dockArea == Qt::RightDockWidgetArea)
    return QSize(s.width(), 0);
  else
    return QSize(0, s.height());
}

//------

CQPaletteGroupStack::
CQPaletteGroupStack(QWidget *parent) :
 QStackedWidget(parent)
{
  setObjectName("stack");
}

void
CQPaletteGroupStack::
addPage(CQPaletteAreaPage *page)
{
  addWidget(page->widget());
}

void
CQPaletteGroupStack::
removePage(CQPaletteAreaPage *page)
{
  removeWidget(page->widget());
}

void
CQPaletteGroupStack::
setPage(CQPaletteAreaPage *page)
{
  setCurrentWidget(page->widget());
}

//------

uint CQPaletteAreaPage::lastId_ = 0;

CQPaletteAreaPage::
CQPaletteAreaPage(QWidget *w) :
 w_(w)
{
  setObjectName("page");

  id_ = ++lastId_;
}
