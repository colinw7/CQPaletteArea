#include <CQPaletteAreaTest.h>
#include <CQPaletteArea.h>
#include <CQPaletteGroup.h>

#include <QApplication>
#include <QVBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QPainter>
#include <QMenu>
#include <QMenuBar>
#include <QTextEdit>
#include <QTabBar>
#include <QStackedWidget>
#include <QStackedLayout>

#include <iostream>

#include "images/transform.xpm"
#include "images/pen.xpm"
#include "images/brush.xpm"
#include "images/console.xpm"
#include "images/mru.xpm"

class PageWidget : public QWidget {
 public:
  PageWidget() { }
 ~PageWidget() { }
};

class PageAction {
 public:
  PageAction() : action_(0) { }

  virtual ~PageAction() { }

  virtual QString title() const = 0;

  QAction *createAction(QMenu *menu) {
    action_ = new QAction(title(), menu);

    action_->setCheckable(true);
    action_->setChecked(true);

    menu->addAction(action_);

    return action_;
  }

 protected:
  QAction *action_;
};

class TransformPage : public CQPaletteAreaPage, public PageAction {
 public:
  TransformPage();

  QString title      () const { return "Transform"; }
  QString windowTitle() const { return "Transform Page"; }
  QIcon   icon       () const { return QIcon(QPixmap((const char **) transform_data)); }

  Qt::DockWidgetAreas allowedAreas() const {
    return Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea;
  }

  void setHidden(bool hidden) {
    if (action_)
      action_->setChecked(! hidden);

    CQPaletteAreaPage::setHidden(hidden);
  }
};

class PenPage : public CQPaletteAreaPage, public PageAction {
 public:
  PenPage();

  QString title      () const { return "Pen"; }
  QString windowTitle() const { return "Pen Page"; }
  QIcon   icon       () const { return QIcon(QPixmap((const char **) pen_data)); }

  Qt::DockWidgetAreas allowedAreas() const {
    return Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea;
  }

  void setHidden(bool hidden) {
    if (hidden) action_->setChecked(false);

    CQPaletteAreaPage::setHidden(hidden);
  }
};

class BrushPage : public CQPaletteAreaPage, public PageAction {
 public:
  BrushPage();

  QString title      () const { return "Brush"; }
  QString windowTitle() const { return "Brush Page"; }
  QIcon   icon       () const { return QIcon(QPixmap((const char **) brush_data)); }

  Qt::DockWidgetAreas allowedAreas() const {
    return Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea;
  }

  void setHidden(bool hidden) {
    if (hidden) action_->setChecked(false);

    CQPaletteAreaPage::setHidden(hidden);
  }
};

class ConsolePage : public CQPaletteAreaPage, public PageAction {
 public:
  ConsolePage();

  QString title      () const { return "Console"; }
  QString windowTitle() const { return "Console Page"; }
  QIcon   icon       () const { return QIcon(QPixmap((const char **) console_data)); }

  Qt::DockWidgetAreas allowedAreas() const {
    return Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea;
  }

  void setHidden(bool hidden) {
    if (hidden) action_->setChecked(false);

    CQPaletteAreaPage::setHidden(hidden);
  }
};

class MRUPage : public CQPaletteAreaPage, public PageAction {
 public:
  MRUPage();

  QString title      () const { return "MRU"; }
  QString windowTitle() const { return "MRU Page"; }
  QIcon   icon       () const { return QIcon(QPixmap((const char **) mru_data)); }

  Qt::DockWidgetAreas allowedAreas() const {
    return Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea |
           Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea;
  }

  void setHidden(bool hidden) {
    if (hidden) action_->setChecked(false);

    CQPaletteAreaPage::setHidden(hidden);
  }
};

class Canvas : public QWidget {
 public:
  Canvas() { }

  void paintEvent(QPaintEvent *);
};

