from unidecode import unidecode
hindi_text = "ये दिल आशिकाना"
with open("test_hindi_out.txt", "w", encoding="utf-8") as f:
    f.write(f"Romanized: {unidecode(hindi_text)}\n")
