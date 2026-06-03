import urllib.request
import urllib.parse
import json

url = "https://lrclib.net/api/search?q=" + urllib.parse.quote("Singh Is Kinng RDB")
try:
    req = urllib.request.Request(url, headers={'User-Agent': 'Cyberdeck/1.0'})
    with urllib.request.urlopen(req) as response:
        data = json.loads(response.read().decode())
        for d in data[:3]:
            print(d.get('trackName'), d.get('artistName'), "Synced:", bool(d.get('syncedLyrics')))
except Exception as e:
    print("Error:", e)
