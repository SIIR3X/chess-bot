#include "pch.h"
#include "ScreenManager.h"

class ScreenManagerTest : public ::testing::Test
{
protected:
    void SetUp() override {}

    void TearDown() override {}
};

TEST_F(ScreenManagerTest, ScreensAreAvailable) {
	auto screens = ScreenManager::instance().screens();
	EXPECT_FALSE(screens.empty()) << "No screens available";
	ASSERT_FALSE(screens.empty()) << "No screens available";
}

TEST_F(ScreenManagerTest, ScreenZeroIsValid) {
    const ScreenInfo* screenZero = ScreenManager::instance().screen(0);
    ASSERT_NE(screenZero, nullptr) << "Screen at index 0 is null";
    EXPECT_GT(screenZero->width, 0);
    EXPECT_GT(screenZero->height, 0);
}

TEST_F(ScreenManagerTest, PrimaryScreenIsNotNull) {
    const ScreenInfo* primary = ScreenManager::instance().primaryScreen();
    ASSERT_NE(primary, nullptr) << "Primary screen is null";
    EXPECT_GT(primary->width, 0);
    EXPECT_GT(primary->height, 0);
}
