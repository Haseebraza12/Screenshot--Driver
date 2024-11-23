import os
import signal
import time
from PIL import ImageGrab  # To take screenshots
import fcntl  # For file descriptor flags

# File path for communication with the kernel module
DEVICE_FILE = "/dev/input/mouse_screenshot"

def handle_signal(signum, frame):
    """
    Signal handler for SIGIO.
    """
    print("Signal received! Taking a screenshot...")
    # Take a screenshot and save it
    screenshot = ImageGrab.grab()
    screenshot.save(f"screenshot_{int(time.time())}.png")
    print("Screenshot saved.")

def main():
    # Open the device file for the kernel module
    try:
        fd = os.open(DEVICE_FILE, os.O_RDONLY)
    except FileNotFoundError:
        print(f"Error: Device file {DEVICE_FILE} not found.")
        print("Make sure the kernel module is loaded and the device file exists.")
        return
    except PermissionError:
        print(f"Error: Permission denied to access {DEVICE_FILE}. Try running with sudo.")
        return

    # Set up the signal handler for SIGIO
    signal.signal(signal.SIGIO, handle_signal)

    # Enable asynchronous I/O on the file descriptor
    try:
        fcntl.fcntl(fd, fcntl.F_SETOWN, os.getpid())  # Set the owner of the file descriptor
        flags = fcntl.fcntl(fd, fcntl.F_GETFL)  # Get current flags
        fcntl.fcntl(fd, fcntl.F_SETFL, flags | os.O_ASYNC)  # Enable asynchronous I/O
    except Exception as e:
        print(f"Error setting up asynchronous I/O: {e}")
        os.close(fd)
        return

    print("Listening for signals... Press Ctrl+C to exit.")
    try:
        while True:
            time.sleep(1)  # Keep the script running
    except KeyboardInterrupt:
        print("\nExiting...")
        os.close(fd)

if __name__ == "__main__":
    main()
