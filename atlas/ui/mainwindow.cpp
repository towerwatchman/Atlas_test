#include "mainwindow.h"

#include <moc_mainwindow.cpp>

#include "./dialog/SettingsDialog.hpp"
#include "./dialog/StatsDialog.hpp"
#include "./dialog/aboutqtdialog.h"
#include "./ui_mainwindow.h"
#include "core/config.hpp"
#include "core/remote/AtlasRemote.hpp"
#include "ui/importer/batchImporter/BatchImportDialog.hpp"
#include "ui/importer/glImporter/GLImporter.hpp"
#include "ui/importer/simpleImporter/SimpleImporter.hpp"
#include "ui/importer/singleImporter/SingleImporter.hpp"
#include "ui/notifications/NotificationMessage.hpp"
#include "ui/notifications/NotificationPopup.hpp"
#include "ui/views/gamelist/GameListDelegate.hpp"

MainWindow::MainWindow( QWidget* parent ) : QMainWindow( parent ), ui( new Ui::MainWindow )
{
	ui->setupUi( this );

	//Check db first, if nothing is there add default
	//default
	connect( ui->SearchBox, &QLineEdit::textChanged, this, &MainWindow::searchTextChanged );
	connect( this, &MainWindow::triggerSearch, &record_search, &Search::searchTextChanged );
	connect( this, &MainWindow::triggerReSearch, &record_search, &Search::runQuery );
	connect( &record_search, &Search::searchCompleted, ui->recordView, &RecordView::setRecords );

	connect( ui->recordView, &RecordView::openDetailedView, this, &MainWindow::switchToDetailed );
	connect( ui->homeButton, &QToolButton::clicked, this, &MainWindow::on_homeButton_pressed );

	//if ( config::geometry::main_window::hasValue() ) restoreGeometry( config::geometry::main_window::get() );
	MainWindow::resize( QSize( config::grid_ui::windowWidth::get(), config::grid_ui::windowHeight::get() ) );

	record_search.moveToThread( &search_thread );
	search_thread.start();

	//hide search icon
	//ui->searchIconSmall->hide();
	ui->NavTop->hide();

	//Set Font
	QFont font;	
	font.setPixelSize( config::application::fontSize::get() );
	font.setFamily( config::application::font::get() == "" ? QApplication::font().defaultFamily() : config::application::font::get() );
	
	try{
		QApplication::setFont( font );
		//throw 505;
	}
	catch(int num)
	{
		spdlog::error("Unable to load default font. Verify system font available");
	}

	config::notify();

	initNotificationPopup( this );
	getNotificationPopup()->hide();
	connect( getNotificationPopup(), &NotificationPopup::popupResized, this, &MainWindow::movePopup );

	//Share the recordView's model to gameList
	ui->gamesTree->setModel( ui->recordView->model() );
	ui->gamesTree->setItemDelegate( new GameListDelegate() );
	//Share selection model
	ui->gamesTree->setSelectionModel( ui->recordView->selectionModel() );

	emit triggerSearch( "", SortOrder::Name, true );

	initNotificationPopup( this );
	getNotificationPopup()->hide();
	connect( getNotificationPopup(), &NotificationPopup::popupResized, this, &MainWindow::movePopup );
	connect( getNotificationPopup(), &NotificationPopup::triggerShow, this, &MainWindow::showMessagePopup );
	connect( getNotificationPopup(), &NotificationPopup::triggerHide, this, &MainWindow::hideMessagePopup );

	//Init remote system
	atlas::initRemoteHandler();
	getNotificationPopup()->createNotification< NotificationMessage >( QString( "Welcome to Atlas!" ), true );

	//Make sure mouse tracking is enabled for view
	ui->recordView->setMouseTracking( true );
}

MainWindow::~MainWindow()
{
	search_thread.exit();
	config::geometry::main_window::set( saveGeometry() );
	delete ui;
}

