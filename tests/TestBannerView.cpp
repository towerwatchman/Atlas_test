//
// Created by kj16609 on 3/4/23.
//

#include <ui/widgets/BannerView.hpp>
#include <QMimeData>
#include <QDropEvent>
#include <QApplication>
#include <h95/database/Database.hpp>

#include "GTestBox.hpp"

class TestBannerView : public ::testing::Test
{
	int argc { 0 };
	char** argv { nullptr };

	QApplication* app { nullptr };

	protected:
	void SetUp() override
	{
		app = new QApplication( argc, argv );
		Database::initalize( "./data/testing.db" );
	}

	void TearDown() override
	{
		delete app;
		Database::deinit();
		std::filesystem::remove( "./data/testing.db" );
	}

	inline static void dropEvent(BannerView& banner, QDropEvent& event)
	{
		banner.dropEvent(&event);
	}
};


TEST_F( TestBannerView, dragNDrop )
{
	BannerView view;

	view.setEditable( true );

	QMimeData data {};
	data.setUrls( { QUrl::fromLocalFile(
		QString::fromStdString( ( std::filesystem::canonical( "./assets/banner/placeholder.jpg" ).string() ) ) ) } );

	QDropEvent event { view.rect().center(), Qt::LinkAction, &data, Qt::LeftButton, Qt::KeyboardModifier::NoModifier };

	TestBannerView::dropEvent(view, event);

	GTEST_ASSERT_EQ( view.path(), std::filesystem::canonical( "./assets/banner/placeholder.jpg" ) );
}
