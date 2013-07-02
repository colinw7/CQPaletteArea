#ifndef CQPaletteGroup_H
#define CQPaletteGroup_H

#include <QTabBar>
#include <QStackedWidget>
#include <QIcon>
#include <map>

class CQPaletteWindow;
class CQPaletteGroupTabBar;
class CQPaletteGroupStack;
class CQPaletteAreaPage;

// class to hold a tabbed set of widgets displayed in a palette sub window
class CQPaletteGroup : public QWidget {
  Q_OBJECT

 public:
  typedef std::vector<CQPaletteAreaPage *> PageArray;

 public:
  CQPaletteGroup(CQPaletteWindow *window);

  CQPaletteWindow *window() const { return window_; }

  void setWindow(CQPaletteWindow *window);

  CQPaletteGroupTabBar *tabbar() const { return tabbar_; }

  CQPaletteGroupStack *stack() const { return stack_; }

  Qt::DockWidgetArea dockArea() const;

  void updateDockArea();

  uint numPages() const { return pages_.size(); }

  void addPage(CQPaletteAreaPage *page);

  void removePage(CQPaletteAreaPage *page);

  CQPaletteAreaPage *currentPage() const;

  CQPaletteAreaPage *getPage(int i) const;

  void getPages(PageArray &pages) const;

  QSize sizeHint() const;

 signals:
  void currentPageChanged(CQPaletteAreaPage *page);

 private slots:
  void setTabIndex(int ind);

 private:
  void updateLayout();

  void showEvent(QShowEvent *);

  void resizeEvent(QResizeEvent *);

 private:
  typedef std::map<uint,CQPaletteAreaPage*> Pages;

  CQPaletteWindow      *window_;
  CQPaletteGroupTabBar *tabbar_;
  CQPaletteGroupStack  *stack_;
  Pages                 pages_;
};

//------

// tabbar
class CQPaletteGroupTabBar : public QTabBar {
  Q_OBJECT

 public:
  CQPaletteGroupTabBar(CQPaletteGroup *group);

  void updateDockArea();

  void addPage(CQPaletteAreaPage *page);

  void removePage(CQPaletteAreaPage *page);

  uint getPageId(int ind) const;

  QSize sizeHint() const;

  QSize minimumSizeHint() const;

 private:
  CQPaletteGroup *group_;
};

//------

// stacked widget
class CQPaletteGroupStack : public QStackedWidget {
  Q_OBJECT

 public:
  CQPaletteGroupStack(QWidget *parent);

  void addPage(CQPaletteAreaPage *page);

  void removePage(CQPaletteAreaPage *page);

  void setPage(CQPaletteAreaPage *page);
};

//------

class CQPaletteAreaPage : public QObject {
  Q_OBJECT

 public:
  CQPaletteAreaPage(QWidget *w=0);

  virtual ~CQPaletteAreaPage() { }

  CQPaletteGroup *group() const { return group_; }

  void setGroup(CQPaletteGroup *group) { group_ = group; }

  QWidget *widget() const { return w_; }

  void setWidget(QWidget *w) { w_ = w; }

  uint id() const { return id_; }

  virtual QString windowTitle() const { return ""; }

  virtual QString title() const { return ""; }
  virtual QIcon   icon () const { return QIcon(); }

 private:
  static uint lastId_;

  CQPaletteGroup *group_; // parent group
  QWidget        *w_;     // child widget
  uint            id_;    // unique id
};

#endif
