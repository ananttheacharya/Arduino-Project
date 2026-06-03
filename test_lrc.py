import urllib.request
import urllib.parse
import json

track = "Singh Is Kinng - Title Song"
artist = "RDB"
url = f"https://lrclib.net/api/get?track_name={urllib.parse.quote(track)}&artist_name={urllib.parse.quote(artist)}"
try:
    req = urllib.request.Request(url, headers={'User-Agent': 'Cyberdeck/1.0'})
    with urllib.request.urlopen(req) as response:
        data = json.loads(response.read().decode())
        print(data.get('syncedLyrics'))
except Exception as e:
    print("Error:", e)
