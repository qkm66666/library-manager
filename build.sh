#!/bin/bash

# Function to clean up background processes on exit
cleanup() {
    echo "Shutting down servers..."
    # Kill the backend Flask server
    if [ ! -z "$BACKEND_PID" ]; then
        kill $BACKEND_PID
        echo "Backend server (PID: $BACKEND_PID) stopped."
    fi
    # Kill the frontend http.server
    if [ ! -z "$FRONTEND_PID" ]; then
        kill $FRONTEND_PID
        echo "Frontend server (PID: $FRONTEND_PID) stopped."
    fi
    exit
}

# Trap script exit signals (like Ctrl+C) to call the cleanup function
trap cleanup SIGINT SIGTERM

# --- Start Backend Server ---
echo "Starting Python Flask backend server on port 8080..."
# Run app.py in the background and store its PID
python app.py &
BACKEND_PID=$!
echo "Backend server started with PID: $BACKEND_PID"
# Give the server a moment to start, important for the browser to connect
sleep 2

# --- Start Frontend Server ---
# The frontend files are in the root directory
echo "Starting frontend server for static files on port 8000..."
# Run a simple Python HTTP server in the background from the root directory
python -m http.server 8000 &
FRONTEND_PID=$!
echo "Frontend server started with PID: $FRONTEND_PID"
sleep 1

# --- Open Browser ---
FRONTEND_URL="http://localhost:8000/index.html"
echo "Opening browser at $FRONTEND_URL"

# Use the appropriate command to open the browser based on the OS
# This handles Windows (Git Bash/MSYS), macOS, and Linux
case "$(uname -s)" in
   Linux*)     xdg-open "$FRONTEND_URL" ;;
   Darwin*)    open "$FRONTEND_URL" ;;
   CYGWIN*|MINGW*|MSYS*) start "$FRONTEND_URL" ;;
   *)          echo "Could not detect OS to open browser automatically." ;;
esac

# --- Wait for user to exit ---
echo "Servers are running. Press Ctrl+C in this terminal to stop both servers."
# The 'wait' command will pause the script here until the background processes finish
# or until the script is terminated by the user (e.g., with Ctrl+C).
wait $BACKEND_PID
wait $FRONTEND_PID