CQPaletteAreaTest::
CQPaletteAreaTest() :
 QMainWindow()
{
  setObjectName("CQPaletteAreaTest");

  Canvas *canvas = new Canvas;

  canvas->setObjectName("Canvas");

  setCentralWidget(canvas);

  //------

  mgr_ = new CQPaletteAreaMgr(this);

  transformPage_ = new TransformPage();
  penPage_       = new PenPage      ();
  brushPage_     = new BrushPage    ();
  consolePage_   = new ConsolePage  ();
  mruPage_       = new MRUPage      ();

  //connect(consolePage_, SIGNAL(pageShown ()), this, SLOT(consoleShown ()));
  //connect(consolePage_, SIGNAL(pageHidden()), this, SLOT(consoleHidden()));

  mgr_->addPage(transformPage_, Qt::LeftDockWidgetArea);
  mgr_->addPage(penPage_      , Qt::LeftDockWidgetArea);
  mgr_->addPage(brushPage_    , Qt::LeftDockWidgetArea);

  mgr_->addPage(consolePage_, Qt::BottomDockWidgetArea);
  mgr_->addPage(mruPage_    , Qt::BottomDockWidgetArea);

  //-----

  QMenu *fileMenu = menuBar()->addMenu("&File");

  QAction *quitAction = new QAction("&Quit", fileMenu);

  fileMenu->addAction(quitAction);

  connect(quitAction, SIGNAL(triggered()), this, SLOT(quitSlot()));

  QMenu *paletteMenu = menuBar()->addMenu("&Palette");

  QAction *page1Action = transformPage_->createAction(paletteMenu);
  QAction *page2Action = penPage_      ->createAction(paletteMenu);
  QAction *page3Action = brushPage_    ->createAction(paletteMenu);
  QAction *page4Action = consolePage_  ->createAction(paletteMenu);
  QAction *page5Action = mruPage_      ->createAction(paletteMenu);

  connect(page1Action, SIGNAL(triggered(bool)), this, SLOT(transformSlot(bool)));
  connect(page2Action, SIGNAL(triggered(bool)), this, SLOT(penSlot(bool)));
  connect(page3Action, SIGNAL(triggered(bool)), this, SLOT(brushSlot(bool)));
  connect(page4Action, SIGNAL(triggered(bool)), this, SLOT(consoleSlot(bool)));
  connect(page5Action, SIGNAL(triggered(bool)), this, SLOT(mruSlot(bool)));

  //connect(qApp, SIGNAL(focusChanged(QWidget*,QWidget*)),
  //        this, SLOT(focusChangedSlot(QWidget*,QWidget*)));
}

void
CQPaletteAreaTest::
quitSlot()
{
  exit(0);
}

void
CQPaletteAreaTest::
transformSlot(bool show)
{
  if (show)
    mgr_->showExpandedPage(transformPage_);
  else
    mgr_->hidePage(transformPage_);
}

void
CQPaletteAreaTest::
penSlot(bool show)
{
  if (show)
    mgr_->showExpandedPage(penPage_);
  else
    mgr_->hidePage(penPage_);
}

void
CQPaletteAreaTest::
brushSlot(bool show)
{
  if (show)
    mgr_->showExpandedPage(brushPage_);
  else
    mgr_->hidePage(brushPage_);
}

void
CQPaletteAreaTest::
consoleSlot(bool show)
{
  if (show)
    mgr_->showExpandedPage(consolePage_);
  else
    mgr_->hidePage(consolePage_);
}

void
CQPaletteAreaTest::
mruSlot(bool show)
{
  if (show)
    mgr_->showExpandedPage(mruPage_);
  else
    mgr_->hidePage(mruPage_);
}

void
CQPaletteAreaTest::
focusChangedSlot(QWidget *oldW, QWidget *newW)
{
  std::cerr << "Focus change:";

  if (oldW)
    std::cerr << " [" << widgetName(oldW).toStdString() << "]";
  else
    std::cerr << " []";

  if (newW)
    std::cerr << " [" << widgetName(newW).toStdString() << "]";
  else
    std::cerr << " []";

  std::cerr << std::endl;
}

void
CQPaletteAreaTest::
consoleShown()
{
  std::cerr << "Console shown" << std::endl;
}

void
CQPaletteAreaTest::
consoleHidden()
{
  std::cerr << "Console hidden" << std::endl;
}

QString
CQPaletteAreaTest::
widgetName(QWidget *w)
{
  QString name = w->objectName();

  if (name != "")
    return name;

  return w->metaObject()->className();
}

//------

