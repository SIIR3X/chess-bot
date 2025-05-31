#include "pch.h"
#include "ScreenRecorder.h"

class ScreenRecorderTest : public ::testing::Test
{
protected:
	void SetUp() override {}

	void TearDown() override {}
};

TEST_F(ScreenRecorderTest, InitializeFailsWithInvalidOutputIndex)
{
	ScreenRecorder recorder;
	EXPECT_FALSE(recorder.initialize(UINT_MAX)); // Invalid index
}

TEST_F(ScreenRecorderTest, StartFailsWithoutInitialization)
{
	ScreenRecorder recorder;
	EXPECT_FALSE(recorder.startRecording([](const cv::Mat&) {}));
}

TEST_F(ScreenRecorderTest, StartAndStopRecording)
{
	ScreenRecorder recorder;

	if (!recorder.initialize(0)) return;

	std::atomic<int> frameCount = 0;

	bool started = recorder.startRecording([&frameCount](const cv::Mat& frame) {
		frameCount++;
	});
	EXPECT_TRUE(started);

	std::this_thread::sleep_for(std::chrono::milliseconds(200));

	recorder.stopRecording();
	EXPECT_GT(frameCount.load(), 0); // Ensure we captured some frames
}

TEST_F(ScreenRecorderTest, StopRecordingIsIdempotent)
{
	ScreenRecorder recorder;

	if (!recorder.initialize(0)) return;
	
	recorder.startRecording([](const cv::Mat&) {});
	recorder.stopRecording(); // First stop

	EXPECT_NO_THROW({
		recorder.stopRecording(); // Second stop should not throw
	});
}