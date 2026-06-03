import asyncio
from winrt.windows.media.control import GlobalSystemMediaTransportControlsSessionManager as MediaManager

async def test_playback():
    sessions = await MediaManager.request_async()
    current_session = sessions.get_current_session()
    if current_session:
        pb = current_session.get_playback_info()
        print("Status:", pb.playback_status)
        print("Type:", type(pb.playback_status))

if __name__ == "__main__":
    asyncio.run(test_playback())