TransformPage::
TransformPage() :
 CQPaletteAreaPage(new PageWidget)
{
  QWidget *w = widget();

  w->setObjectName("transform");

  QVBoxLayout *layout = new QVBoxLayout(w);
  layout->setMargin(2); layout->setSpacing(2);

  layout->addWidget(new QPushButton("Button 1"));
  layout->addWidget(new QPushButton("Button 2"));
  layout->addWidget(new QPushButton("Button 3"));
  layout->addStretch();
}

//------

PenPage::
PenPage() :
 CQPaletteAreaPage(new PageWidget)
{
  QWidget *w = widget();

  w->setObjectName("pen");

  QVBoxLayout *layout = new QVBoxLayout(w);
  layout->setMargin(2); layout->setSpacing(2);

  layout->addWidget(new QPushButton("Button 4"));
  layout->addWidget(new QPushButton("Button 5"));
  layout->addWidget(new QPushButton("Button 6"));
  layout->addStretch();
}

//------

BrushPage::
BrushPage() :
 CQPaletteAreaPage(new PageWidget)
{
  QWidget *w = widget();

  w->setObjectName("brush");

  QVBoxLayout *layout = new QVBoxLayout(w);
  layout->setMargin(2); layout->setSpacing(2);

  QComboBox *combo1 = new QComboBox(w);

  combo1->setFocusPolicy(Qt::NoFocus);

  combo1->addItem("Item 1");
  combo1->addItem("Item 2");
  combo1->addItem("Item 3");
  combo1->addItem("Item 4");

  layout->addWidget(combo1);
  layout->addWidget(new QPushButton("Button 7"));
  layout->addWidget(new QPushButton("Button 8"));
  layout->addWidget(new QPushButton("Button 9"));
  layout->addStretch();
}

//------

ConsolePage::
ConsolePage() :
 CQPaletteAreaPage(new PageWidget)
{
  QWidget *w = widget();

  w->setObjectName("console");

  QHBoxLayout *layout = new QHBoxLayout(w);
  layout->setMargin(2); layout->setSpacing(2);

  QTextEdit *edit = new QTextEdit;

  layout->addWidget(edit);
}

//------

MRUPage::
MRUPage() :
 CQPaletteAreaPage(new PageWidget)
{
  QWidget *w = widget();

  w->setObjectName("mru");

  QVBoxLayout *layout = new QVBoxLayout(w);
  layout->setMargin(0); layout->setSpacing(0);

  QTabBar *tab = new QTabBar(w);

  tab->addTab("Recent");
  tab->addTab("Favorites");

  QStackedWidget *stack = new QStackedWidget(w);

  qobject_cast<QStackedLayout *>(stack->layout())->setStackingMode(QStackedLayout::StackAll);

  //stack->layout()->setSizeConstraint(QLayout::SetMinAndMaxSize);

  layout->addWidget(tab);
  layout->addWidget(stack);

  QWidget *page1 = new QWidget(w);
  QWidget *page2 = new QWidget(w);

  page1->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  page2->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

  page1->setFixedHeight(32);
  page2->setFixedHeight(32);

  QHBoxLayout *pageLayout1 = new QHBoxLayout(page1);
  pageLayout1->setMargin(0); pageLayout1->setSpacing(0);

  QHBoxLayout *pageLayout2 = new QHBoxLayout(page2);
  pageLayout2->setMargin(0); pageLayout2->setSpacing(0);

  pageLayout1->addWidget(new QPushButton("One"));
  pageLayout1->addStretch();

  pageLayout2->addWidget(new QPushButton("Two"));
  pageLayout2->addStretch();

  stack->addWidget(page1);
  stack->addWidget(page2);

  //stack->setFixedHeight(32);

  //setFixedHeight(64);

  connect(tab, SIGNAL(currentChanged(int)), stack, SLOT(setCurrentIndex(int)));

  w->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

  //setPageHeight(tab->sizeHint().height() + 32);

  //setSizeType(SIZE_FIXED);
  int height = tab->height() + page1->height() + 16;

  setFixedHeight(height);
}

//------

void
Canvas::
paintEvent(QPaintEvent *)
{
  QPainter p(this);

  p.fillRect(rect(), QBrush(QColor(0,0,0)));
}

//------

int
main(int argc, char **argv)
{
  QApplication app(argc, argv);

  CQPaletteAreaTest *test = new CQPaletteAreaTest;

  test->resize(600, 600);

  test->show();

  return app.exec();
}
