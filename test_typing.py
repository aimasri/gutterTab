import time
import subprocess

subprocess.Popen(['xvfb-run', '-a', './build/gutterTab'])
time.sleep(3)

# Click dashboard
subprocess.run(['xdotool', 'mousemove', '4', '100', 'click', '1'])
time.sleep(1)

# Click first card
subprocess.run(['xdotool', 'mousemove', '300', '300', 'click', '1'])
time.sleep(1)

# Type
subprocess.run(['xdotool', 'type', 'HELLO HTML TEST'])
time.sleep(1)

subprocess.run(['killall', 'gutterTab'])
