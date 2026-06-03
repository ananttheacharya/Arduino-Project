import asyncio
import traceback
from winrt.windows.media.control import GlobalSystemMediaTransportControlsSessionManager as MediaManager

async def test_media():
    try:
        sessions = await MediaManager.request_async()
        current_session = sessions.get_current_session()
        if current_session:
            print("Session found:", current_session.source_app_user_model_id)
            info = await current_session.try_get_media_properties_async()
            print("Title:", info.title)
            print("Artist:", info.artist)
        else:
            print("No current session.")
    except Exception as e:
        print("Media error:")
        traceback.print_exc()

if __name__ == "__main__":
    asyncio.run(test_media())
