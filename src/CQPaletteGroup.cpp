#include <CQPaletteGroup.h>
#include <CQPaletteArea.h>
#include <CQWidgetUtil.h>
#include <QVariant>
#include <cassert>

CQPaletteGroupMgr *
CQPaletteGroupMgr::
getInstance()
{
  static CQPaletteGroupMgr *mgr;

  if (! mgr)
    mgr = new CQPaletteGroupMgr;

  return mgr;
}

CQPaletteGroupMgr::
CQPaletteGroupMgr()
{
}

CQPaletteGroup *
CQPaletteGroupMgr::
createGroup(CQPaletteWindow *window)
{
  CQPaletteGroup *group = new CQPaletteGroup(window);

  groups_.push_back(group);

  return group;
}

void
CQPaletteGroupMgr::
removeGroup(CQPaletteGroup *group)
{
  uint i = 0;

  for ( ; i < groups_.size(); ++i)
    if (groups_[i] == group)
      break;

  assert(i < groups_.size());

  for ( ; i < groups_.size(); ++i)
    groups_[i] = groups_[i + 1];

  groups_.pop_back();
}

CQPaletteGroup *
CQPaletteGroupMgr::
getGroup(const QString &name) const
{
  for (uint i = 0; i < groups_.size(); ++i)
    if (groups_[i]->objectName() == name)
      return groups_[i];

  return nullptr;
}

CQPaletteGroup *
CQPaletteGroupMgr::
getGroupFromTabBar(const QString &name) const
{
  for (uint i = 0; i < groups_.size(); ++i)
    if (groups_[i]->tabbar()->objectName() == name)
      return groups_[i];

  return nullptr;
}

//-------

CQPaletteGroup::
CQPaletteGroup(CQPaletteWindow *window) :
 window_(window)
{
  setObjectName("group");

  setFocusPolicy(Qt::NoFocus);

  tabbar_ = new CQPaletteGroupTabBar(this);
  stack_  = new CQPaletteGroupStack (this);

  connect(tabbar_, SIGNAL(currentChanged(int)), this, SLOT(setTabIndex(int)));
  connect(tabbar_, SIGNAL(currentPressed(int)), this, SLOT(pressTabIndex(int)));

  connect(tabbar_, SIGNAL(tabMovePageSignal(const QString &, int, const QString &, int)),
          this, SLOT(tabMovePageSlot(const QString &, int, const QString &, int)));

  updateLayout();
}

CQPaletteGroup::
~CQPaletteGroup()
{
  CQPaletteGroupMgrInst->removeGroup(this);
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

  page->setDockArea(dockArea());

  pages_[page->id()] = page;

  page->setHidden(false);

  tabbar_->addPage(page);
  stack_ ->addPage(page);
}

void
CQPaletteGroup::
insertPage(int ind, CQPaletteAreaPage *page)
{
  page->setGroup(this);

  page->setDockArea(dockArea());

  pages_[page->id()] = page;

  tabbar_->insertPage(ind, page);
  stack_ ->addPage   (page);
}

void
CQPaletteGroup::
removePage(CQPaletteAreaPage *page)
{
  page->setGroup(nullptr);

  page->setHidden(true);

  tabbar_->removePage(page);
  stack_ ->removePage(page);

  pages_.erase(page->id());

  if (! pages_.empty())
    setCurrentPage(pages_.begin()->second);
}

void
CQPaletteGroup::
showPage(CQPaletteAreaPage *page)
{
  if (! page->hidden())
    return;

  page->setHidden(false);

  tabbar_->addPage(page);
  stack_ ->addPage(page);

  if (! currentPage())
    setCurrentPage(page);
}

void
CQPaletteGroup::
hidePage(CQPaletteAreaPage *page)
{
  if (page->hidden())
    return;

  bool current = (page == currentPage());

  page->setHidden(true);

  tabbar_->removePage(page);
  stack_ ->removePage(page);

  if (current)
    updateCurrentPage();
}

CQPaletteAreaPage *
CQPaletteGroup::
currentPage() const
{
  int ind = tabbar_->currentIndex();

  if (ind < 0)
    return nullptr;

  return getPageForIndex(ind);
}

int
CQPaletteGroup::
currentIndex() const
{
  return tabbar_->currentIndex();
}

void
CQPaletteGroup::
setCurrentPage(CQPaletteAreaPage *page)
{
  for (int i = 0; i < tabbar_->count(); ++i) {
    int ind = tabbar_->tabInd(i);

    uint id = tabbar_->getPageId(ind);

    if (id == page->id()) {
      tabbar_->setCurrentIndex(ind);

      return;
    }
  }
}

void
CQPaletteGroup::
updateCurrentPage()
{
  for (Pages::const_iterator p = pages_.begin(); p != pages_.end(); ++p) {
    CQPaletteAreaPage *page = (*p).second;

    if (! page->hidden()) {
      setCurrentPage(page);
      return;
    }
  }
}

CQPaletteAreaPage *
CQPaletteGroup::
getPage(int i) const
{
  PageArray pages;

  getPages(pages);

  return pages[uint(i)];
}

uint
CQPaletteGroup::
numPages() const
{
  uint num = 0;

  for (Pages::const_iterator p = pages_.begin(); p != pages_.end(); ++p) {
    CQPaletteAreaPage *page = (*p).second;

    if (! page->hidden())
      ++num;
  }

  return num;
}