void MainWindow::on_actionSimpleImporter_triggered()
{
	QMessageBox::information( this, "Importer", "Please select the game directory" );
	if ( const auto dir = QFileDialog::getExistingDirectory(
			 this, "Open directory", QDir::homePath(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks );
	     !dir.isEmpty() )
	{
		SimpleImporter importer { this };
		importer.setRoot( dir );
		importer.exec();
		//Trigger search
		emit triggerReSearch();
	}
	else
		QMessageBox::information( this, "Error", "No directory provided." );
}

void MainWindow::on_actionSingleImporter_triggered()
{
	SingleImporter importer { this };
	importer.exec();
	emit triggerReSearch();
}

void MainWindow::on_actionBulkImporter_triggered()
{
	BatchImportDialog importer { this };
	importer.exec();
	emit triggerReSearch();
}

void MainWindow::on_actionGameListImporter_triggered()
{
	QMessageBox::information(
		this, "Game List", "Please select root (Root being the games directory) for your game list install" );

	if ( const auto dir = QFileDialog::getExistingDirectory(
			 this, "Open directory", QDir::homePath(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks );
	     !dir.isEmpty() )
	{
		GLImporter* importer { new GLImporter( this ) };
		importer->show();
		importer->setImportDir( { dir.toStdString() } );
		importer->exec();
		delete importer;
		emit triggerReSearch();
	}
	else
		QMessageBox::information( this, "Error", "No directory provided." );
}

void MainWindow::on_actionOptions_triggered()
{
	SettingsDialog settingsDialog { this };
	settingsDialog.setModal( true );
	settingsDialog.exec();

	ui->recordView->reloadConfig();

	QWidget::repaint();
}

void MainWindow::switchToDetailed( const Record record )
{
	ui->detailedRecordView->setRecord( record );
	ui->stackedWidget->setCurrentIndex( 1 );
}

void MainWindow::on_homeButton_pressed()
{
	//ui->detailedRecordView->clearRecord();
	ui->stackedWidget->setCurrentIndex( 0 );
}

void MainWindow::resizeEvent( QResizeEvent* event )
{
	spdlog::debug( "Window Resize Event" );
	QMainWindow::resizeEvent( event );

	//Store window height and width
	config::grid_ui::windowHeight::
		set( config::grid_ui::windowHeight::get() != MainWindow::height() ? MainWindow::height() :
	                                                                        config::grid_ui::windowHeight::get() );
	config::grid_ui::windowWidth::
		set( config::grid_ui::windowWidth::get() != MainWindow::width() ? MainWindow::width() :
	                                                                      config::grid_ui::windowWidth::get() );

	config::grid_ui::itemViewWidth::set( ui->recordView->viewport()->width() );
	config::grid_ui::itemViewHeight::set( ui->recordView->viewport()->height() );

	ui->recordView->reloadConfig();

	movePopup();
}

void MainWindow::showEvent( [[maybe_unused]] QShowEvent* event )
{
	//Store Banner View Dims
	config::grid_ui::itemViewWidth::set( ui->recordView->viewport()->width() );
	config::grid_ui::itemViewHeight::set( ui->recordView->viewport()->height() );
	ui->recordView->reloadConfig();
}

void MainWindow::on_actionExit_triggered()
{
	QApplication::quit();
}

void MainWindow::on_actionAboutQt_triggered()
{
	AboutQtDialog aboutQtDialog { this };
	aboutQtDialog.setModal( true );
	aboutQtDialog.exec();
}

void MainWindow::on_actionViewFileHistory_triggered()
{
	StatsDialog dialog;
	dialog.exec();
}

void MainWindow::searchTextChanged( const QString str )
{
	const auto search_type = [ & ]()
	{
		switch ( ui->sortSelection->currentIndex() )
		{
			default:
				[[fallthrough]];
			case 0:
				return SortOrder::Name;
			case 1:
				return SortOrder::Creator;
			case 2:
				return SortOrder::Engine;
			case 3:
				return SortOrder::Time;
		}
	}();

	emit triggerSearch( str, search_type, ui->sortOrderButton->text() == "ASC" );
}

void MainWindow::on_sortOrderButton_clicked()
{
	if ( ui->sortOrderButton->text() == "ASC" )
		ui->sortOrderButton->setText( "DESC" );
	else
		ui->sortOrderButton->setText( "ASC" );

	searchTextChanged( ui->SearchBox->text() );
}

void MainWindow::on_sortSelection_currentIndexChanged( [[maybe_unused]] int index )
{
	searchTextChanged( ui->SearchBox->text() );
}

void MainWindow::showMessagePopup()
{
	auto& task_popup { *getNotificationPopup() };

	task_popup.show();
	movePopup();
}

void MainWindow::hideMessagePopup()
{
	auto& task_popup { *getNotificationPopup() };
	task_popup.hide();
}

void MainWindow::on_btnShowMessageLog_clicked()
{
	auto& task_popup { *getNotificationPopup() };
	if ( task_popup.isVisible() )
		hideMessagePopup();
	else
		showMessagePopup();
}

void MainWindow::moveEvent( QMoveEvent* event )
{
	QMainWindow::moveEvent( event );
	movePopup();
}

void MainWindow::movePopup()
{
	auto& task_popup { *getNotificationPopup() };
	const auto [ x, y ] = task_popup.size();

	const auto point {
		ui->recordView->mapToGlobal( ui->recordView->rect().bottomRight() - QPoint { x, y } - QPoint( 5, 5 ) )
	};
	task_popup.move( point );
}

void MainWindow::taskPopupResized()
{
	movePopup();
}
