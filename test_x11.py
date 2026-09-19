import time
import subprocess

# Wait for app to be ready
time.sleep(2)
# Click to open the dashboard
subprocess.run(['xdotool', 'mousemove', '10', '10', 'click', '1'])
time.sleep(1)
# Click the first bento card (it should be roughly at 200, 200 depending on screen)
subprocess.run(['xdotool', 'mousemove', '200', '200', 'click', '1'])
time.sleep(1)
# Type something
subprocess.run(['xdotool', 'type', 'HELLO WORLD'])
time.sleep(1)
# Click close
subprocess.run(['xdotool', 'mousemove', '750', '25', 'click', '1']) # Assuming close button is near top right