void
CQPaletteGroup::
getPages(PageArray &pages) const
{
  for (Pages::const_iterator p = pages_.begin(); p != pages_.end(); ++p) {
    CQPaletteAreaPage *page = (*p).second;

    if (! page->hidden())
      pages.push_back(page);
  }
}

void
CQPaletteGroup::
setTabIndex(int ind)
{
  CQPaletteAreaPage *page = getPageForIndex(ind);
  if (! page) return;

  stack_->setPage(page);

  emit currentPageChanged(page);

  if (! window()->area()->isExpanded())
    window()->area()->expandSlot();
}

void
CQPaletteGroup::
pressTabIndex(int /*ind*/)
{
  window()->toggleExpandSlot();
}

void
CQPaletteGroup::
tabMovePageSlot(const QString &fromName, int fromIndex, const QString &toName, int /*toIndex*/)
{
  CQPaletteGroup *group1 = CQPaletteGroupMgrInst->getGroupFromTabBar(fromName);
  CQPaletteGroup *group2 = CQPaletteGroupMgrInst->getGroupFromTabBar(toName);

  assert(group2 == this);

  CQPaletteAreaPage *page1 = group1->getPageForIndex(fromIndex);
//CQPaletteAreaPage *page2 = group2->getPageForIndex(toIndex);

  if (page1->allowedAreas() & group2->window()->dockArea())
    group1->window()->movePage(page1, group2->window());
}

CQPaletteAreaPage *
CQPaletteGroup::
getPageForIndex(int ind) const
{
  uint id = tabbar_->getPageId(ind);

  Pages::const_iterator p = pages_.find(id);

  if (p == pages_.end())
    return nullptr;

  return (*p).second;
}

void
CQPaletteGroup::
updateLayout()
{
  if (! isVisible()) return;

  int w = width ();
  int h = height();

  Qt::DockWidgetArea dockArea = this->dockArea();

  int tw, th;

  if (dockArea == Qt::LeftDockWidgetArea || dockArea == Qt::RightDockWidgetArea) {
    tw = tabbar()->minimumSizeHint().width();
    th = std::max(tabbar()->height(), tabbar()->minimumSizeHint().height());
  }
  else {
    tw = std::max(tabbar()->width (), tabbar()->minimumSizeHint().width ());
    th = tabbar()->minimumSizeHint().height();
  }

  tabbar_->resize(tw, th);

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
 CQTabBar(group), group_(group)
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

#if 0
  if      (dockArea == Qt::LeftDockWidgetArea)
    setShape(QTabBar::RoundedWest);
  else if (dockArea == Qt::RightDockWidgetArea)
    setShape(QTabBar::RoundedEast);
  else if (dockArea == Qt::TopDockWidgetArea)
    setShape(QTabBar::RoundedNorth);
  else if (dockArea == Qt::BottomDockWidgetArea)
    setShape(QTabBar::RoundedSouth);
#else
  if      (dockArea == Qt::LeftDockWidgetArea)
    setPosition(CQTabBar::Position::West);
  else if (dockArea == Qt::RightDockWidgetArea)
    setPosition(CQTabBar::Position::East);
  else if (dockArea == Qt::TopDockWidgetArea)
    setPosition(CQTabBar::Position::North);
  else if (dockArea == Qt::BottomDockWidgetArea)
    setPosition(CQTabBar::Position::South);
#endif
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
insertPage(int ind, CQPaletteAreaPage *page)
{
  int ind1 = insertTab(ind, page->icon(), page->title());

  setTabData(ind1, page->id());
}

void
CQPaletteGroupTabBar::
removePage(CQPaletteAreaPage *page)
{
  int i = 0, ind = -1;

  for ( ; i < count(); ++i) {
    ind = tabInd(i);

    if (getPageId(ind) == page->id())
      break;
  }

  assert(i < count());

  removeTab(ind);
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

CQPaletteGroupStack::
~CQPaletteGroupStack()
{
  while (count())
    removeWidget(widget(0));
}

void
CQPaletteGroupStack::
addPage(CQPaletteAreaPage *page)
{
  QWidget *pw = parentWidget();
  QWidget *cw = page->widget()->parentWidget();

  assert(pw != cw);

  addWidget(page->widget());
}

void
CQPaletteGroupStack::
removePage(CQPaletteAreaPage *page)
{
  removeWidget(page->widget());

  page->widget()->setParent(nullptr);
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
 w_(w), dockArea_(Qt::NoDockWidgetArea), hidden_(false), fixedWidth_(100), fixedHeight_(100),
 widthResizable_(true), heightResizable_(true)
{
  setObjectName("page");

  id_ = ++lastId_;
}

void
CQPaletteAreaPage::
setWidget(QWidget *w)
{
  w_ = w;
}

// get page min/max width
void
CQPaletteAreaPage::
getMinMaxWidth(int &min_w, int &max_w) const
{
  if (widthResizable()) {
    QWidget *w = const_cast<CQPaletteAreaPage *>(this)->widget();

    QSize s = CQWidgetUtil::SmartMinSize(w);

    min_w = s.width();
    max_w = w->maximumWidth();
  }
  else {
    min_w = fixedWidth_;
    max_w = min_w;
  }
}

// get page min/max height
void
CQPaletteAreaPage::
getMinMaxHeight(int &min_h, int &max_h) const
{
  if (heightResizable()) {
    QWidget *w = const_cast<CQPaletteAreaPage *>(this)->widget();

    QSize s = CQWidgetUtil::SmartMinSize(w);

    min_h = s.height();
    max_h = w->maximumHeight();
  }
  else {
    min_h = fixedHeight_;
    max_h = min_h;
  }
}
