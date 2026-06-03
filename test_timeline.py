import asyncio
from winrt.windows.media.control import GlobalSystemMediaTransportControlsSessionManager as MediaManager

async def test_timeline():
    sessions = await MediaManager.request_async()
    current_session = sessions.get_current_session()
    if current_session:
        timeline = current_session.get_timeline_properties()
        print(type(timeline.position))
        print("Pos:", timeline.position.total_seconds())
        print("End:", timeline.end_time.total_seconds())
    else:
        print("No session")

if __name__ == "__main__":
    asyncio.run(test_timeline())
