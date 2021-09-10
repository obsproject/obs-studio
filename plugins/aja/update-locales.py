import os

if __name__ == "__main__":
    locale_dir = os.path.join(os.getcwd(), "plugins/aja/data/locale")
    locales = os.listdir(locale_dir)
    english = []
    with open(os.path.join(locale_dir, "en-US.ini"), "r") as en_us:
        english = en_us.readlines()
    
    for l in locales:
        with open(os.path.join(locale_dir, l), "w") as loc:
            print(f"Writing en-US.ini into locale file {l}")
            for e in english:
                loc.write(f"{e}")
