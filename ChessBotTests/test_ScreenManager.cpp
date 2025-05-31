#include "pch.h"
#include "ScreenManager.h"

class ScreenManagerTest : public ::testing::Test
{
protected:
    void SetUp() override {}

    void TearDown() override {}
};

TEST_F(ScreenManagerTest, ScreensAreAvailable)
{
	QVector<QScreen*> screens = ScreenManager::instance().screens();
	ASSERT_FALSE(screens.isEmpty()) << "No screens available";
}

TEST_F(ScreenManagerTest, ScreenOneIsNotNull)
{
	QScreen* screenOne = ScreenManager::instance().screen(0);
	ASSERT_NE(screenOne, nullptr) << "Screen one is null";
}

TEST_F(ScreenManagerTest, PrimaryScreenIsNotNull)
{
	QScreen* primaryScreen = ScreenManager::instance().primaryScreen();
	ASSERT_NE(primaryScreen, nullptr) << "Primary screen is null";
}
