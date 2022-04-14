import os
os.system('curl -sL https://sentry.io/get-cli/ | bash')

def process_sentry(directory):
    for r, d, f in os.walk(directory):
        for file in f:
            if 'lib' in file or 'obs' in file or '.so' in file or '.dylib' in file:
                path = os.path.join(directory, file)
                os.system("dsymutil " + path)
                os.system("sentry-cli --auth-token ${SENTRY_AUTH_TOKEN} upload-dif --org streamlabs-desktop --project obs-server " + path + ".dSYM/Contents/Resources/DWARF/" + file)
    for r, d, f in os.walk(directory):
        for file in f:
            if 'lib' in file or 'obs' in file or '.so' in file or '.dylib' in file:
                path = os.path.join(directory, file)
                os.system("dsymutil " + path)
                os.system("sentry-cli --auth-token ${SENTRY_AUTH_TOKEN} upload-dif --org streamlabs-desktop --project obs-server-preview " + path + ".dSYM/Contents/Resources/DWARF/" + file)

# Upload obs debug files
process_sentry(os.path.join(os.environ['PWD'], 'build', 'rundir', os.environ['BUILDCONFIG'], 'bin'))
# Upload obs-plugins debug files
process_sentry(os.path.join(os.environ['PWD'], 'build', 'rundir', os.environ['BUILDCONFIG'], 'obs-plugins'))