
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeWidget>

#include "h95/database/Search.hpp"

QT_BEGIN_NAMESPACE

namespace Ui
{
	class MainWindow;
}

QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
	Q_DISABLE_COPY_MOVE( MainWindow )
	Q_OBJECT

	Search record_search {};

  public:

	MainWindow( QWidget *parent = nullptr );
	~MainWindow();

  private:

	Ui::MainWindow *ui;

	void addTreeRoot( QString name, QString record_id );
	void addTreeChild( QTreeWidgetItem *parent, QString name, QString record_id );
	void openBatchImportDialog();

  signals:
	void triggerSearch();

  private slots:
	void on_actionImport_triggered();
	void on_actionOptions_triggered();
	void switchToDetailed(const Record record);
	//void on_homeButton_pressed();
};

#endif // MAINWINDOW_H
