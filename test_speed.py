import asyncio
import time
from winrt.windows.media.control import GlobalSystemMediaTransportControlsSessionManager as MediaManager

async def test_speed():
    start = time.time()
    for _ in range(20):
        sessions = await MediaManager.request_async()
        current_session = sessions.get_current_session()
        if current_session:
            t = current_session.get_timeline_properties()
    end = time.time()
    print("Avg time:", (end - start) / 20.0)

if __name__ == "__main__":
    asyncio.run(test_speed())
