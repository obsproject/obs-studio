#!/bin/bash

# Check if a file path is passed as an argument
if [ -z "$1" ]; then
    echo "Usage: $0 <path_to_binary>"
    exit 1
fi

BINARY_PATH=$1

# Use otool to get the list of linked libraries
LIB_PATHS=$(otool -L "$BINARY_PATH" | grep "obs-deps" | awk '{print $1}')

# Check if any obs-deps libraries were found
if [ -z "$LIB_PATHS" ]; then
    echo "No obs-deps libraries found in $BINARY_PATH."
else
    # Loop through each library path and change it to @loader_path, removing version from the name
    for OLD_PATH in $LIB_PATHS; do
        # Extract the base library name without version
        LIB_NAME=$(basename "$OLD_PATH" | sed -E 's/\.[0-9]+\.dylib$/.dylib/')
        
        # Construct the new path using @loader_path
        NEW_PATH="@loader_path/$LIB_NAME"
        
        # Print what we are changing for logging
        echo "Changing $OLD_PATH to $NEW_PATH"
        
        # Run the install_name_tool command to make the change
        install_name_tool -change "$OLD_PATH" "$NEW_PATH" "$BINARY_PATH"
    done

    echo "All obs-deps libraries have been updated to use @loader_path and version-less names in $BINARY_PATH."
fi

# Additional libraries that use @rpath should be converted to @loader_path
OTHER_LIBS=$(otool -L "$BINARY_PATH" | grep "@rpath" | awk '{print $1}')

if [ -n "$OTHER_LIBS" ]; then
    for OLD_PATH in $OTHER_LIBS; do
        # Extract the base library name without version
        LIB_NAME=$(basename "$OLD_PATH" | sed -E 's/\.[0-9]+\.dylib$/.dylib/')
        
        # Construct the new path using @loader_path
        NEW_PATH="@loader_path/$LIB_NAME"
        
        # Print what we are changing for logging
        echo "Changing $OLD_PATH to $NEW_PATH"
        
        # Run the install_name_tool command to make the change
        install_name_tool -change "$OLD_PATH" "$NEW_PATH" "$BINARY_PATH"
    done

    echo "All @rpath libraries have been updated to use @loader_path in $BINARY_PATH."
else
    echo "No @rpath libraries found."
fi

# Add @executable_path/../Frameworks to rpath as a fallback
echo "Adding @executable_path/../Frameworks as an rpath fallback"
install_name_tool -add_rpath "@executable_path/../Frameworks" "$BINARY_PATH"

# Check if the binary is signed (for macOS app distribution)
CODESIGN_STATUS=$(codesign -vv "$BINARY_PATH" 2>&1)

if [[ "$CODESIGN_STATUS" == *"not signed"* ]]; then
    echo "Binary is not signed. Please sign it for distribution if necessary."
else
    echo "Binary is already signed."
fi

echo "All modifications are done."
