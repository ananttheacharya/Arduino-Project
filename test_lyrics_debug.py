import urllib.request
import urllib.parse
import json
import re
from unidecode import unidecode

def clean_title(title):
    t = re.sub(r'\(.*?\)', '', title)
    t = t.split('-')[0]
    return t.strip()

title = "Singh Is Kinng"
artist = "RDB"

queries = [
    f"{title} {artist}",
    f"{clean_title(title)} {artist}",
    f"{clean_title(title)}"
]

for q in queries:
    print("Querying:", q)
    try:
        url = "https://lrclib.net/api/search?q=" + urllib.parse.quote(q)
        req = urllib.request.Request(url, headers={'User-Agent': 'Cyberdeck/1.0'})
        with urllib.request.urlopen(req, timeout=5) as response:
            data = json.loads(response.read().decode())
            print("Results count:", len(data) if isinstance(data, list) else data)
            for d in data:
                if d.get('syncedLyrics') and (artist.lower() in (d.get('artistName') or "").lower()):
                    print("Found exact artist match!")
                    break
            else:
                if q == queries[-1]:
                    for d in data:
                        if d.get('syncedLyrics'):
                            print("Found fallback match!")
                            break
    except Exception as e:
        print("Error:", e)
