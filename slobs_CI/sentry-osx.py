import os
os.system('curl -sL https://sentry.io/get-cli/ | bash')

def process_sentry(directory):
    for root, dirs, files in os.walk(directory):
        for file in files:
            if '.so' in file or '.dylib' in file or '.' not in file:
                path = os.path.join(root, file)
                os.system("dsymutil " + path)
                os.system("sentry-cli --auth-token ${SENTRY_AUTH_TOKEN} upload-dif --org streamlabs-desktop --project obs-server " + path + ".dSYM/Contents/Resources/DWARF/" + file)                
                os.system("dsymutil " + path)
                os.system("sentry-cli --auth-token ${SENTRY_AUTH_TOKEN} upload-dif --org streamlabs-desktop --project obs-server-preview " + path + ".dSYM/Contents/Resources/DWARF/" + file)

# Upload obs debug files
process_sentry(os.path.join(os.environ['PWD'], 'build'))