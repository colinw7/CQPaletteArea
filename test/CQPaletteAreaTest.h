#include <QMainWindow>

class CQPaletteAreaMgr;
class CQPaletteArea;

class TransformPage;
class PenPage;
class BrushPage;
class ConsolePage;
class MRUPage;

class CQPaletteAreaTest : public QMainWindow {
  Q_OBJECT

 public:
  CQPaletteAreaTest();

 public slots:
  void quitSlot();
  void transformSlot(bool);
  void penSlot(bool);
  void brushSlot(bool);
  void consoleSlot(bool);
  void mruSlot(bool);

 private slots:
  void focusChangedSlot(QWidget *oldW, QWidget *newW);

  void consoleShown();
  void consoleHidden();

 private:
  QString widgetName(QWidget *w);

 private:
  CQPaletteAreaMgr *mgr_;
  TransformPage    *transformPage_;
  PenPage          *penPage_;
  BrushPage        *brushPage_;
  ConsolePage      *consolePage_;
  MRUPage          *mruPage_;
};
