import os
import subprocess

def run_command(command):
    print(f"Running command: {command}")
    result = subprocess.run(command, shell=True, capture_output=True, text=True)
    if result.stdout:
        print(f"Output: {result.stdout}")
    if result.stderr:
        print(f"Error: {result.stderr}")
    return result

# Print all environment variables
print("Environment Variables:")
for key, value in os.environ.items():
    print(f"{key}={value}")

# Run the command to install the Sentry CLI
run_command('curl -sL https://sentry.io/get-cli/ | bash')

def process_sentry(directory):
    print(f"Processing directory: {directory}")
    if not os.path.exists(directory):
        print(f"Error: Directory {directory} does not exist!")
        return

    # List the contents of the directory
    print(f"Listing contents of directory: {directory}")
    run_command(f'ls -la {directory}')

    for root, dirs, files in os.walk(directory):
        print(f"Current directory: {root}")
        print(f"Subdirectories: {dirs}")
        print(f"Files: {files}")
        for file in files:
            if '.so' in file or '.dylib' in file or '.' not in file:
                path = os.path.join(root, file)
                print(f"Processing file: {path}")
                
                # Run dsymutil on the file
                run_command(f"dsymutil {path}")
                
                # Upload the debug file to Sentry
                sentry_command = f"sentry-cli --auth-token {os.environ.get('SENTRY_AUTH_TOKEN', '')} upload-dif --org streamlabs-desktop --project obs-server {path}.dSYM/Contents/Resources/DWARF/{file}"
                run_command(sentry_command)
                
                # Repeat the upload for the second project
                sentry_command_preview = f"sentry-cli --auth-token {os.environ.get('SENTRY_AUTH_TOKEN', '')} upload-dif --org streamlabs-desktop --project obs-server-preview {path}.dSYM/Contents/Resources/DWARF/{file}"
                run_command(sentry_command_preview)

# Check if the required environment variables are set
required_env_vars = ['PWD', 'InstallPath', 'SENTRY_AUTH_TOKEN']
for var in required_env_vars:
    if var not in os.environ:
        print(f"Warning: Environment variable {var} is not set!")

# Upload obs debug files
process_sentry(os.path.join(os.environ.get('PWD', ''), "build" , os.environ.get('InstallPath', '')))
